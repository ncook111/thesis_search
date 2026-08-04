
#ifndef INDEXER_H
#define INDEXER_H

#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, const char *argv[]);
int parse_options(po::options_description &desc, po::variables_map &vm, const int argc, const char *argv[]);

#endif