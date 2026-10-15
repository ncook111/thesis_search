#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/ingest.h"
#include "../src/helper.h"
#include "testing_setup.h"
#include <filesystem>

class create_mode_test : public index_setup {};
class recreate_mode_test : public index_setup {};

/* test parse_options */
TEST(parse_options_test, test_verbose) {
    const int argc = 15;
    const char *argv[15] = {
        "./indexer",
        "--mode",
        "create",
        "--encoding",
        "pef",
        "--index-path",
        "index1",
        "--data-path",
        "data1",
        "--query",
        "query1",
        "--fields",
        "field1",
        "--output-path",
        "output1"
    };

    po::options_description desc("Allowed options");
    po::variables_map vm;
    int ret = parse_options(desc, vm, argc, argv);

    ASSERT_EQ(ret, 0);

    std::string act_mode;
    
    ASSERT_NO_THROW(act_mode = vm["mode"].as<std::string>());
    ASSERT_EQ(act_mode, "create");

    uint8_t enc_type;

    ASSERT_NO_THROW(enc_type = str_to_enc_type(vm["encoding"].as<std::string>()));
    ASSERT_EQ(enc_type, EncType::PEF);

    std::string data_path;
    ASSERT_NO_THROW(data_path = vm["data-path"].as<std::string>());
    ASSERT_EQ(data_path, "data1");

    std::string index_path;
    ASSERT_NO_THROW(index_path = vm["index-path"].as<std::string>());
    ASSERT_EQ(index_path, "index1");

    std::string query;
    ASSERT_NO_THROW(query = vm["query"].as<std::string>());
    ASSERT_EQ(query, "query1");

    std::vector<std::string> fields;
    ASSERT_NO_THROW(fields = vm["fields"].as<std::vector<std::string>>());
    std::vector<std::string> expected = {"field1"};
    ASSERT_EQ(fields, expected);

    std::string output_path;
    ASSERT_NO_THROW(output_path = vm["output-path"].as<std::string>());
    ASSERT_EQ(output_path, "output1");
}

TEST(parse_options_test, test_concise) {
    const int argc = 15;
    const char *argv[15] = {
        "./indexer",
        "-m",
        "create",
        "-e",
        "pef",
        "-i",
        "index1",
        "-d",
        "data1",
        "-q",
        "query1",
        "-f",
        "field1",
        "-o",
        "output1"
    };

    po::options_description desc("Allowed options");
    po::variables_map vm;
    int ret = parse_options(desc, vm, argc, argv);

    ASSERT_EQ(ret, 0);

    std::string act_mode;
    
    ASSERT_NO_THROW(act_mode = vm["mode"].as<std::string>());
    ASSERT_EQ(act_mode, "create");

    uint8_t enc_type;

    ASSERT_NO_THROW(enc_type = str_to_enc_type(vm["encoding"].as<std::string>()));
    ASSERT_EQ(enc_type, EncType::PEF);

    std::string data_path;
    ASSERT_NO_THROW(data_path = vm["data-path"].as<std::string>());
    ASSERT_EQ(data_path, "data1");

    std::string index_path;
    ASSERT_NO_THROW(index_path = vm["index-path"].as<std::string>());
    ASSERT_EQ(index_path, "index1");

    std::string query;
    ASSERT_NO_THROW(query = vm["query"].as<std::string>());
    ASSERT_EQ(query, "query1");

    std::vector<std::string> fields;
    ASSERT_NO_THROW(fields = vm["fields"].as<std::vector<std::string>>());
    std::vector<std::string> expected = {"field1"};
    ASSERT_EQ(fields, expected);

    std::string output_path;
    ASSERT_NO_THROW(output_path = vm["output-path"].as<std::string>());
    ASSERT_EQ(output_path, "output1");
}

TEST(parse_options_test, test_reorder) {
    const int argc = 15;
    const char *argv[15] = {
        "./indexer",
        "-m",
        "create",
        "-d",
        "data1",
        "-q",
        "query1",
        "--index-path",
        "index1",
        "-f",
        "field1",
        "-o",
        "output1",
        "--encoding",
        "pef"
    };

    po::options_description desc("Allowed options");
    po::variables_map vm;
    int ret = parse_options(desc, vm, argc, argv);

    ASSERT_EQ(ret, 0);

    std::string act_mode;
    
    ASSERT_NO_THROW(act_mode = vm["mode"].as<std::string>());
    ASSERT_EQ(act_mode, "create");

    uint8_t enc_type;

    ASSERT_NO_THROW(enc_type = str_to_enc_type(vm["encoding"].as<std::string>()));
    ASSERT_EQ(enc_type, EncType::PEF);

    std::string data_path;
    ASSERT_NO_THROW(data_path = vm["data-path"].as<std::string>());
    ASSERT_EQ(data_path, "data1");

    std::string index_path;
    ASSERT_NO_THROW(index_path = vm["index-path"].as<std::string>());
    ASSERT_EQ(index_path, "index1");

    std::string query;
    ASSERT_NO_THROW(query = vm["query"].as<std::string>());
    ASSERT_EQ(query, "query1");

    std::vector<std::string> fields;
    ASSERT_NO_THROW(fields = vm["fields"].as<std::vector<std::string>>());
    std::vector<std::string> expected = {"field1"};
    ASSERT_EQ(fields, expected);

    std::string output_path;
    ASSERT_NO_THROW(output_path = vm["output-path"].as<std::string>());
    ASSERT_EQ(output_path, "output1");
}

