#ifndef LOGFMT_PARSER_H
#define LOGFMT_PARSER_H

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

class LogfmtParser {
public:
    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = LogfmtParser;
        using difference_type = std::ptrdiff_t;
        using pointer = LogfmtParser*;
        using reference = LogfmtParser&;

        iterator() : parser_(nullptr) {}
        explicit iterator(LogfmtParser* p) : parser_(p) {
            init();
        }

        iterator& operator++() { advance(); return *this; }

        reference operator*() const { return *parser_; }
        pointer operator->() const { return parser_; }

        bool operator==(const iterator& other) const { return parser_ == other.parser_; }
        bool operator!=(const iterator& other) const { return !(*this == other); }

    private:
        void advance() {
            if (!parser_) {
                return;
            }

            if (!parser_->buffered_rows.empty()) {
                parser_->buffered_rows.pop_front();
                parser_->buffered_line_offsets.pop_front();
            }

            if (!parser_->has_next_row()) {
                parser_ = nullptr;
            }
        }

        void init() {
            if (!parser_) {
                return;
            }

            if (!parser_->has_next_row()) {
                parser_ = nullptr;
            }
        }

        LogfmtParser* parser_;
    };

    explicit LogfmtParser(const std::string &file_path);
    LogfmtParser() = default;
    iterator begin() { return iterator(this); }
    iterator end()   { return iterator(); }

    std::unordered_map<std::string, std::string> current_row();
    uint64_t current_offset();

private:
    std::deque<std::unordered_map<std::string, std::string>> buffered_rows;
    std::deque<uint64_t> buffered_line_offsets;
    uint8_t *file;
    size_t file_size;
    uint64_t file_offset;

    void buffer_lines(size_t n_lines);
    std::unordered_map<std::string, std::string> parse_line(const std::string &data);
    std::pair<std::string, uint64_t> read_next_line();
    bool has_next_row();
    
};

#endif