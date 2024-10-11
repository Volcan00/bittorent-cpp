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

// Helper function to prepare the handshake message
std::string prepare_handshake_message(const std::string& info_hash, const std::string& peer_id) {
    return "\x13" + std::string("BitTorrent protocol") + std::string(8, '\0') + hex_to_binary(info_hash) + peer_id;
}

// Helper function to send a message
bool send_message(int client_socket, const std::string &message) {
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

// Helper function to receive a message of a given length
std::string receive_message(int client_socket, size_t length) {
    std::string message;
    message.resize(length);
    size_t total_received = 0;

    while (total_received < length) {
        ssize_t bytes_received = recv(client_socket, &message[total_received], length - total_received, 0);
        if (bytes_received < 0) {
            std::cerr << "Failed to receive message. Error: " << strerror(errno) << std::endl;
            return "";
        }
        if (bytes_received == 0) {
            std::cerr << "Connection closed by peer." << std::endl;
            return "";
        }

        // Log the received bytes
        std::cerr << "Received " << bytes_received << " bytes: ";
        for (size_t i = 0; i < bytes_received; ++i) {
            std::cerr << std::hex << static_cast<int>(static_cast<unsigned char>(message[total_received + i])) << " ";
        }
        std::cerr << std::dec << std::endl;

        total_received += bytes_received;
    }

    return message;
}

// Helper function to receive and process the handshake response
bool receive_handshake_response(int client_socket, char* response_buffer, size_t buffer_size) {
    ssize_t bytes_received = recv_all(client_socket, response_buffer, buffer_size);
    if (bytes_received == 68) {
        int length = static_cast<unsigned char>(response_buffer[0]);
        std::string received_peer_id(response_buffer + 1 + length + 8 + 20, 20);

        // Convert peer ID to hexadecimal
        std::ostringstream oss;
        for (unsigned char c : received_peer_id) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
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

// Function to download a piece from the peer
bool download_piece(int client_socket, int piece_index, int piece_length, const std::string& info_hash) {
    const int block_size = 16 * 1024;
    int num_blocks = piece_length / block_size + (piece_length % block_size != 0);

    //Step 1: Wait for a bitfield message (id: 5)
    std::string bitfield_header = receive_message(client_socket, 5);
    if(bitfield_header.empty() || bitfield_header[4] != 5) {
        std::cerr << "Failed to receive bitfield message." << std::endl;
        return false;
    }

    // Extract the length of the bitfield payload (first 4 bytes)
    int payload_length = bitfield_header[3];

    // Now receive the bitfield payload
    std::string bitfield_payload = receive_message(client_socket, payload_length); // Receiving the actual bitfield data
    if (bitfield_payload.empty()) {
        std::cerr << "Failed to receive bitfield payload." << std::endl;
        return false;
    }

    //Step 2: Send interested message (id: 2)
    std::string interested_message = "\x00\x00\x00\x01\x02"; // Message length (4 bytes) + message id (1 byte)
    if(!send_message(client_socket, interested_message)) {
        std::cerr << "Failed to send interested message." << std::endl;
        return false;
    }

    //Step 3: Wait for unchoke message (id: 1)
    std::string unchoke_message = receive_message(client_socket, 5);
    if(unchoke_message.empty() || unchoke_message[4] != 1) {
        std::cerr << "Failed to receive unchoke message." << std::endl;
        return false;
    }

    std::vector<char> piece_data(piece_length); // Buffer to store the piece

    //Step 4: Request each block
    for(int block_index = 0; block_index < num_blocks; ++ block_index) {
        int block_offset = block_index * block_size;
        int block_length = (block_index == num_blocks - 1) ? (piece_length % block_size) : block_size;

        //Prepare the request message (id: 6)
        std::string request_message(17, 0);
        int message_length = htonl(13); // 13 bytes for the payload
        int index_n = htonl(piece_index);
        int offset_n = htonl(block_offset);
        int length_n = htonl(block_length);

        memcpy(&request_message[0], &message_length, 4); // Message length (4 bytes)
        request_message[4] = 6;                          // Message if (1 byte)
        memcpy(&request_message[5], &index_n, 4);        // Piece index (4 bytes)
        memcpy(&request_message[9], &offset_n, 4);       // Block offset (4 bytes)
        memcpy(&request_message[13], &length_n, 4);      // Block length (4 bytes)

        //Send the request
        if(!send_message(client_socket, request_message)) {
            std::cerr << "Failed to send request message." << std::endl;
        }

        //Step 5: Wait for piece message (id: 7)
        std::string piece_message = receive_message(client_socket, block_length + 13); // 13-byte header + block data
        if(piece_message.empty() || piece_message[0] != 7) {
            std::cerr << "Failed to receive piece message." << std::endl;
            return false;
        }

        //Copy the block data into the piece_data buffer
        memcpy(&piece_data[block_offset], &piece_message[13], block_length);
    }

    //Step 6: Verify the piece's integrity by comparing its hash with the piece hash in the torrent file
    std::string piece_hash = sha1(std::string(piece_data.begin(), piece_data.end()));
    return false;

    //Save the piece to disk
    std::ofstream output_file("/tmp/test-piece-" + std::to_string(piece_index), std::ios::binary);
    output_file.write(piece_data.data(), piece_data.size());
    output_file.close();

    return true;
}

// The main complete_handshake function
void complete_handshake(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id, int piece_index, int piece_length) {
    // Step 1: Create a socket
    int client_socket = create_socket();
    if (client_socket == -1) return;

    // Step 2: Set up server address
    sockaddr_in server_address;
    if (!setup_server_address(ip, port, server_address)) {
        close(client_socket);
        return;
    }

    // Step 3: Connect to the server
    if (!connect_to_server(client_socket, server_address)) {
        close(client_socket);
        return;
    }

    // Step 4: Prepare the handshake message
    std::string handshake_message = prepare_handshake_message(info_hash, peer_id);

    // Step 5: Send the handshake message
    if (!send_message(client_socket, handshake_message)) {
        close(client_socket);
        return;
    }

    // Step 6: Receive the handshake response
    char response_buffer[68] = {0}; // Buffer for the response
    if (!receive_handshake_response(client_socket, response_buffer, sizeof(response_buffer))) {
        close(client_socket);
        return;
    }

    // Handshake is complete; now proceed to download a piece

    // Step 7: Download a piece from the peer
    if (!download_piece(client_socket, piece_index, piece_length, info_hash)) {
        std::cerr << "Failed to download piece " << piece_index << std::endl;
        close(client_socket);
        return;
    }

    // Close the socket after completing the handshake
    close(client_socket);
}