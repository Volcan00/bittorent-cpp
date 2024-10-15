#ifndef HANDSHAKE_FUNCTIONS_H
#define HANDSHAKE_FUNCTIONS_H

#include "InfoFunctions.h"
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

void split_ip_and_port(const std::string& peer, std::string& ip, int& port);
ssize_t recv_all(int socket, char* buffer, size_t len);
int create_socket();
bool setup_server_address(const std::string& ip, int port, sockaddr_in& server_address);
bool connect_to_server(int client_socket, const sockaddr_in& server_address);
std::string prepare_handshake_message(const std::string& info_hash, const std::string& peer_id);
bool send_handshake_message(int client_socket, const std::string& message);
bool receive_handshake_response(int client_socket, char* response_buffer, size_t buffer_size, bool download);
bool perform_handshake(int client_socket, const std::string& info_hash, const std::string& peer_id);
bool handle_bitfield_message(int client_socket);
bool send_interested_message(int client_socket);
bool wait_for_unchoke(int client_socket);
bool send_request_message(int client_socket, int piece_index, int block_offset, int block_length);
bool receive_piece_block(int client_socket, char* piece_buffer, int piece_index, int block_offset, int block_length);
bool download_piece(int client_socket, int piece_index, int piece_length, const std::string& download_filename);
void complete_download(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id, int piece_index, int piece_length, 
                       const std::string& download_filename = "", bool download = false);



#endif