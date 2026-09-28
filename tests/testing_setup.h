
#ifndef TESTING_SETUP_H
#define TESTING_SETUP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <filesystem>
#include <fstream>
#include "../src/ingest.h"
#include "../src/helper.h"

static std::string index_path = "../../tests/test_data/all_indexes/";
static std::string data_path = "../../tests/test_data/all_input_data/data_1";

class index_setup : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        std::filesystem::create_directory(index_path);
        create_mode(index_path+"index_1_pef", data_path, {}, EncType::PEF);
        create_mode(index_path+"index_1_gvb", data_path, {}, EncType::GVB);
    }

    static void TearDownTestSuite() {
        std::filesystem::remove_all(index_path);
    }
};

#endif