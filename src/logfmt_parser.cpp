#include "logfmt_parser.h"
#include "helper.h"
#include <unordered_map>
#include <iostream>

#define LINES_TO_BUFFER 100

LogfmtParser::LogfmtParser(const std::string &file_path)
    : mf(open_mmap_r(file_path)),
      file_size(mf.size()),
      file_offset(0) {
    buffer_lines(LINES_TO_BUFFER);
}

LogfmtParser::LogfmtParser(const std::string &file_path, const uint64_t start_addr, const uint64_t end_addr)
    : mf(open_mmap_r(file_path)),
      file_size(std::min<uint64_t>(end_addr, mf.size())),
      file_offset(start_addr) {
    buffer_lines(LINES_TO_BUFFER);
}

std::unordered_map<std::string, std::string> LogfmtParser::current_row() {
    if (buffered_rows.size())
        return buffered_rows.front();
    else
        return std::unordered_map<std::string, std::string>();
}

int64_t LogfmtParser::current_offset() {
    if (buffered_line_offsets.size())
        return buffered_line_offsets.front();
    else
        return -1; // TODO: Check this
}

std::pair<std::string, uint64_t> LogfmtParser::read_next_line() {
    size_t line_start = file_offset;
	uint8_t* file = static_cast<uint8_t *>(mf.addr());
    std::string line;

    bool in_quotes = false;
    bool is_escaped = false;

    for (size_t i = file_offset; i < file_size; ++i) {
        switch (file[i]) {
            case '\n':
                if (!in_quotes && !is_escaped) {
                    size_t size = i - file_offset;
                    line.assign(reinterpret_cast<const char*>(&file[line_start]), size);
                    file_offset = i + 1;
                    
                    return std::make_pair(line, line_start);
                }

                is_escaped = false;
                break;
            case '"':
                if (!is_escaped) in_quotes = !in_quotes;

                is_escaped = false;
                break;
            case '\\':
                is_escaped = !is_escaped;
                break;
            default:
                is_escaped = false;
                break;
        }
    }

    // Handle final line without a nl
    if (!line.size()) {
        size_t size = file_size - file_offset;
        line.assign(reinterpret_cast<const char*>(&file[line_start]), size);
        file_offset = file_size;
    }

    return std::make_pair(line, line_start); 
}

void LogfmtParser::buffer_lines(const size_t n_lines) {
    for (size_t i = 0; i < n_lines; ++i) {
        if (file_offset >= file_size) break; 

        auto [line, line_start] = read_next_line();
		if (is_blank(line)) continue;
        buffered_rows.push_back(parse_line(line));
        buffered_line_offsets.push_back(line_start);
    }
}

bool LogfmtParser::has_next_row() {
    if (buffered_rows.size() && buffered_line_offsets.size()) { // Rows already buffered
        return true;
    } 
        
    if (file_offset < file_size) { // More to read from file
            buffer_lines(LINES_TO_BUFFER);
            return true;
    }

    // No rows buffered and nothing left to read from file
    return false;
}

/*
    https://github.com/grahamking/logfmtcpp/tree/master
    Source code taken from this repo and modified
*/
std::unordered_map<std::string, std::string> LogfmtParser::parse_line(const std::string &data) {
    std::unordered_map<std::string, std::string> m;

	uint32_t key_start = 0;
	uint32_t key_end = 0;
	uint32_t val_start = 0;
	uint32_t val_end = 0;
	bool is_key = true;
	bool in_quotes = false;
	bool in_key = false;
	bool in_value = false;
	bool is_escape = false;
	char c = 0;
	for (uint32_t i = 0; i < data.size(); ++i) {
		c = data[i];

		if (c == '\\' && !is_escape) {
			is_escape = true;
			continue;
		}

		if (is_key && c > ' ' && c != '"' && c != '=') {
			if (!in_key) {
				key_start = i;
				in_key=true;
			}
		} else if (c == '"' && !is_escape) {
			if (in_quotes) {
				val_end = i;
			}
			in_quotes = !in_quotes;
		} else if (c == '=' && !in_quotes) {
			is_key = false;
			key_end = i;
		} else if (c == ' ' && !in_quotes) {
			if (val_end < val_start) {
				val_end = i;
			}
            
            if (key_end == 0) {
				key_end = key_start+1;
			}
			
            if (val_start == 0) {
				// empty value
				m.emplace(data.substr(key_start, key_end-key_start), "true");
			} else {
				// normal case
				m.emplace(
						data.substr(key_start, key_end-key_start),
						data.substr(val_start, val_end-val_start)
				);
			}
			is_key = true;
			in_key = false;
			in_value = false;
			key_start = key_end = val_start = val_end = 0;
		}
		else {
			if (!in_value) {
				val_start = i;
				in_value = true;
			}
		}
		is_escape = false;
	}

	// add last value
	if (key_end == 0) {
		key_end = data.size();
	}
	
    if (val_start == 0) {
		// empty value
		m.emplace(data.substr(key_start, key_end-key_start), "true");
	} else {
		// normal case
		m.emplace(
				data.substr(key_start, key_end-key_start),
				data.substr(val_start, val_end-val_start)
		);
	}

    return m;
}