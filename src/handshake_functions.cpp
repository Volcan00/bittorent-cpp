#include "InfoFunctions.h"
#include "PeerFunctions.h"
#include "HandshakeFunctions.h"

//Helper function to split peer's IP and port
void split_ip_and_port(const std::string& peer, std::string& ip, int& port) {
    size_t colon_index = peer.find(':');

    if(colon_index == std::string::npos) {
        std::cerr << "Invalid peer address format." << std::endl;
        return;
    }

    ip = peer.substr(0, colon_index);
    port = std::stoi(peer.substr(colon_index + 1));
}

// Helper function to receive exactly 'len' bytes from the socket
ssize_t recv_all(int socket, char* buffer, size_t length) {
    ssize_t total_received = 0;
    while (total_received < length) {
        ssize_t bytes_received = recv(socket, buffer + total_received, length - total_received, 0);
        if (bytes_received < 0) {
            perror("recv");
            std::cerr << "Failed after receiving " << total_received << " bytes." << std::endl;
            return -1; // Error
        } else if (bytes_received == 0) {
            std::cerr << "Connection closed after receiving " << total_received << " bytes." << std::endl;
            break; // Connection closed
        }
        total_received += bytes_received;
    }
    return total_received; // Return the total bytes received
}


// Helper function to create a socket
int create_socket() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Failed to create a socket." << std::endl;
    }
    return client_socket;
}

// Helper function to set up the server address
bool setup_server_address(const std::string& ip, int port, sockaddr_in& server_address) {
    memset(&server_address, 0, sizeof(server_address)); // Clear the structure
    server_address.sin_family = AF_INET;                // IPv4
    server_address.sin_port = htons(port);              // Convert to network byte order
    if (inet_pton(AF_INET, ip.c_str(), &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid IP address." << std::endl;
        return false;
    }
    return true;
}

// Helper function to connect to the server
bool connect_to_server(int client_socket, const sockaddr_in& server_address) {
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        std::cerr << "Failed to connect to server." << std::endl;
        return false;
    }
    return true;
}

// Helper function to establish a connection to a server
int establish_connection(const std::string& ip, int port) {
    // Step 1: Create a socket
    int client_socket = create_socket();
    if (client_socket == -1) return -1;

    // Step 2: Set up server address
    sockaddr_in server_address;
    if (!setup_server_address(ip, port, server_address)) {
        close(client_socket);
        return -1;
    }

    // Step 3: Connect to the server
    if (!connect_to_server(client_socket, server_address)) {
        close(client_socket);
        return -1;
    }

    // Return the connected socket
    return client_socket;
}

// Helper function to prepare the handshake message
std::string prepare_handshake_message(const std::string& info_hash, const std::string& peer_id) {
    return "\x13" + std::string("BitTorrent protocol") + std::string(8, '\0') + hex_to_binary(info_hash) + peer_id;
}

// Helper function to send a message
bool send_handshake_message(int client_socket, const std::string &message) {
    size_t total_sent = 0;
    size_t message_length = message.size();

    while (total_sent < message_length) {
        ssize_t bytes_sent = send(client_socket, message.data() + total_sent, message_length - total_sent, 0);
        if (bytes_sent < 0) {
            std::cerr << "Error sending message: " << strerror(errno) << std::endl;
            return false;
        }
        total_sent += bytes_sent;
    }
    return true;
}

// Helper function to receive and process the handshake response
bool receive_handshake_response(int client_socket, char* response_buffer, size_t buffer_size, bool download) {
    ssize_t bytes_received = recv_all(client_socket, response_buffer, buffer_size);
    if (bytes_received == 68) {
        int length = static_cast<unsigned char>(response_buffer[0]);
        std::string received_peer_id(response_buffer + 1 + length + 8 + 20, 20);

        // Convert peer ID to hexadecimal
        std::ostringstream oss;
        for (unsigned char c : received_peer_id) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }

        if(!download)
            std::cout << "Peer ID: " << oss.str() << std::endl;

        return true;
    } else if (bytes_received == 0) {
        std::cerr << "Connection closed by the server." << std::endl;
        return false;
    } else {
        perror("Failed to receive full handshake response");
        return false;
    }
}

// Function to perform the handshake with the peer
bool perform_handshake(int client_socket, const std::string& info_hash, const std::string& peer_id, bool download) {
    // Step 4: Prepare the handshake message
    std::string handshake_message = prepare_handshake_message(info_hash, peer_id);

    // Step 5: Send the handshake message
    if (!send_handshake_message(client_socket, handshake_message)) {
        return false; // Failure to send handshake message
    }

    // Step 6: Receive the handshake response
    char response_buffer[68] = {0}; // Buffer for the response
    if (!receive_handshake_response(client_socket, response_buffer, sizeof(response_buffer), download)) {
        return false; // Failure to receive handshake response
    }

    return true; // Handshake successful
}

// The main complete_handshake function
void complete_handshake(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id, bool download) {
    // Establish the connection
    int client_socket = establish_connection(ip, port);
    if (client_socket == -1) return;  // Connection failed

    // Now perform the handshake
    if (!perform_handshake(client_socket, info_hash, peer_id, download)) {
        close(client_socket);
        return;
    }  

    // Cleanup
    close(client_socket);
}