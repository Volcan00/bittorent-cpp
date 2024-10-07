#include "DecodeFunctions.h"

int main(int argc, char* argv[]) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "decode") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        std::string encoded_value = argv[2];
        int start_position = 0;
        json decoded_value = decode_bencoded_value(encoded_value, start_position);
        std::cout << decoded_value.dump() << std::endl;
    }
    else if(command == "info") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        std::string filename = argv[2];

        try {
            std::string encoded_value = read_torrent_file(filename);

            if(encoded_value.empty()) {
                std::cerr << "Error: Could not read or decode the file." << '\n';
                return 1;
            }

            int start_position = 0;

            json decoded_value = decode_bencoded_value(encoded_value, start_position);

            if(decoded_value.contains("announce")) {
                std::string tracker_url = decoded_value["announce"].get<std::string>();
                std::cout << "Tracker URL: " << tracker_url << '\n';
            }
            else {
                std::cerr << "Error: Missing 'announce' key in the torrent file" << '\n';
                return 1;
            }

            if(decoded_value.contains("info")) {
                json info_dict = decoded_value["info"];

                if(info_dict.contains("length")) {
                    int64_t file_length = info_dict["length"].get<int64_t>();
                    std::cout << "Length: " << file_length << '\n';
                }
                else {
                    std::cerr << "Error: Missing 'info' dictionary in torrent file." << '\n';
                    return 1;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
        }
    }
    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
