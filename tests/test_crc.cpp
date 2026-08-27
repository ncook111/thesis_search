#include "../src/ingest.h"
#include "../src/crc.h"
#include "../src/helper.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>

static std::string index_path = "../../tests/test_data/all_indexes/index_1_crc";
static std::string data_path = "../../tests/test_data/all_input_data/data_1";

class crc_test : public ::testing::Test {
protected:
    void TearDown() {
        std::filesystem::remove_all(index_path);
    }
};

TEST_F(crc_test, simple_index_checksum_valid) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path, data_path, {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    ASSERT_TRUE(check_checksum_file(index_path + "/checksum.crc", index_path));
}

TEST_F(crc_test, simple_index_checksum_invalid) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path, data_path, {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    // Modify an index file
    std::ofstream file;
    file.open(index_path + "/offsets/0.off", std::ios::binary);
    uint32_t zero = 0;
    file.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
    file.close();

    ASSERT_FALSE(check_checksum_file(index_path + "/checksum.crc", index_path));
}