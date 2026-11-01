#include "tokenizer.h"
#include "simdbp128.h"
#include "index.h"
#include "coder.h"
#include "fst.h"
#include "helper.h"
#include "crc.h"
#include "fls.h"
#include "pef.h"
#include "segment.h"
#include "metadata.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <thread>

constexpr size_t PARTITION_SIZE = 128 << 20;

void create_index(const std::vector<std::string> &fields,
                  const std::string &index_dir_str,
                  const std::string &data_dir_str, 
                  const uint8_t enc_type,
                  const uint32_t partition_size,
                  const uint8_t num_threads,
                  const bool run_checks) {
  const std::filesystem::path index_dir = index_dir_str;
  const std::filesystem::path data_dir = data_dir_str;

  // Create index directory, fails gracefully if directory already exists
  std::filesystem::create_directory(index_dir);

  // Check data directory exists
  if (!std::filesystem::exists(data_dir)) {
    throw std::invalid_argument("Data directory does not exist");
  }

  // Get list of supported files to index
  std::vector<std::filesystem::path> files;
  for (const auto &entry : std::filesystem::directory_iterator(data_dir)) {
    if (entry.is_regular_file() && valid_extension(entry.path().extension())) {
      files.push_back(entry.path());
    }
  }

  std::vector<std::string> file_mappings_vec(files.size());

  std::vector<std::vector<uint64_t>> all_segment_offsets(files.size());
  std::vector<std::vector<uint64_t>> all_segment_docid_modifiers(files.size());

  for (size_t file_id = 0; file_id < files.size(); ++file_id) {
    file_mappings_vec[file_id] = files[file_id].filename().string();

    std::cout << "Segmenting File: " << file_mappings_vec[file_id] << "\n";

    std::vector<uint64_t> segment_offsets;
    std::vector<uint64_t> segment_docid_modifiers;
    partition_file(files[file_id], 0, partition_size, segment_offsets, segment_docid_modifiers);

  {
    const MappedFile mf = open_mmap_r(files[file_id]);
    const char *p = static_cast<const char *>(mf.addr());
    for (size_t k = 1; k + 1 < segment_offsets.size(); ++k)
      if (p[segment_offsets[k] - 1] != '\n')
        std::cerr << "bad boundary " << k << " at byte " << segment_offsets[k] << '\n';
  }

    if (segment_offsets.size() < 2) continue; // Empty file

    all_segment_offsets[file_id] = std::move(segment_offsets);
    all_segment_docid_modifiers[file_id] = std::move(segment_docid_modifiers);
  }

  index_files(fields, index_dir, data_dir, enc_type, file_mappings_vec, all_segment_offsets, all_segment_docid_modifiers, num_threads, run_checks);
  
  Metadata metadata = Metadata::create(std::move(index_dir.string()), std::move(all_segment_offsets), std::move(all_segment_docid_modifiers), std::move(file_mappings_vec), enc_type);

  create_checksum_file(index_dir_str);
}

void index_files(const std::vector<std::string> &fields,
                const std::filesystem::path &index_dir,
                const std::filesystem::path &data_dir,
                const uint8_t enc_type,
                std::vector<std::string> &file_mappings_vec,
                std::vector<std::vector<uint64_t>> &all_segment_offsets,
                std::vector<std::vector<uint64_t>> &all_segment_docid_modifiers,
                const uint8_t num_threads,
                const bool run_checks) {
  
  // Build segment tasks
  std::vector<std::pair<uint64_t, uint64_t>> segment_tasks;
  for (uint64_t i = 0; i < all_segment_offsets.size(); ++i) {
    for (uint64_t j = 0; j + 1 < all_segment_offsets[i].size(); ++j) {
      segment_tasks.push_back({i, j});
    }
  }

  uint64_t num_segments = segment_tasks.size();

  std::cout << " Indexing " + std::to_string(file_mappings_vec.size()) + " File\\s:\n";

  const size_t n_threads = std::min<size_t>(num_segments, num_threads);
  std::cout << "  Threads Utilized: " << n_threads << "\n";
  std::cout << "\r  Processed: " << "0.00%" << std::flush;

  std::atomic<size_t> next{0};
  std::atomic<size_t> num_segments_completed{0};
  std::mutex print_mtx;
  std::mutex err_mtx;
  std::exception_ptr first_error;
  std::stop_source stop;

  auto worker = [&] {
    while (!stop.stop_requested()) {
      const size_t task_id = next.fetch_add(1, std::memory_order_relaxed);
      if (task_id >= num_segments) break; // Last partition marks EOF
      
      try {
        auto [file_id, segment_id] = segment_tasks[task_id];
        std::string data_path = (data_dir / file_mappings_vec[file_id]).string();
        create_segment(data_path, fields, index_dir, file_id, segment_id, all_segment_offsets[file_id], enc_type, run_checks);
        const size_t curr_completed = num_segments_completed.fetch_add(1, std::memory_order_relaxed) + 1;

        // Print progress indicator
        {
          std::lock_guard lk(print_mtx);
          std::cout << "\r  Processed: " << (100.0 * curr_completed / num_segments) << "%" << std::flush;
        }
      } catch (...) {
        std::lock_guard lk(err_mtx);
        if (!first_error) first_error = std::current_exception();
        stop.request_stop();
      }
    }
  };
  {
  std::vector<std::jthread> pool;
  pool.reserve(n_threads);
  for (size_t t = 0; t < n_threads; ++t) pool.emplace_back(worker);
  }

  // erase progress line
  std::cout << "\r\x1b[2K" << std::flush;
  std::cout << "Completed\n" << std::flush;

  if (first_error) std::rethrow_exception(first_error);
}

