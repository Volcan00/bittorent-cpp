#include "PeerFunctions.h"
#include "HandshakeFunctions.h"

// Helper function to receive exactly 'len' bytes from the socket
ssize_t recv_all(int socket, char* buffer, size_t len) {
    size_t total_bytes_received = 0;
    while (total_bytes_received < len) {
        ssize_t bytes_received = recv(socket, buffer + total_bytes_received, len - total_bytes_received, 0);
        if (bytes_received <= 0) {
            return bytes_received; // Return on error or connection close
        }
        total_bytes_received += bytes_received;
    }
    return total_bytes_received;
}

void complete_handshake(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id) {
    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket == -1) {
        std::cerr << "Failed to create a socket." << std::endl;
        return;
    }

    // Define the server address
    sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address)); // Clear the structure
    server_address.sin_family = AF_INET;                // IPv4
    server_address.sin_port = htons(port);              // Convert to network byte order
    if(inet_pton(AF_INET, ip.c_str(), &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid IP address." << std::endl;
        close(client_socket);
        return;
    }

    // Connect to the server
    if(connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
        std::cerr << "Connection to " << ip << ":" << port << " failed." << std::endl;
        close(client_socket);
        return;
    }

    // Prepare the handshake message
    // Here we assume a BitTorrent-like handshake message structure for the example
    // Prepare the handshake message
    std::string handshake_message = "\x13" + std::string("BitTorrent protocol") + std::string(8, '\0') + hex_to_binary(info_hash) + peer_id;

    // Send the handshake message
    ssize_t bytes_sent = send(client_socket, handshake_message.c_str(), handshake_message.size(), 0);
    if(bytes_sent == -1) {
        std::cerr << "Failed to send handshake message." << std::endl;
        close(client_socket);
        return;
    }

    // Receive the response from the server
    char response_buffer[68] = {0}; // Buffer for response (1 + 19 + 8 + 20 + 20 = 68 bytes)
    ssize_t bytes_received = recv_all(client_socket, response_buffer, sizeof(response_buffer));
    if (bytes_received == 68) {
        // Extract the peer ID from the response
        int length = static_cast<unsigned char>(response_buffer[0]);
        std::string received_peer_id(response_buffer + 1 + length + 8 + 20, 20);

        // Print Peer ID as hexadecimal
        std::ostringstream oss;
        for (unsigned char c : received_peer_id) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
        }

        std::cout << "Peer ID: " << oss.str() << std::endl;

    } else if (bytes_received == 0) {
        std::cerr << "Connection closed by the server." << std::endl;
    } else {
        perror("Failed to receive full handshake response");
    }

    close(client_socket);
}