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

// Helper function to receive a peer message and extract message id and payload length
bool receive_peer_message(int client_socket, int &message_id, size_t& payload_length) {
    //Buffer for message length and ID
    char header[5] = {0};

    //Receive 4 bytes (message length) + 1 byte (message ID)
    ssize_t bytes_received = recv_all(client_socket, header, sizeof(header));
    if(bytes_received != sizeof(header)) {
        std::cerr << "Failed to receive peer message header." << std::endl;
        return false;
    }

    //Extract message length (4 bytes, big-endian) and message ID (1 byte)
    uint32_t length = ntohl(*(uint32_t*)header); // Convert network byte order to host byte order
    message_id = static_cast<unsigned char>(header[4]); // 5th byte is the message ID
    payload_length = length - 1; // Subtract 1 for the message ID

    return true;
}

// Function to receive and skip the bitfield message
bool handle_bitfield_message(int client_socket) {
    int message_id;
    size_t payload_length;

    // Receive the message
    if(!receive_peer_message(client_socket, message_id, payload_length)) {
        return false;
    }

    // Check if it's a bitfield message (ID 5)
    if(message_id == 5) {
        // Skip the payload
        char* payload_buffer = new char[payload_length];
        ssize_t bytes_skipped = recv_all(client_socket, payload_buffer, payload_length);
        delete[] payload_buffer;

        if(bytes_skipped != payload_length) {
            std::cerr << "Error skipping bitfield payload." << std::endl;
            return false;
        }
    }
    else {
        std::cerr << "Expected bitfield message, but received another message with ID: " << message_id << std::endl;
        return false;
    }

    return true;
}

// Helper function to send the "interested" message
bool send_interested_message(int client_socket) {
    // Construct the interested message: 4 bytes for length (1) + 1 byte for message ID (2)
    char message[5] = {0}; // 4 bytes for length + 1 byte for message ID

    // Set message length (4 bytes in big-endian format) and message ID (2)
    uint32_t message_length = htonl(1); // The length is 1 because there's no payload
    memcpy(message, &message_length, sizeof(message_length));
    message[4] = 2; // Message ID for "interested"

    //Send message
    ssize_t bytes_sent = send(client_socket, message, sizeof(message), 0);
    if(bytes_sent != sizeof(message)) {
        std::cerr << "Error sending interested message." << std::endl;
        return false;
    }

    return true;
}

// Helper function to wait for the "unchoke" message
bool wait_for_unchoke(int client_socket) {
    int message_id;
    size_t payload_length;

    // Loop to continuously wait for messages until an unchoke message is received
    while(true) {
        //Receive the next peer message
        if(!receive_peer_message(client_socket, message_id, payload_length)) {
            return false;
        }

        // Check if it's an unchoke message (ID 1)
        if(message_id == 1) {
            return true;
        }
        else {
            if(payload_length > 0) {
                char* payload_buffer = new char[payload_length];
                ssize_t bytes_skipped = recv_all(client_socket, payload_buffer, payload_length);
                delete[] payload_buffer;

                if (bytes_skipped != payload_length) {
                    std::cerr << "Error skipping message payload." << std::endl;
                    return false;
                }
            }
        }
    }
}

// Helper function to send a request for a block of a piece
bool send_request_message(int client_socket, int piece_index, int block_offset, int block_length) {
    char message[17]; // 4 bytes for length, 1 byte for message ID, and 12 bytes for the request

    // Construct the message
    uint32_t message_length = htonl(13); // The length is 13 because we have 12 bytes for the request and 1 for the ID
    memcpy(message, &message_length, sizeof(message_length));
    message[4] = 6; // Message ID for "request"

    // Set up the request payload
    uint32_t piece_index_n = htonl(piece_index);
    uint32_t block_offset_n = htonl(block_offset);
    uint32_t block_length_n = htonl(block_length);

    memcpy(message + 5, &piece_index_n, sizeof(piece_index_n));
    memcpy(message + 9, &block_offset_n, sizeof(block_offset_n));
    memcpy(message + 13, &block_length_n, sizeof(block_length_n));

    // Send the message
    ssize_t bytes_sent = send(client_socket, message, sizeof(message), 0);
    if (bytes_sent != sizeof(message)) {
        std::cerr << "Error sending request message." << std::endl;
        return false;
    }

    return true;
}

