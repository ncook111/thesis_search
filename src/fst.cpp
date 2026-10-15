#include "fst.h"
#include "segment.h"
#include <fst/fstlib.h>

StdCompactFst read_from_disk(const std::string &file_name, const size_t read_pos) {
  std::ifstream in(file_name, std::ios::binary);
  in.seekg(read_pos);
  fst::FstReadOptions opts(file_name);
  opts.mode = fst::FstReadOptions::MAP;

  StdCompactFst *fst = StdCompactFst::Read(in, opts);

  if (!fst)
    throw std::runtime_error("Failed to read FST: " + file_name);

  in.close();

  return *fst;
}

size_t write_to_disk(const StdCompactFst &fst, std::ofstream &file) {
  file.seekp(0, std::ios::end);
  size_t write_pos = file.tellp();
  fst::FstWriteOptions opts;
  opts.align = true;
  fst.Write(file, opts);


  return write_pos;
}

size_t get_fst_pos(const std::string &file_name, const uint8_t fst_type) {
  switch (fst_type) {
    case FstType::ALPHA:
      return get_alpha_fst_pos(file_name);
    case FstType::FLOAT:
      return get_float_fst_pos(file_name);
    case FstType::FIELDS:
      return get_fields_fst_pos(file_name);
    case FstType::OTHER:
      return 0;
    default:
      throw std::runtime_error("Fst_type not valid: " + std::to_string(fst_type));
  }
}

