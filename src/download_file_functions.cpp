#include "DownloadFileFunctions.h"

// Helper function to download the entire file into a buffer and write it to disk
bool download_full_file(int client_socket, const std::vector<std::string>& piece_hashes, int piece_length, int file_length, const std::string& download_filename) {
    // Step 1: Pre-allocate buffer to store the full file
    char* file_buffer = new char[file_length];

    // Step 2: Loop through each piece, download it, and store it in the buffer
    for (int i = 0; i < piece_hashes.size(); ++i) {
        int current_piece_size = (i == piece_hashes.size() - 1) ? (file_length % piece_length) : piece_length;
        int buffer_offset = i * piece_length;

        if (!download_piece(client_socket, i, current_piece_size, piece_hashes[i], "", file_buffer, buffer_offset)) {
            std::cerr << "Failed to download piece " << i << ". Aborting download." << std::endl;
            delete[] file_buffer;
            return false;
        }
    }

    // Step 3: Write the buffer to disk
    std::ofstream output_file(download_filename, std::ios::binary);
    if (!output_file) {
        std::cerr << "Failed to open output file: " << download_filename << std::endl;
        delete[] file_buffer;
        return false;
    }

    output_file.write(file_buffer, file_length);
    output_file.close();

    // Step 4: Cleanup
    delete[] file_buffer;
    return true;
}

void complete_file_download(const std::string& peer_ip, int port, const std::string& info_hash, const std::string& peer_id, const std::vector<std::string>& piece_hashes, int piece_length, int file_length, const std::string& download_filename) {
    // Step 1: Establish a connection to the peer
    int client_socket = establish_connection(peer_ip, port);
    if (client_socket == -1) {
        std::cerr << "Failed to connect to peer." << std::endl;
        return;
    }

    // Step 2: Perform the handshake with the peer
    if (!perform_handshake(client_socket, info_hash, peer_id, true)) {
        close(client_socket);
        return;
    }

    // Step 3: Handle bitfield message (optional)
    if (!handle_bitfield_message(client_socket)) {
        close(client_socket);
        return;
    }

    // Step 4: Send interested message and wait for unchoke
    if (!send_interested_message(client_socket) || !wait_for_unchoke(client_socket)) {
        close(client_socket);
        return;
    }

    // Step 5: Download the file and write it to disk
    if (!download_full_file(client_socket, piece_hashes, piece_length, file_length, download_filename)) {
        std::cerr << "File download failed." << std::endl;
    }

    // Step 8: Cleanup
    close(client_socket);
}
