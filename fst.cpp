#include <fst/fstlib.h>
#include "fst.h"

void read_from_disk(fst::StdConstFst *&fst, const std::string &file_name) {
    fst = fst::StdConstFst::Read(file_name);

    if (!fst) throw std::runtime_error("Failed to read FST");
}