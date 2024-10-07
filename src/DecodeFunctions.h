#ifndef DECODE_FUNCTIONS_H
#define DECODE_FUNCTIONS_H

#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

std::string read_torrent_file(const std::string& filename);
json decode_bencoded_string(const std::string& encoded_value, int& start_position);
json decode_encoded_integer(const std::string& encoded_value, int& start_position);
json decode_bencoded_list(const std::string& encoded_value, int& start_position);
json decode_bencoded_dictionary(const std::string& encoded_value, int& start_position);
json decode_bencoded_value(const std::string& encoded_value, int& start_position);

#endif