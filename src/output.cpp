#include "output.h"
#include "search.h"
#include "fst.h"

void output_entries(const FieldMap &matches, const std::string &index_path,
                   const std::string &data_path, const std::string &output_path, const uint8_t enc_type) {
  std::unique_ptr<StdCompactFst> fst = read_from_disk(index_path + "/file_mappings.fst");
  const std::vector<std::string> names = get_vec_strings_from_fst(*fst);
  const EntryMap result = get_entries_offsets(matches, names, data_path, index_path, enc_type);
  for (const auto &[file_id, entries] : result) {
    const std::string file_name = id_to_string(*fst, file_id);
    const std::string file_path = data_path + '/' + file_name;

    const MappedFile mf = open_mmap_r(file_path);

    const char *data_file = static_cast<char *>(mf.addr());
    if (output_path.size()) {
      entries_to_file(output_path, file_name, entries, data_file);
    } else {
      entries_to_cout(file_name, entries, data_file);
    }

  }
}

void output_counts(const FieldMap &matches, const std::string &index_path, const std::string &output_path) {
  std::unique_ptr<StdCompactFst> fst = read_from_disk(index_path + "/file_mappings.fst");
  
  for (const auto &[file_id, docids] : matches) {
    const std::string file_name = id_to_string(*fst, file_id);
    
    if (output_path.size()) {
      counts_to_file(output_path, file_name, docids.size());
    } else {
      counts_to_cout(file_name, docids.size());
    }
  }
}


void entries_to_cout(const std::string &file_name, const std::vector<uint64_t> &entries, const char *data_file) {
  std::cout << "File: " << file_name << '\n';
  if (!entries.empty()) {
    for (auto it = entries.begin(); it < entries.end() - 1; it += 2) {
      uint64_t size = *(it + 1) - *it;
      std::cout.write(&data_file[*it], size);
      std::cout << '\n';
    }
  }
}

void entries_to_file(const std::string &output_path, const std::string &file_name, const std::vector<uint64_t> &entries, const char *data_file) {
  std::ofstream output_file;
  output_file.open(output_path);

  output_file << "File: " << file_name << '\n';
  if (!entries.empty()) {
    for (auto it = entries.begin(); it < entries.end() - 1; it += 2) {
      uint64_t size = *(it + 1) - *it;
      output_file.write(&data_file[*it], size);
      output_file << '\n';
    }
  }

  output_file << '\n';

  output_file.close();
}

void counts_to_cout(const std::string &file_name, const size_t count) {
  std::cout << "File: " << file_name << '\n'
            << "  Matching Documents: " << count << '\n';
}

void counts_to_file(const std::string &output_path, const std::string &file_name, const size_t count) {
  std::ofstream output_file;
  output_file.open(output_path);

  output_file << "File: " << file_name << '\n'
           << "Matching Documents: " << count << '\n';
  
  output_file.close();
}