TEST(parse_options_test, test_eq) {
    const int argc = 8;
    const char *argv[8] = {
        "./indexer",
        "--mode=create",
        "--data-path=data1",
        "--query=query1",
        "--index-path=index1",
        "--field=field1",
        "--output-path=output1",
        "--encoding=pef"
    };

    po::options_description desc("Allowed options");
    po::variables_map vm;
    int ret = parse_options(desc, vm, argc, argv);

    ASSERT_EQ(ret, 0);

    std::string act_mode;
    
    ASSERT_NO_THROW(act_mode = vm["mode"].as<std::string>());
    ASSERT_EQ(act_mode, "create");

    uint8_t enc_type;

    ASSERT_NO_THROW(enc_type = str_to_enc_type(vm["encoding"].as<std::string>()));
    ASSERT_EQ(enc_type, EncType::PEF);

    std::string data_path;
    ASSERT_NO_THROW(data_path = vm["data-path"].as<std::string>());
    ASSERT_EQ(data_path, "data1");

    std::string index_path;
    ASSERT_NO_THROW(index_path = vm["index-path"].as<std::string>());
    ASSERT_EQ(index_path, "index1");

    std::string query;
    ASSERT_NO_THROW(query = vm["query"].as<std::string>());
    ASSERT_EQ(query, "query1");

    std::vector<std::string> fields;
    ASSERT_NO_THROW(fields = vm["fields"].as<std::vector<std::string>>());
    std::vector<std::string> expected = {"field1"};
    ASSERT_EQ(fields, expected);

    std::string output_path;
    ASSERT_NO_THROW(output_path = vm["output-path"].as<std::string>());
    ASSERT_EQ(output_path, "output1");
}

TEST(parse_options_test, test_vector) {
    const int argc = 24;
    const char *argv[24] = {
        "./indexer",
        "-m",
        "create",
        "-d",
        "data1",
        "-q",
        "query1",
        "--index-path",
        "index1",
        "-f",
        "field1",
        "field2",
        "field3",
        "field4",
        "field5",
        "field6",
        "7",
        "8",
        "9",
        "10",
        "-o",
        "output1",
        "--encoding",
        "pef"
    };

    po::options_description desc("Allowed options");
    po::variables_map vm;
    int ret = parse_options(desc, vm, argc, argv);

    ASSERT_EQ(ret, 0);

    std::vector<std::string> fields;
    ASSERT_NO_THROW(fields = vm["fields"].as<std::vector<std::string>>());
    std::vector<std::string> expected = {"field1", "field2", "field3", "field4", "field5", "field6", "7", "8", "9", "10"};
    ASSERT_EQ(fields, expected);
}

/* test create_mode */
TEST_F(create_mode_test, test_empty_data_path) {
    ASSERT_EQ(create_mode("any", "", {}, EncType::PEF), 1);
}

TEST_F(create_mode_test, test_index_exists) {
    ASSERT_EQ(create_mode(index_path+"_pef", "", {}, EncType::PEF), 1);
}

TEST_F(create_mode_test, test_invalid_data_path) {
    ASSERT_EQ(create_mode("any", "../invalid", {}, EncType::PEF), 1);
}

TEST_F(create_mode_test, test_valid_instantiation) {
    ASSERT_EQ(create_mode(index_path+"_pef1", data_path, {}, EncType::PEF), 0);
    std::filesystem::remove_all(index_path+"_pef1");
}

/* test recreate_mode */
TEST_F(recreate_mode_test, test_empty_data_path) {
    ASSERT_EQ(recreate_mode(index_path+"_pef", "", {}, EncType::PEF), 1);
}

TEST_F(recreate_mode_test, test_index_exists_valid) {
    create_mode(index_path+"_pef", data_path, {}, EncType::PEF);
    ASSERT_EQ(recreate_mode(index_path+"_pef", data_path, {}, EncType::PEF), 0);
}

TEST_F(recreate_mode_test, test_invalid_index) {
    ASSERT_EQ(recreate_mode("invalid", data_path, {}, EncType::PEF), 1);
}

TEST_F(recreate_mode_test, test_invalid_data_path) {
    ASSERT_EQ(recreate_mode(index_path+"_pef", "../invalid", {}, EncType::PEF), 1);
}