void update_index(const std::vector<std::string> &fields,
                  const std::string &index_dir_str,
                  const std::string &data_dir_str,
                  const bool run_checks) {
  const std::filesystem::path index_dir = index_dir_str;
  const std::filesystem::path data_dir = data_dir_str;

  // Check data directory exists
  if (!std::filesystem::exists(data_dir)) {
    throw std::invalid_argument("Data directory does not exist");
  }

  Metadata metadata = Metadata::read(index_dir);

  for (auto &f : metadata.get_file_mappings()) {
    std::cout << "existing: " << f << std::endl;
  }

  // Get list of files currently indexed
  std::vector<std::string> files_indexed = metadata.get_file_mappings();

  // Get list of supported files within data directory
  std::vector<std::filesystem::path> files_current;
  for (const auto &entry : std::filesystem::directory_iterator(data_dir)) {
    if (entry.is_regular_file() && valid_extension(entry.path().extension())) {
      files_current.push_back(entry.path());
    }
  }

  std::vector<std::string> new_files;

  for (auto &f_cur : files_current) {
    auto name = f_cur.filename().string();
    if (std::ranges::find(files_indexed, name) == files_indexed.end())
      new_files.push_back(name);
  }

  for (auto &f : new_files) std::cout << "New file: " << f << '\n';


  std::vector<std::string> files_to_update;
  for (auto &f : files_indexed) {
    uint64_t num_documents;
    switch (str_to_file_type(f)) {
      case FileType::CSV:
        num_documents = count_documents_csv(data_dir.string() + '/' + f);
        break;
      case FileType::JSON:
        num_documents = count_documents_json(data_dir.string() + '/' + f);
        break;
      case FileType::LOGFMT:
        num_documents = count_documents_logfmt(data_dir.string() + '/' + f);
        break;
    }

    uint64_t file_id = metadata.get_file_id(f);
    uint64_t num_documents_prev = metadata.get_num_documents(file_id);

    if (num_documents != num_documents_prev) {
      files_to_update.push_back(f);
    }
  }

  // Add in new files (appended, so existing file_ids are unchanged)
  std::vector<std::string> file_mappings_vec = metadata.get_file_mappings();
  uint64_t file_id = file_mappings_vec.size();
  std::vector<std::vector<uint64_t>> all_segment_offsets = metadata.get_all_segment_offsets();
  std::vector<std::vector<uint64_t>> all_segment_docid_modifiers = metadata.get_all_segment_docid_modifiers();

  // Make room for the new files so they can be written at vec[file_id]
  const size_t total_files = file_mappings_vec.size() + new_files.size();
  file_mappings_vec.resize(total_files);
  all_segment_offsets.resize(total_files);
  all_segment_docid_modifiers.resize(total_files);

  // Only new files get offsets here, so index_files skips the existing ones
  std::vector<std::vector<uint64_t>> new_segment_offsets(total_files);
  for (auto &f : new_files) {
      std::cout << data_dir.string() + '/' + f << '\n';
    file_mappings_vec[file_id] = f;
    partition_file(data_dir / f, 0, PARTITION_SIZE, new_segment_offsets[file_id], all_segment_docid_modifiers[file_id]);
    if (new_segment_offsets[file_id].size() >= 2) {
      all_segment_offsets[file_id].assign(new_segment_offsets[file_id].begin(), new_segment_offsets[file_id].end() - 1);
    } else { // Empty file
      new_segment_offsets[file_id].clear();
      all_segment_docid_modifiers[file_id].clear();
    }
    ++file_id;
  }

  index_files(fields, index_dir, data_dir, metadata.get_enc_type(), file_mappings_vec, new_segment_offsets, all_segment_docid_modifiers, 1, run_checks);

  metadata.update_metadata_file(std::move(all_segment_offsets), std::move(all_segment_docid_modifiers), std::move(file_mappings_vec));
  
  // Update existing files
  new_segment_offsets.clear();
  for (auto &f : files_to_update) {
    uint64_t file_id = metadata.get_file_id(f);
    uint64_t old_file_size = metadata.get_file_size(file_id);
    uint64_t new_file_size = std::filesystem::file_size(data_dir / f);
    partition_file(data_dir / f, 0, PARTITION_SIZE, new_segment_offsets[file_id], all_segment_docid_modifiers[file_id]);
  }

  index_files(fields, index_dir, data_dir, metadata.get_enc_type(), file_mappings_vec, new_segment_offsets, all_segment_docid_modifiers, 1, run_checks);
  metadata.update_metadata_file(std::move(all_segment_offsets), std::move(all_segment_docid_modifiers), std::move(file_mappings_vec));
}

