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
    int port = 6881;
    int64_t uploaded = 0;
    int64_t downloaded = 0;
    int64_t left = file_length;
    bool compact = true;


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

        std::vector<std::string> peers = get_peers(tracker_url, info_hash, peer_id, port, uploaded, downloaded, file_length, compact);
        
        print_peers(peers);
    }
    else if(command == "handshake") {
        if(argc < 4) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        std::string peer = argv[3];

        get_info(filename, tracker_url, file_length, info_hash, piece_length, pieces_hashes);

        std::string ip;
        int ip_port;
        split_ip_and_port(peer, ip, ip_port);

        complete_handshake(ip, ip_port, info_hash, peer_id, 0, piece_length);
    }
    else if(command == "download_piece") {
        if(argc < 6) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1; 
        }

        std::string download_filename = argv[3];
        std::string torrent_filename = argv[4];
        get_info(torrent_filename, tracker_url, file_length, info_hash, piece_length, pieces_hashes);

        std::string peer = get_peers(tracker_url, info_hash, peer_id, port, uploaded, downloaded, file_length, compact)[0];

        std::string ip;
        int ip_port;
        split_ip_and_port(peer, ip, ip_port);

        int piece_index = std::stoi(argv[5]);

        complete_handshake(ip, ip_port, info_hash, peer_id, piece_index, piece_length, download_filename);
    }
    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}