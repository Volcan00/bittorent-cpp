#include "DecodeFunctions.h"
#include "InfoFunctions.h"
#include "PeerFunctions.h"
#include "HandshakeFunctions.h"

int main(int argc, char* argv[]) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    std::string tracker_url = "";
    std::string peer_id = "89504192450758722672";
    int64_t file_length;
    std::string info_hash = "";
    int64_t piece_length;
    std::vector<std::string> pieces_hashes = {};


    if (command == "decode") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        std::string encoded_value = argv[2];
        handle_decode_command(encoded_value);
    }
    else if(command == "info") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        get_info(filename, tracker_url, file_length, info_hash, piece_length, pieces_hashes);
        print_info(tracker_url, file_length, info_hash, piece_length, pieces_hashes);
    }
    else if(command == "peers") {
        if(argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        get_info(filename, tracker_url, file_length, info_hash, piece_length, pieces_hashes);

        int port = 6881;
        int64_t uploaded = 0;
        int64_t downloaded = 0;
        int64_t left = file_length;
        bool compact = true;

        send_tracker_get_request(tracker_url, info_hash, peer_id, port, uploaded, downloaded, left, compact);
    }
    else if(command == "handshake") {
        if(argc < 4) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        std::string peer = argv[3];

        get_info(filename, tracker_url, file_length, info_hash, piece_length, pieces_hashes);

        size_t colon_index = peer.find(':');

        if(colon_index == std::string::npos) {
            std::cerr << "Invalid peer address format." << std::endl;
            return 1;
        }

        std::string ip = peer.substr(0, colon_index);
        int port = std::stoi(peer.substr(colon_index + 1));

        complete_handshake(ip, port, info_hash, peer_id);
    }
    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