void merge_index(const std::vector<std::string> &fields,
                  const std::string &index_dir_str,
                  const std::string &data_dir_str, 
                  const bool run_checks) {
  const std::filesystem::path index_dir = index_dir_str;
  const std::filesystem::path data_dir = data_dir_str;

  // Determine appropriate segments to merge
  // Appropriate segments, when combined, remain below the 
  // max segment parition size
  Metadata metadata = Metadata::read(index_dir);

  uint64_t num_files = metadata.get_num_files();

  using Merge = std::tuple<uint64_t, uint64_t, uint64_t>;
  std::vector<std::vector<Merge>> merges(num_files);
  for (uint64_t file_id = 0; file_id < num_files; ++file_id) {
    uint64_t num_segments = metadata.get_num_segments(file_id);
    std::vector<uint64_t> segment_offsets = metadata.get_all_segment_offsets()[file_id];
    std::vector<uint64_t> segment_sizes = metadata.get_segment_sizes(file_id);

    std::vector<Merge> proposed_merges;
    
    // Determine all segments that are below segment partition size when combined
    uint64_t w_min = 0;
    for (uint64_t w_min = 0; w_min < segment_sizes.size() - 1; ++w_min) {
      for (uint64_t i = w_min + 1; i < segment_sizes.size(); ++i) {
        uint64_t combined_size = segment_sizes[w_min] + segment_sizes[i];
        if (segment_sizes[w_min] + segment_sizes[i] <= PARTITION_SIZE) {
          proposed_merges.push_back({w_min, i, combined_size});
        }
      }
    }

    // Choose "best" segments to combine when segments overlap
    // E.g Segments 1,2 and 2,3 can both be combined
    // Choose combination with sizes closest to max partition size
    std::sort(proposed_merges.begin(), proposed_merges.end(), 
    [&](const Merge& a, const Merge& b) 
    { 
        uint64_t a_size = std::get<2>(a);
        uint64_t b_size = std::get<2>(b);
        return (PARTITION_SIZE - a_size) < (PARTITION_SIZE - b_size); 
    });

    std::unordered_set<uint64_t> used;
    used.reserve(proposed_merges.size() * 2);

    std::vector<Merge> result;
    for (const auto& merge : proposed_merges) {
        auto [a, b, size] = merge;
        if (used.count(a) || used.count(b))
            continue;
        used.insert(a);
        used.insert(b);
        result.push_back(merge);
    }

    merges[file_id] = std::move(result);
  }

  for (auto &per_file_merges : merges) {
    for (auto &merge : per_file_merges) {
        merge_segments(fields, index_dir_str, data_dir_str, metadata.get_enc_type());
    }
  }
}