// Helper function to receive a piece block and write it into the buffer
bool receive_piece_block(int client_socket, char* piece_buffer, int piece_index, int block_offset, int block_length) 
{
    // Message header is 9 bytes: 4 bytes for piece index, 4 bytes for block offset, 1 byte for ID
    char message_header[13];

    // Receive the message header (13 bytes total: 4 for length, 1 for ID, 4 for index, 4 for offset)
    ssize_t bytes_received = recv_all(client_socket, message_header, sizeof(message_header));
    
    if (bytes_received < 0) {
        // An error occurred while receiving the header
        std::cerr << "Error receiving piece header: " << strerror(errno) << std::endl;
        return false;
    } else if (bytes_received == 0) {
        // The connection was closed before we could receive any data
        std::cerr << "Connection closed by the peer while receiving piece header." << std::endl;
        return false;
    }

    // Extract the message length (first 4 bytes)
    uint32_t message_length = ntohl(*reinterpret_cast<uint32_t*>(message_header));

    // Extract the message ID (next 1 byte, should be 7 for "piece")
    uint8_t message_id = static_cast<uint8_t>(message_header[4]);
    if (message_id != 7) {
        std::cerr << "Unexpected message ID: " << static_cast<int>(message_id) << std::endl;
        return false;
    }

    // Extract the piece index and block offset
    uint32_t received_piece_index = ntohl(*reinterpret_cast<uint32_t*>(message_header + 5));
    uint32_t received_block_offset = ntohl(*reinterpret_cast<uint32_t*>(message_header + 9));

    if (received_piece_index != piece_index || received_block_offset != block_offset) {
        std::cerr << "Piece or block offset mismatch. Expected piece index: " << piece_index 
                  << ", received: " << received_piece_index 
                  << ", Expected block offset: " << block_offset 
                  << ", received: " << received_block_offset << std::endl;
        return false;
    }

    // Now receive the block data
    ssize_t block_bytes_received = recv_all(client_socket, piece_buffer + block_offset, block_length);

    if (block_bytes_received < 0) {
        // Some error occurred during receiving
        std::cerr << "Error receiving piece block data: " << strerror(errno) << std::endl;
        return false;
    } else if (block_bytes_received == 0) {
        // Connection closed before receiving the full block
        std::cerr << "Connection closed before receiving full block data." << std::endl;
        return false;
    }

    if (block_bytes_received != block_length) {
        std::cerr << "Expected " << block_length << " bytes, but received " << block_bytes_received << " bytes." << std::endl;
        return false;
    }

    return true;
}

// Function to download a piece
bool download_piece(int client_socket, int piece_index, int piece_length, const std::string& download_filename) {
    // Step 7: Receive and handle the bitfield message
    if (!handle_bitfield_message(client_socket)) {
        return false; // Failure to handle bitfield message
    }

    // Step 8: Send the interested message
    if (!send_interested_message(client_socket)) {
        return false; // Failure to send interested message
    }

    // Step 9: Wait for the unchoke message
    if (!wait_for_unchoke(client_socket)) {
        return false; // Failure to wait for unchoke message
    }

    // Step 10: Break the piece into blocks and request them
    const int BLOCK_SIZE = 16 * 1024; // 16 KiB block size
    char* piece_buffer = new char[piece_length]; // Buffer to hold the entire piece
    size_t total_received = 0; // Track total bytes received

    for (int block_offset = 0; block_offset < piece_length; block_offset += BLOCK_SIZE) {
        int block_length = std::min(BLOCK_SIZE, piece_length - block_offset);

        // Send the request for this block
        if (!send_request_message(client_socket, piece_index, block_offset, block_length)) {
            delete[] piece_buffer; // Cleanup on failure
            return false; // Failure to send request message
        }

        // Receive the block data
        if (!receive_piece_block(client_socket, piece_buffer, piece_index, block_offset, block_length)) {
            delete[] piece_buffer; // Cleanup on failure
            return false; // Failure to receive piece block
        }

        // Check if the block was received as a 0-byte message indicating download completion
        if (block_length == 0) {
            break; // Exit the loop if the download is complete
        }

        // Update total bytes received
        total_received += block_length;

        // If we have received all data for the piece, break out of the loop
        if (total_received >= piece_length) {
            break; // All blocks for this piece have been received
        }
    }

    // Step 11: Write the piece to disk
    std::ofstream output_file(download_filename, std::ios::binary);
    if (!output_file) {
        std::cerr << "Failed to open output file." << std::endl;
        delete[] piece_buffer; // Cleanup on failure
        return false; // Failure to open output file
    }

    output_file.write(piece_buffer, piece_length);
    output_file.close();

    // Cleanup
    delete[] piece_buffer; // Free the buffer
    return true; // Download successful
}


// The main complete_handshake function
void complete_download(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id, int piece_index, int piece_length, const std::string& download_filename, bool download) {
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

    // Now perform the handshake
    if (!perform_handshake(client_socket, info_hash, peer_id, download)) {
        close(client_socket);
        return;
    }

    if(download) {
        // Handshake is complete; now proceed to download a piece
        if (!download_piece(client_socket, piece_index, piece_length, download_filename)) {
            close(client_socket);
            return;
        }
    }    
    

    // Cleanup
    close(client_socket);
}