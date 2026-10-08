#include "fst.h"
#include <fst/fstlib.h>

std::unique_ptr<StdCompactFst> read_from_disk(const std::string &file_name) {
  std::ifstream in(file_name, std::ios::binary);
  fst::FstReadOptions opts(file_name);
  opts.mode = fst::FstReadOptions::MAP;

  StdCompactFst *fst = StdCompactFst::Read(in, opts);

  if (!fst)
    throw std::runtime_error("Failed to read FST");

  return std::unique_ptr<StdCompactFst>(fst);
}