void merge_segments(const std::vector<std::string> &fields,
                  const std::string &index_dir_str,
                  const std::string &data_dir_str, 
                  const bool run_checks) {
  const std::filesystem::path index_dir = index_dir_str;
  const std::filesystem::path data_dir = data_dir_str;
}

void create_segment(const std::string &data_path,
                    const std::vector<std::string> &fields,
                    const std::filesystem::path &index_dir,
                    const size_t file_id,
                    const size_t segment_id,
                    const std::vector<uint64_t> &segment_offsets,
                    const uint8_t enc_type,
                    const bool run_checks) {
  const uint64_t segment_offset = segment_offsets[segment_id];
  const uint64_t segment_size = segment_offsets[segment_id + 1] - segment_offset;
  const std::string segment_path = index_dir.string() + '/' + std::to_string(file_id) + "_segment_" + std::to_string(segment_id) + ".seg"; // File_id;


  Tokenizer tokenizer = Tokenizer::from_file(data_path, fields, segment_offset, segment_size);

  std::unique_ptr<char[]> output_buf(new char[16 << 20]);
  std::ofstream segment_file;
  segment_file.rdbuf()->pubsetbuf(output_buf.get(), 16 << 20);
  segment_file.open(segment_path, std::ios::binary);


  // Leave room at start of file for fst offset positions
  Segment_header segment_header = {0,0,0,0};
  segment_file.write(reinterpret_cast<const char*>(&segment_header), sizeof(segment_header));

  generate_offsets(tokenizer, segment_file, enc_type, run_checks);
  generate_fsts(tokenizer, segment_file, run_checks);

  segment_file.close();

  if (run_checks) {
    check_fst(tokenizer.getAlphaTokens(), segment_path, FstType::ALPHA);
    check_fst(tokenizer.getFloatTokens(), segment_path, FstType::FLOAT);
    check_fst(tokenizer.fields, segment_path, FstType::FIELDS);
    check_offsets_file(tokenizer, segment_path, enc_type);
    check_all_pef_tokens(tokenizer, segment_path);
  }
}


void generate_fsts(Tokenizer &tokenizer, std::ofstream &file, const bool run_checks) {
    StdCompactFst alpha_fst = create_fst<std::string>(tokenizer.getAlphaTokens());
    size_t alpha_pos = write_to_disk(alpha_fst, file);
    update_alpha_fst_pos(file, alpha_pos);
    
    if (!run_checks)
      tokenizer.alpha_tokens.clear();

    StdCompactFst float_fst = create_fst<std::vector<uint8_t>>(tokens_from_map<std::vector<uint8_t>>(sorted_view(tokenizer.float_tokens)));
    size_t float_pos =  write_to_disk(float_fst, file);
    update_float_fst_pos(file, float_pos);
    
    if (!run_checks)
      tokenizer.float_tokens.clear();
    
    StdCompactFst fields_fst = create_fst<std::string>(tokenizer.fields);
    size_t fields_pos = write_to_disk(fields_fst, file);  
    update_fields_fst_pos(file, fields_pos); 

    if (!run_checks)
      tokenizer.fields.clear();


}

void generate_offsets(const Tokenizer &tokenizer,
                           std::ofstream &file,
                           const uint8_t enc_type,
                           const bool run_checks) {

  Offsets_header header = {};
  const std::streampos header_pos = file.tellp();
  file.write(reinterpret_cast<const char *>(&header),
             sizeof(header)); // Reserve for later use

  const Write_pos alpha_pos =
      write_offsets_to_file(sorted_view(tokenizer.alpha_tokens), file, enc_type);
  const Write_pos num_pos =
      write_offsets_to_file(sorted_view(tokenizer.num_tokens), file, enc_type);
  const Write_pos neg_num_pos =
      write_offsets_to_file(sorted_view(tokenizer.neg_num_tokens), file, enc_type);
  const Write_pos float_pos =
      write_offsets_to_file(sorted_view(tokenizer.float_tokens), file, enc_type);
  const Write_pos date_pos =
      write_offsets_to_file(sorted_view(tokenizer.d_tokens), file, enc_type);
  const Write_pos time_pos =
      write_offsets_to_file(sorted_view(tokenizer.t_tokens), file, enc_type);
  const Write_pos ipv4_pos =
      write_offsets_to_file(sorted_view(tokenizer.ipv4_tokens), file, enc_type);
  const uint64_t entry_offsets_pos = write_entry_offsets_to_file(
      tokenizer.entry_offsets, file, enc_type); // Handled differently to above

  const uint64_t num_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getNumTokens(), file);
  const uint64_t neg_num_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getNegNumTokens(), file);
  const uint64_t date_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getDateTokens(), file);
  const uint64_t time_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getTimeTokens(), file);
  const uint64_t ip_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getIPTokens(), file);

  // Populate header with offset values then write to file
  header = {alpha_pos.offsets_pos,
            alpha_pos.id_map_pos,
            num_pos.offsets_pos,
            num_pos.id_map_pos,
            neg_num_pos.offsets_pos,
            neg_num_pos.id_map_pos,
            float_pos.offsets_pos,
            float_pos.id_map_pos,
            date_pos.offsets_pos,
            date_pos.id_map_pos,
            time_pos.offsets_pos,
            time_pos.id_map_pos,
            ipv4_pos.offsets_pos,
            ipv4_pos.id_map_pos,
            entry_offsets_pos,
            num_tok_off,
            neg_num_tok_off,
            date_tok_off,
            time_tok_off,
            ip_tok_off};
  
  // Write offsets_header
  const std::streampos current_pos = file.tellp();
  file.seekp(header_pos);
  file.write(reinterpret_cast<const char *>(&header),
             sizeof(header)); // Write header data
  file.seekp(current_pos);

  // Write offsets pos to segment_header
  update_offsets_header_pos(file, header_pos);
}

