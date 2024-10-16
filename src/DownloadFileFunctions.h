#ifndef DOWNLOAD_FILE_FUNCTIONS_H
#define DOWNLOAD_FILE_FUNCTIONS_H

#include "DownloadPieceFunctions.h"


bool verify_piece_hash(const char* piece_data, size_t piece_length, const std::string& expected_hash);
bool download_entire_file(int client_socket, const std::vector<std::string>& pieces_hashes, int piece_length, int file_length, const std::string& download_filename);
void complete_file_download(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id, const std::vector<std::string>& pieces_hashes, int piece_length, int file_length, const std::string& download_filename, bool download = true);

#endif