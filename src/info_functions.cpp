#include "InfoFunctions.h"

// Function to read the content of a torrent file
std::string read_torrent_file(const std::string& filename) {
    // Open the file in binary mode
    std::ifstream file(filename, std::ios::binary);

    // Check if the file was opened successfully
    if(!file.is_open()) {
        std::cerr << "Error: Could not open the file " << filename << std::endl;
        return "";
    }

    // Read the file content into a string
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Close the file
    file.close();

    return content; // Return the content of the file
}

// Function to encode a JSON object into bencoded format
std::string bencode(const json& obj) {
    std::string encoded;

    // Handle string encoding
    if(obj.is_string()) {
        encoded += std::to_string(obj.get<std::string>().length()) + ":" + obj.get<std::string>();
    }
    // Handle integer encoding
    else if(obj.is_number_integer()) {
        encoded += "i" + std::to_string(obj.get<int64_t>()) + "e";
    }
    // Handle array encoding
    else if(obj.is_array()) {
        encoded += "l"; // Start of list

        // Encode each item in the array
        for(const auto& item : obj) {
            encoded += bencode(item);
        }

        encoded += "e"; // End of list
    }
    // Handle object (dictionary) encoding
    else if(obj.is_object()) {
        encoded += "d"; // Start of dictionary

        // Encode each key-value pair
        for(const auto& [key, value] : obj.items()) {
            encoded += bencode(key) + bencode(value);
        }

        encoded += "e"; // End of dictionary
    }

    return encoded; // Return the bencoded string
}

// Function to calculate the SHA-1 hash of an input string
std::string sha1(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH]; // Array to hold the hash

    // Calculate SHA-1 hash
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);

    std::ostringstream hex_stream;

    // Convert each byte of the hash to hexadecimal
    for(const auto& byte : hash) {
        hex_stream << std::setw(2) <<  std::setfill('0') << std::hex << static_cast<int>(byte);
    }

    return hex_stream.str(); // Return the hexadecimal hash
}

// Function to handle the info command for reading a torrent file
void get_info(const std::string& filename, std::string& tracker_url, int64_t& file_length, std::string& info_hash, int64_t& piece_length, 
                      std::vector<std::string>& pieces_hashes) {
    try {
        // Read the torrent file
        std::string encoded_value = read_torrent_file(filename);

        // Check if the file content was read successfully
        if(encoded_value.empty()) {
            throw std::runtime_error("Error: Could not read or decode the file.");
        }

        int start_position = 0; // Initialize start position for decoding

        // Decode the bencoded value
        json decoded_value = decode_bencoded_value(encoded_value, start_position);

        // Extract the tracker URL from the decoded value
        if(decoded_value.contains("announce")) {
            tracker_url = decoded_value["announce"].get<std::string>();
        }
        else {
            throw std::runtime_error("Error: Missing 'announce' key in the torrent file");
        }

        // Extract the info dictionary
        if(decoded_value.contains("info")) {
            json info_dict = decoded_value["info"];

            // Extract and display the file length
            if(info_dict.contains("length")) {
                file_length = info_dict["length"].get<int64_t>();
            }
            else {
                throw std::runtime_error("Error: Missing 'length' key in dictionary.");
            }

            // Bencode the info dictionary and calculate its SHA-1 hash
            std::string bencoded_info = bencode(info_dict);
            info_hash = sha1(bencoded_info);

            //Extract and display the piece length
            if(info_dict.contains("piece length")) {
                piece_length = info_dict["piece length"].get<std::int64_t>();
            }
            else {
                throw std::runtime_error("Error: Missing 'piece length' key in dictionary.");
            }
            
            // Check if the "pieces" key exists, which contains concatenated SHA-1 hashes (20 bytes each) for the file's pieces.
            if(info_dict.contains("pieces")) {
                // Extract the "pieces" key, which is a concatenated string of 20-byte SHA-1 hashes
                json pieces = info_dict["pieces"];

                // Convert the json pieces to a string
                std::string pieces_string = pieces.get<std::string>();

                // Each piece hash is 20 bytes
                int64_t pieces_size = pieces_string.size();
                int64_t piece_size = 20;

                // Iterate over the pieces_string, taking chunks of 20 bytes and converting to hexadecimal
                for(int i = 0; i < pieces_size / piece_size; ++i) {
                    // Extract the 20-byte SHA-1 hash for each piece
                    std::string piece_hash = pieces_string.substr(i * piece_size, piece_size);

                    // Convert the piece hash into hexadecimal format for output
                    std::ostringstream oss;
                    for (unsigned char c : piece_hash) {
                        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
                    }

                    // Push the hexadecimal hash of the piece into the vector
                    pieces_hashes.push_back(oss.str());
                }
            }
            else {
                throw std::runtime_error("Error: Missing 'pieces' key in dictionary.");
            }
        }
        else {
            throw std::runtime_error("Error: Missing 'info' dictionary in torrent file.");
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl; // Output any caught exceptions
    }
}

void print_info(const std::string& tracker_url, const int64_t& file_length, const std::string& info_hash, const int64_t& piece_length, 
                const std::vector<std::string>& pieces_hashes) {
    std::cout << "Tracker URL: " << tracker_url << std::endl;
    std::cout << "Length: " << file_length << std::endl;
    std::cout << "Info Hash: " << info_hash << std::endl;
    std::cout << "Piece Length: " << piece_length << std::endl;
    std::cout << "Piece Hashes: " << std::endl;

    for(const auto& hash : pieces_hashes) {
        std::cout << hash << std::endl;
    }
}