void check_all_pef_tokens(const Tokenizer &tokenizer,
                          const std::string &file_path) {
  check_pef_tokens<uint64_t>(tokenizer.getNumTokens(), file_path,
                             TokenType::PINT);
  check_pef_tokens<uint64_t>(tokenizer.getNegNumTokens(), file_path,
                             TokenType::NINT);
  check_pef_tokens<uint64_t>(tokenizer.getDateTokens(), file_path,
                             TokenType::DATE);
  check_pef_tokens<uint64_t>(tokenizer.getTimeTokens(), file_path,
                             TokenType::TIME);
  check_pef_tokens<uint64_t>(tokenizer.getIPTokens(), file_path,
                             TokenType::IPV4);
}

const uint64_t
write_entry_offsets_to_file(const std::vector<uint32_t> &entry_offsets,
                            std::ofstream &file, const uint8_t enc_type) {
  const uint64_t write_pos = file.tellp();                            
  
  if (entry_offsets.size() == 0)
    return write_pos;

  switch (enc_type) {
    case EncType::GVB:
      write_gvb_to_file<uint32_t>(entry_offsets, file);
      break;
    case EncType::PEF:
      write_pef_to_file<uint32_t>(entry_offsets, file);
      break;
    case EncType::FLS:
      write_fls_to_file(entry_offsets, file);
      break;
    case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
    case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM: 
      write_bp128_to_file(entry_offsets, file, enc_type);
      break;
    default:
      throw std::invalid_argument("Not a valid encoding type");
      break;
  }

  return write_pos;
}

void check_offsets_file(const Tokenizer &tokenizer,
                        const std::string &file_name, const uint8_t enc_type) {
  bool check = true;
  check = check && check_offsets_file_by_type(sorted_view(tokenizer.alpha_tokens), file_name, TokenType::ALPHA, enc_type);
  check = check && check_offsets_file_by_type(sorted_view(tokenizer.num_tokens), file_name, TokenType::PINT ,enc_type);
  check = check && check_offsets_file_by_type(sorted_view(tokenizer.neg_num_tokens), file_name, TokenType::NINT ,enc_type);
  check = check && check_offsets_file_by_type(sorted_view(tokenizer.float_tokens), file_name, TokenType::FLOAT, enc_type);
  check = check && check_offsets_file_by_type(sorted_view(tokenizer.d_tokens), file_name, TokenType::DATE, enc_type);
  check = check && check_offsets_file_by_type(sorted_view(tokenizer.t_tokens), file_name, TokenType::TIME, enc_type);
  check = check && check_offsets_file_by_type(sorted_view(tokenizer.ipv4_tokens), file_name, TokenType::IPV4, enc_type);
  check = check && check_entry_offsets(tokenizer.entry_offsets, file_name, enc_type);

  if (check) {
    std::cout << file_name << ": Offsets file correct" << std::endl;
  } else {
    std::cout << file_name << ": Offsets file incorrect" << std::endl;
  }
}

