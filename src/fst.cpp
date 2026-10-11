#include "fst.h"
#include <fst/fstlib.h>

std::unique_ptr<StdCompactFst> read_from_disk(const std::string &file_name, const size_t read_pos) {
  std::ifstream in(file_name, std::ios::binary);
  in.seekg(read_pos);
  fst::FstReadOptions opts(file_name);
  opts.mode = fst::FstReadOptions::MAP;

  StdCompactFst *fst = StdCompactFst::Read(in, opts);

  if (!fst)
    throw std::runtime_error("Failed to read FST");

  in.close();

  return std::unique_ptr<StdCompactFst>(fst);
}

size_t write_to_disk(std::unique_ptr<StdCompactFst> fst, std::ofstream &file) {
  file.seekp(0, std::ios::end);
  size_t write_pos = file.tellp();
  fst::FstWriteOptions opts;
  opts.align = true;
  (*fst).Write(file, opts);


  return write_pos;
}

Fst_header get_header_from_fst(const std::string &file_name) {
  // Read in header
  std::ifstream in(file_name, std::ios::binary);
  Fst_header fst_header = {};
  in.read(reinterpret_cast<char*>(&fst_header), sizeof(fst_header));
  in.close();

  return fst_header;
}