#include "DownloadPieceFunctions.h"

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
bool download_piece(int client_socket, int piece_index, int piece_length, const std::string& expected_hash, const std::string& download_filename, char* file_buffer, int64_t buffer_offset) {
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

    std::string piece(piece_buffer, piece_length);

    std::string calculated_hash = sha1(piece);

    // Check if the calculated hash matches the expected hash
    if (calculated_hash != expected_hash) {
        std::cerr << "Hash mismatch! Downloaded piece is corrupted." << std::endl;
        delete[] piece_buffer; // Cleanup on failure
        return false; // Hash mismatch
    }

    if(file_buffer) {
        std::memcpy(file_buffer + buffer_offset, piece_buffer, piece_length);
    }
    else if(!download_filename.empty()) {
        // Step 11: Write the piece to disk
        std::ofstream output_file(download_filename, std::ios::binary);
        if (!output_file) {
            std::cerr << "Failed to open output file." << std::endl;
            delete[] piece_buffer; // Cleanup on failure
            return false; // Failure to open output file
        }
        output_file.write(piece_buffer, piece_length);
        output_file.close();
    }

    // Cleanup
    delete[] piece_buffer; // Free the buffer
    return true; // Download successful
}

// The main complete_handshake function
void complete_piece_download(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id, int piece_index, int piece_length, const std::string& expected_hash, const std::string& download_filename, bool download) {
    // Establish the connection
    int client_socket = establish_connection(ip, port);
    if (client_socket == -1) return;  // Connection failed

    // Now perform the handshake
    if (!perform_handshake(client_socket, info_hash, peer_id, download)) {
        close(client_socket);
        return;
    }

    // Step 7: Receive and handle the bitfield message
    if (!handle_bitfield_message(client_socket)) {
        return; // Failure to handle bitfield message
    }

    // Step 8: Send the interested message
    if (!send_interested_message(client_socket)) {
        return; // Failure to send interested message
    }

    // Step 9: Wait for the unchoke message
    if (!wait_for_unchoke(client_socket)) {
        return; // Failure to wait for unchoke message
    }

    // Handshake is complete; now proceed to download a piece
    if (!download_piece(client_socket, piece_index, piece_length, expected_hash, download_filename)) {
        close(client_socket);
        return;
    }  
    
    // Cleanup
    close(client_socket);
}