const bool check_entry_offsets(const std::vector<uint32_t> &entry_offsets,
                               const std::string &file_name,
                               const uint8_t enc_type) {
  std::vector<uint32_t> input =
      get_entry_offsets_from_file(file_name, enc_type);

  if (input == entry_offsets) {
    std::cout << "Entry offsets correct: " << std::endl;
    return true;
  } else {
    std::cout << "Entry offsets incorrect: " << std::endl;
    std::cout << "read in size: " << input.size() << std::endl;
    std::cout << "og size: " << entry_offsets.size() << std::endl;
    return false;
  }
}

const std::pair<uint64_t, uint64_t> get_id_offsets_setup(const Offsets_header &header, const uint8_t type) {
  uint64_t addr_offset = 0;
  uint64_t size_to_read = 0;

  switch (type) {
  case TokenType::ALPHA:
    size_to_read = header.num_offsets_header - header.alpha_id_map_header;
    addr_offset = header.alpha_id_map_header;
    break;
  case TokenType::PINT:
    size_to_read = header.neg_num_offsets_header - header.num_id_map_header;
    addr_offset = header.num_id_map_header;
    break;
  case TokenType::NINT:
    size_to_read = header.float_offsets_header - header.neg_num_id_map_header;
    addr_offset = header.neg_num_id_map_header;
    break;
  case TokenType::FLOAT:
    size_to_read = header.date_offsets_header - header.float_id_map_header;
    addr_offset = header.float_id_map_header;
    break;
  case TokenType::DATE:
    size_to_read = header.time_offsets_header - header.date_id_map_header;
    addr_offset = header.date_id_map_header;
    break;
  case TokenType::TIME:
    size_to_read = header.ipv4_offsets_header - header.time_id_map_header;
    addr_offset = header.time_id_map_header;
    break;
  case TokenType::IPV4:
    size_to_read = header.entry_offsets_header - header.ipv4_id_map_header;
    addr_offset = header.ipv4_id_map_header;
    break;
  default:
    throw std::invalid_argument("Not a valid TokenType");
  }

  return std::make_pair(addr_offset, size_to_read);
}

