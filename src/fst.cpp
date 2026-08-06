#include "fst.h"
#include <fst/fstlib.h>

std::unique_ptr<fst::StdConstFst> read_from_disk(const std::string &file_name) {
  fst::StdConstFst *fst = fst::StdConstFst::Read(file_name);

  if (!fst)
    throw std::runtime_error("Failed to read FST");

  return std::unique_ptr<fst::StdConstFst>(fst);
}