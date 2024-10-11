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
bool send_message(int client_socket, const std::string& message);
bool receive_handshake_response(int client_socket, char* response_buffer, size_t buffer_size);
void complete_handshake(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id, int piece_index, int piece_length);

#endif