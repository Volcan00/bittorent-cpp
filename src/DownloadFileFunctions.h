#ifndef DOWNLOAD_FILE_FUNCTIONS_H
#define DOWNLOAD_FILE_FUNCTIONS_H

#include "DownloadPieceFunctions.h"

void complete_file_download(const std::string& peer_ip, int port, const std::string& info_hash, const std::string& peer_id, const std::vector<std::string>& piece_hashes, int piece_length, int file_length, const std::string& download_filename);

#endif