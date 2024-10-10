#ifndef INFO_FUNCTIONS_H
#define INFO_FUNCTIONS_H

#include "DecodeFunctions.h"
#include <fstream>
#include <sstream>
#include <openssl/sha.h>

std::string read_torrent_file(const std::string& filename);
std::string bencode(const json& obj);
std::string sha1(const std::string& input);
void get_info(const std::string& filename, std::string& tracker_url, int64_t& file_length, std::string& info_hash, int64_t& piece_length, 
                      std::vector<std::string>& pieces_hashes);
void print_info(const std::string& tracker_url, const int64_t& file_length, const std::string& info_hash, const int64_t& piece_length, 
                const std::vector<std::string>& pieces_hashes);

#endif