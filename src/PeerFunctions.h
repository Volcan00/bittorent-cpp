#ifndef PEER_FUNCTIONS_H
#define PEER_FUNCTIONS_H

#include "DecodeFunctions.h"
#include <sstream>
#include <curl/curl.h>


std::string hex_to_binary(const std::string& hex);
std::string url_encode(const std::string& value);
static size_t write_callback(void* content, size_t size, size_t nmemb, std::string* response);
std::vector<std::string> get_peers(const std::string& tracker_url, const std::string info_hash, const std::string& peer_id, int port, int64_t uploaded, 
                              int64_t downloaded, int64_t left, bool compact);
void print_peers(const std::vector<std::string>& peers);
#endif