const std::vector<uint32_t>
get_id_offsets_from_file(const std::string &file_name, const uint8_t type,
                         const uint8_t enc_type) {
  const MappedFile mf = open_mmap_r(file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();
  // Read in header
  Offsets_header header = read_offsets_header(file);

  const auto &[addr_offset, size_to_read] = get_id_offsets_setup(header, type);

  if (!size_to_read)
    return std::vector<uint32_t>(); // No ids of requested type present in file

  switch (enc_type) {
    case EncType::GVB:
      return read_gvb_from_file<uint32_t>(file + addr_offset, size_to_read);
    case EncType::PEF:
      return read_pef_from_file<uint32_t>(file + addr_offset, size_to_read);
    case EncType::FLS: 
      return read_fls_from_file(file + addr_offset, size_to_read);
    case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
    case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM: 
      return read_bp128_from_file(file + addr_offset, size_to_read, enc_type);
    default:
      throw std::invalid_argument("Not a valid encoding type");
  }
}

const std::vector<uint32_t> get_tok_offsets_from_file(
  const uint64_t token_id, const std::vector<uint32_t> &id_to_offset, const std::vector<uint32_t> &field_ids,
  const std::string &file_name, const uint8_t type, const uint8_t enc_type) {
  const MappedFile mf = open_mmap_r(file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();
    
  const auto [length, offset] = get_tok_offsets_setup(id_to_offset, file, type);
  return get_tok_offsets(token_id, id_to_offset, field_ids, length, file + offset, enc_type);
}

const std::vector<uint32_t> get_tok_offsets_from_file(
  const Range token_ids, const std::vector<uint32_t> &id_to_offset, const std::vector<uint32_t> &field_ids,
  const std::string &file_name, const uint8_t type, const uint8_t enc_type) {
  const MappedFile mf = open_mmap_r(file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();

  const auto [length, offset] = get_tok_offsets_setup(id_to_offset, file, type);

  std::vector<uint32_t> offsets;

  const auto [min, max] = token_ids;
  for (size_t id = min; id <= max; ++id) {
    std::vector<uint32_t> cur_offsets =
        get_tok_offsets(id, id_to_offset, field_ids, length, file + offset, enc_type);
    offsets.insert(offsets.end(), std::make_move_iterator(cur_offsets.begin()),
                   std::make_move_iterator(cur_offsets.end()));
  }

  std::sort(offsets.begin(), offsets.end());

  return offsets;
}

const std::pair<uint64_t, uint64_t>
get_tok_offsets_setup(const std::vector<uint32_t> &id_to_offset,
                      const uint8_t *file, const uint8_t type) {
  // Read in header
  Offsets_header header = read_offsets_header(file);

  // Get offset to start of token offsets in file
  uint64_t tok_offsets_length = 0;
  uint64_t offset = 0;

  switch (type) {
    case TokenType::ALPHA:
      tok_offsets_length = header.alpha_id_map_header - header.alpha_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No alpha offsets present in file");
      offset = header.alpha_offsets_header;
      break;
    case TokenType::PINT:
      tok_offsets_length = header.num_id_map_header - header.num_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No int offsets present in file");
      offset = header.num_offsets_header;
      break;
    case TokenType::NINT:
      tok_offsets_length = header.neg_num_id_map_header - header.neg_num_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No int offsets present in file");
      offset = header.neg_num_offsets_header;
      break;
    case TokenType::FLOAT:
      tok_offsets_length = header.float_id_map_header - header.float_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No float offsets present in file");
      offset = header.float_offsets_header;
      break;
    case TokenType::DATE:
      tok_offsets_length = header.date_id_map_header - header.date_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No date offsets present in file");
      offset = header.date_offsets_header;
      break;
    case TokenType::TIME:
      tok_offsets_length = header.time_id_map_header - header.time_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No time offsets present in file");
      offset = header.time_offsets_header;
      break;
    case TokenType::IPV4:
      tok_offsets_length = header.ipv4_id_map_header - header.ipv4_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No ipv4 offsets present in file");
      offset = header.ipv4_offsets_header;
      break;
    default:
      throw std::invalid_argument("Not a valid token type");
  }

  return {tok_offsets_length, offset};
}

// Output converts offsets values from file from 1 indexing back to 0 indexing
const std::vector<uint32_t>
get_tok_offsets(const uint64_t token_id,
                const std::vector<uint32_t> &id_to_offset,
                const std::vector<uint32_t> &req_field_ids,
                const uint64_t tok_offsets_length,
                const uint8_t *file, const uint8_t enc_type) {
  uint64_t size_to_read;
  if (token_id == id_to_offset.size() - 1) {
    size_to_read = tok_offsets_length - id_to_offset[token_id];
  } else {
    size_to_read = id_to_offset[token_id + 1] - id_to_offset[token_id];
  }

  uint64_t addr_offset = id_to_offset[token_id];

  Fields_header f_header = {};
  // Get fields header
  std::memcpy(&f_header.num_fields, file + addr_offset, sizeof(uint16_t));
  size_t f_header_offset = addr_offset + sizeof(uint16_t);

  // Read field_ids
  f_header.field_ids.resize(f_header.num_fields);
  std::memcpy(f_header.field_ids.data(), file + f_header_offset, f_header.num_fields * sizeof(uint16_t));
  f_header_offset += f_header.num_fields * sizeof(uint16_t);

  // Read field_offsets
  f_header.field_offsets.resize(f_header.num_fields);
  std::memcpy(f_header.field_offsets.data(), file + f_header_offset, f_header.num_fields * sizeof(uint32_t));

  std::vector<uint64_t> field_sizes;
  for (uint64_t i = 0; i < f_header.num_fields - 1; ++i) {
    field_sizes.push_back(f_header.field_offsets[i+1] - f_header.field_offsets[i]);
  }

  field_sizes.push_back(size_to_read - (f_header.field_offsets[f_header.num_fields - 1] - id_to_offset[token_id]));

  std::vector<uint32_t> all_docids;
  for (uint64_t i = 0; i < f_header.field_ids.size(); ++i) {

    // Continue if requesting specific field_ids but not found in postings list
    if (!req_field_ids.empty()) {
      if (!std::binary_search(req_field_ids.begin(), req_field_ids.end(), f_header.field_ids[i]))
        continue;
    }

    addr_offset = f_header.field_offsets[i];

    std::vector<uint32_t> docids;
    switch (enc_type) {
      case EncType::GVB:
        docids = read_gvb_from_file<uint32_t>(file + addr_offset, field_sizes[i]);
        break;
      case EncType::PEF:
        docids = read_pef_from_file<uint32_t>(file + addr_offset, field_sizes[i]);
        break;
      case EncType::FLS:
        docids = read_fls_from_file(file + addr_offset, field_sizes[i]);
        break;
      case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
      case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM: 
        docids = read_bp128_from_file(file + addr_offset, field_sizes[i], enc_type);
        break;
      default:
        throw std::invalid_argument("Not a valid encoding type");
    }

    std::vector<uint32_t> merged;
    merged.reserve(docids.size() + all_docids.size());
    std::merge(docids.begin(), docids.end(), all_docids.begin(), all_docids.end(), std::back_inserter(merged));
    all_docids = merged;
  }

  std::vector<uint32_t> dedup = remove_dups_sorted(all_docids);

  return dedup;
}

const std::vector<uint32_t> read_entry_offsets_gvb(const uint8_t *file, const size_t size_to_read) {
  std::vector<uint32_t> decoded = read_gvb_from_file<uint32_t>(file, size_to_read);

  return decoded;
}

const std::vector<uint32_t> read_entry_offsets_pef(const uint8_t *file, const size_t size_to_read) {
  std::vector<uint32_t> decoded = read_pef_from_file<uint32_t>(file, size_to_read);

  return decoded;
}

const std::vector<uint32_t> read_entry_offsets_fls(const uint8_t *file, const size_t size_to_read) {
  std::vector<uint32_t> decoded = read_fls_from_file(file, size_to_read);

  return decoded;
}

const std::vector<uint32_t> read_entry_offsets_bp128(const uint8_t *file, const size_t size_to_read, uint8_t enc_type) {
  std::vector<uint32_t> decoded = read_bp128_from_file(file, size_to_read, enc_type);

  return decoded;
}

// Output converts offsets values from file from 1 indexing back to 0 indexing
const std::vector<uint32_t>
get_entry_offsets_from_file(const std::string &file_name,
                            const uint8_t enc_type) {
  const MappedFile mf = open_mmap_r(file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();

  // Read in header
  Offsets_header header = read_offsets_header(file);

  const size_t size_to_read =
      header.int_toks_header - header.entry_offsets_header;

  if (size_to_read == 0)
    throw std::invalid_argument("No entry offsets present in file");

  switch (enc_type) {
  case EncType::GVB:
    return read_entry_offsets_gvb(file + header.entry_offsets_header, size_to_read);
  case EncType::PEF:
    return read_entry_offsets_pef(file + header.entry_offsets_header, size_to_read);
  case EncType::FLS:
    return read_entry_offsets_fls(file + header.entry_offsets_header, size_to_read);
  case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
  case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM: 
    return read_entry_offsets_bp128(file + header.entry_offsets_header, size_to_read, enc_type);
  default:
    throw std::invalid_argument("Not a valid encoding type");
  }
}

std::pair<uint64_t, uint64_t>
get_pef_header_details_by_type(const uint8_t *file, const uint64_t file_size,
                               const uint8_t type) {
  // Read in header
  Offsets_header header = read_offsets_header(file);

  uint64_t offset = 0;
  uint64_t size_to_read = 0;
  switch (type) {
  case TokenType::PINT:
    offset = header.int_toks_header;
    size_to_read = header.neg_int_toks_header - header.int_toks_header;
    break;
  case TokenType::NINT:
    offset = header.neg_int_toks_header;
    size_to_read = header.date_toks_header - header.neg_int_toks_header;
    break;
  case TokenType::DATE:
    offset = header.date_toks_header;
    size_to_read = header.time_toks_header - header.date_toks_header;
    break;
  case TokenType::TIME:
    offset = header.time_toks_header;
    size_to_read = header.ipv4_toks_header - header.time_toks_header;
    break;
  case TokenType::IPV4:
    offset = header.ipv4_toks_header;
    size_to_read = get_alpha_fst_pos(file) - header.ipv4_toks_header;
    break;
  default:
    throw std::invalid_argument("Type not valid here");
  }

  return {offset, size_to_read};
}

void check_index_dir_valid(const std::string &index_path) {
  // Check metadata file exists
  if (!std::filesystem::exists(index_path + "/.metadata"))
    throw std::invalid_argument("File mappings file does not exist: " + index_path + "/.metadata");
    
    // Check checksum file exists
  if (!std::filesystem::exists(index_path + "/.checksum"))
    throw std::invalid_argument("Checksum file does not exist: " + index_path + "/.checksum");
}