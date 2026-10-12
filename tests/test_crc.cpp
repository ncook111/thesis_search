#include "../src/ingest.h"
#include "../src/crc.h"
#include "../src/helper.h"
#include "fuzzing.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>

static std::string index_path = "../../tests/test_data/all_indexes";
static std::string data_path = "../../tests/test_data/all_input_data/data_1";

class crc_test : public ::testing::Test {
protected:
    void SetUp() {
        std::filesystem::create_directory(index_path);
    }

    void TearDown() {
        std::filesystem::remove_all(index_path);
    }
};

class crc_test_fuzzed : public ::testing::Test {
protected:
    void SetUp() {
        std::filesystem::create_directory(index_path);
        generateFuzzedIndexFolder(index_path + "/index_fuzzed");
    }
    
    void TearDown() {
        std::filesystem::remove_all(index_path);
    }
};

TEST_F(crc_test, simple_index_checksum_valid) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path + "/index_1_crc", data_path, {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    ASSERT_TRUE(check_checksum_file(index_path + "/index_1_crc"));
}

TEST_F(crc_test, simple_index_checksum_invalid) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path + "/index_1_crc", data_path, {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    // Modify an index file
    std::ofstream file;
    file.open(index_path + "/index_1_crc/segments/segment_0.seg", std::ios::binary);
    uint32_t zero = 0;
    file.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
    file.close();

    ASSERT_FALSE(check_checksum_file(index_path + "/index_1_crc"));
}

TEST_F(crc_test_fuzzed, checksum_valid) {
    create_checksum_file(index_path + "/index_fuzzed");
    
    ASSERT_TRUE(check_checksum_file(index_path + "/index_fuzzed"));
}

TEST_F(crc_test_fuzzed, checksum_invalid) {
    create_checksum_file(index_path + "/index_fuzzed");

    ASSERT_TRUE(check_checksum_file(index_path + "/index_fuzzed"));
    
    // Flip a random bit in a random file, then verify the checksum doesn't match the original
    std::mt19937_64 eng(42);
    std::uniform_int_distribution<uint32_t> distr_64;

    for (size_t i = 0; i < 100; i++) {
        // Get list of supported files to index
        std::vector<std::filesystem::path> files;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(index_path + "/index_fuzzed")) {
            if (entry.is_regular_file() && entry.path().extension() != ".crc") // ignore checksum file
                files.push_back(entry.path());
        }

        // Pick a file at random and change a bit within the file at random
        std::filesystem::path chosen_file = files[distr_64(eng) % files.size()];
        size_t file_size = std::filesystem::file_size(chosen_file);
        std::fstream file;
        file.open(chosen_file, std::ios::binary | std::ios::in | std::ios::out);

        // Get random offset
        size_t r_offset = distr_64(eng) % file_size;

        // Read random byte
        uint8_t byte_from_file;
        uint8_t flipped_byte;
        file.seekg(r_offset);
        file.read(reinterpret_cast<char *>(&byte_from_file), 1);
        
        // Flip a random bit
        flipped_byte = byte_from_file ^ (1 << (distr_64(eng) % 8));

        // Write back to file
        file.seekp(r_offset);
        file.write(reinterpret_cast<const char *>(&flipped_byte), 1);
        file.flush();

        // Verify checksum is invalid
        ASSERT_FALSE(check_checksum_file(index_path + "/index_fuzzed"));

        // Write original byte back to file
        file.seekp(r_offset);
        file.write(reinterpret_cast<const char *>(&byte_from_file), 1);
        file.flush();

        // Verify checksum is now valid
        ASSERT_TRUE(check_checksum_file(index_path + "/index_fuzzed"));

        file.close();
    }
}