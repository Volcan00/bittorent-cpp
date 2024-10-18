#ifndef DOWNLOAD_PIECE_FUNCTIONS_H
#define DOWNLOAD_PIECE_FUNCTIONS_H

#include "HandshakeFunctions.h"

bool handle_bitfield_message(int client_socket);
bool send_interested_message(int client_socket);
bool wait_for_unchoke(int client_socket);
bool handle_preparational_messages(int client_socket);
bool send_request_message(int client_socket, int piece_index, int block_offset, int block_length);
bool receive_piece_block(int client_socket, char* piece_buffer, int piece_index, int block_offset, int block_length);
bool download_piece(int client_socket, int piece_index, int piece_length, const std::string& expected_hash, const std::string& download_filename);
void complete_piece_download(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id, int piece_index, int piece_length, const std::string& expeceted_hash, const std::string& download_filename, bool download = true);

#endif