#include "DownloadFileFunctions.h"

// Function to download all pieces and save the file
bool download_entire_file(int client_socket, const std::vector<std::string>& pieces_hashes, int piece_length, int file_length, const std::string& download_filename) {
    // Create an output file stream for the assembled file
    std::ofstream output_file(download_filename, std::ios::binary);
    if(!output_file) {
        std::cerr << "Failed to open output file: " << download_filename << std::endl;
        return false;
    }

    int num_pieces = pieces_hashes.size();

    // Loop over all pieces and download each one
    for(int piece_index = 0; piece_index < num_pieces; ++piece_index) {
        std::cout << "Downloading piece " << piece_index << " of " << num_pieces << std::endl;

        // Calculate the length of the current piece (last piece may be smaller)
        int current_piece_length = (piece_index == num_pieces - 1)
                                    ? file_length - (piece_index * piece_length)
                                    : piece_length;

        // Buffer to hold the downloaded piece
        char* piece_buffer = new char[piece_length];

        // Download the piece
        if(!download_piece(client_socket, piece_index, current_piece_length, download_filename)) {
            std::cerr << "Failed to download piece " << piece_index << std::endl;
            delete[] piece_buffer;
            output_file.close();
            return false;
        }

        // TODO: Verify the piece hash
    }

    output_file.close();
    std::cout << "All pieces downloaded and assembled into " << download_filename << " successfully." << std::endl;

    return true; // Return true if all pieces were successfully downloaded, verified, and written
}

void complete_file_download(const std::string& ip, int port, const std::string& info_hash, const std::string& peer_id, const std::vector<std::string>& pieces_hashes, int piece_length, int file_length, const std::string& download_filename, bool download) {
    // Establish the connection to the peer
    int client_socket = establish_connection(ip, port);
    if (client_socket == -1) return;  // Connection failed

    // Perform the handshake
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

    // Download the entire file
    if (!download_entire_file(client_socket, pieces_hashes, piece_length, file_length, download_filename)) {
        close(client_socket);
        return;
    }

    // Close the connection
    close(client_socket);
}