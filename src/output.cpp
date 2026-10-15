#include "output.h"
#include "search.h"
#include "fst.h"
#include "metadata.h"

void output_entries(const FieldMap &matches, const Metadata &metadata,
                   const std::string &data_path, const std::string &output_path) {
  const EntryMap result = get_entries_offsets(matches, data_path, metadata);
  std::ofstream output_file;

  if (output_path.size())
    output_file.open(output_path);

  for (const auto &[file_id, entries] : result) {
    const std::string file_name = metadata.get_file_name(file_id);
    const std::string file_path = data_path + '/' + file_name;

    const MappedFile mf = open_mmap_r(file_path);

    const char *data_file = static_cast<char *>(mf.addr());

    if (output_path.size()) {
      entries_to_file(output_file, file_name, entries, data_file);
    } else {
      entries_to_cout(file_name, entries, data_file);
    }

  }
}

void output_counts(const FieldMap &matches, const Metadata &metadata, const std::string &output_path) {
  std::ofstream output_file;
  
  if (output_path.size())
    output_file.open(output_path);

  for (const auto &[file_id, docids] : matches) {
    const std::string file_name = metadata.get_file_name(file_id);
    if (output_path.size()) {
      counts_to_file(output_file, file_name, docids.size());
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

void entries_to_file(std::ofstream &output_file, const std::string &file_name, const std::vector<uint64_t> &entries, const char *data_file) {
  output_file << "File: " << file_name << '\n';
  if (!entries.empty()) {
    for (auto it = entries.begin(); it < entries.end() - 1; it += 2) {
      uint64_t size = *(it + 1) - *it;
      output_file.write(&data_file[*it], size);
      output_file << '\n';
    }
  }

  output_file << '\n';
}

void counts_to_cout(const std::string &file_name, const size_t count) {
  std::cout << "File: " << file_name << '\n'
            << "  Matching Documents: " << count << '\n';
}

void counts_to_file(std::ofstream &output_file, const std::string &file_name, const size_t count) {
  output_file << "File: " << file_name << '\n'
           << "Matching Documents: " << count << '\n';
}