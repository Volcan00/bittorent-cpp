#include "DecodeFunctions.h"

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

// Function to decode a bencoded string
json decode_bencoded_string(const std::string &encoded_value, int &start_position)
{
    // Find the index of the colon that separates length and string
    size_t colon_index = encoded_value.find(':', start_position);

    // If colon is found, extract the string
    if (colon_index != std::string::npos)
    {
        std::string number_string = encoded_value.substr(start_position, colon_index);
        int64_t number = std::atoll(number_string.c_str()); // Convert length to integer

        // Check if the encoded length exceeds the string length
        if(colon_index + 1 + number > encoded_value.size()) {
            throw std::runtime_error("Invalid encoded string length: " + encoded_value);
        }

        // Extract the string based on the specified length
        std::string str = encoded_value.substr(colon_index + 1, number); // Update start position for next decoding
        start_position = number + colon_index + 1;

        return json(str); // Return the decoded string as a JSON object
    }
    else
    {
        throw std::runtime_error("Invalid encoded string format: " + encoded_value);
    }
}

// Function to decode a bencoded integer
json decode_encoded_integer(const std::string &encoded_value, int &start_position)
{
    // Find the end position marked by 'e'
    int end_position = encoded_value.find('e', start_position);

    // If 'e' is found, extract the integer
    if (end_position != std::string::npos)
    {
        ++start_position; // Move past 'i'

        // Extract the integer from the string
        std::string number_string = encoded_value.substr(start_position, end_position - 1);
        int64_t number = std::atoll(number_string.c_str()); // Convert to integer

        start_position = end_position + 1; // Update start position

        return json(number); // Return the decoded integer as a JSON object
    }
    else
    {
        throw std::runtime_error("Invalid encoded integer format: " + encoded_value);
    }
}

// Function to decode a bencoded list
json decode_bencoded_list(const std::string &encoded_value, int &start_position)
{
    ++start_position; // Move past 'l'

    json result = json::array(); // Create a JSON array to hold the result

    // Loop until the end of the list is reached
    while (start_position < encoded_value.size() && encoded_value[start_position] != 'e')
    {
        result += decode_bencoded_value(encoded_value, start_position); // Decode each item
    }

    // Check for the end of the list
    if (encoded_value[start_position] == 'e') {
        ++start_position; // Move past 'e'
    }
    else {
        throw std::runtime_error("Invalid end of list: " + encoded_value);
    }

    return result; // Return the decoded list as a JSON array
}

// Function to decode a bencoded dictionary
json decode_bencoded_dictionary(const std::string &encoded_value, int &start_position)
{
    ++start_position; // Move past 'd'

    json result = json::object(); // Create a JSON object to hold the result

    // Loop until the end of the dictionary is reached
    while (start_position < encoded_value.size() && encoded_value[start_position] != 'e')
    {
        json key = decode_bencoded_value(encoded_value, start_position); // Decode key
        json value = decode_bencoded_value(encoded_value, start_position); // Decode value

        // Ensure the key is a string
        if (key.is_string()) {
            result[key.get<std::string>()] = value; // Insert key-value pair into the object
        } else {
            throw std::runtime_error("Invalid dictionary key type: " + encoded_value);
        }
    }

    // Check for the end of the dictionary
    if (encoded_value[start_position] == 'e') {
        ++start_position; // Move past 'e'
    }
    else
    {
        throw std::runtime_error("Invalid end of dictionary: " + encoded_value);
    }

    return result; // Return the decoded dictionary as a JSON object
}

// Function to decode a bencoded value (string, integer, list, or dictionary)
json decode_bencoded_value(const std::string &encoded_value, int &start_position)
{
    int encoded_value_len = static_cast<int>(encoded_value.length());

    // Check if the next character indicates a string
    if (std::isdigit(encoded_value[start_position]))
    {
        // Example: "5:hello" -> "hello"
        return decode_bencoded_string(encoded_value, start_position);
    }
    // Check if the next character indicates an integer
    else if (encoded_value[start_position] == 'i')
    {
        // Example "i42e" -> 42
        return decode_encoded_integer(encoded_value, start_position);
    }
    // Check if the next character indicates a list
    else if (encoded_value[start_position] == 'l')
    {
        // Example "l5:helloi52ee" -> ["hello", 52]
        return decode_bencoded_list(encoded_value, start_position);
    }
    // Check if the next character indicates a dictionary
    else if (encoded_value[start_position] == 'd')
    {
        return decode_bencoded_dictionary(encoded_value, start_position);
    }
    else
    {
        throw std::runtime_error("Unhandled encoded value: " + encoded_value);
    }
}

// Function to handle the decode command
void handle_decode_command(const std::string& encoded_value) {
    int start_position = 0; // Initialize start position for decoding
    json decoded_value = decode_bencoded_value(encoded_value, start_position); // Decode the value
    std::cout << decoded_value.dump() << std::endl; // Output the decoded JSON
}

// Function to handle the info command for reading a torrent file
void handle_info_command(const std::string& filename) {
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
            std::string tracker_url = decoded_value["announce"].get<std::string>();
            std::cout << "Tracker URL: " << tracker_url << std::endl;
        }
        else {
            throw std::runtime_error("Error: Missing 'announce' key in the torrent file");
        }

        // Extract the info dictionary
        if(decoded_value.contains("info")) {
            json info_dict = decoded_value["info"];

            // Extract and display the file length
            if(info_dict.contains("length")) {
                int64_t file_length = info_dict["length"].get<int64_t>();
                std::cout << "Length: " << file_length << std::endl;
            }
            else {
                throw std::runtime_error("Error: Missing 'length' key in dictionary.");
            }

            // Bencode the info dictionary and calculate its SHA-1 hash
            std::string bencoded_info = bencode(info_dict);
            std::string info_hash = sha1(bencoded_info);
            std::cout << "Info Hash: " << info_hash << std::endl;

            //Extract and display the piece length
            if(info_dict.contains("piece length")) {
                int64_t piece_length = info_dict["piece length"].get<std::int64_t>();
                std::cout << "Piece Length: " << piece_length << std::endl;
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

                std::cout << "Piece Hashes: " << std::endl;

                // Iterate over the pieces_string, taking chunks of 20 bytes and converting to hexadecimal
                for(int i = 0; i < pieces_size / piece_size; ++i) {
                    // Extract the 20-byte SHA-1 hash for each piece
                    std::string piece_hash = pieces_string.substr(i * piece_size, piece_size);

                    // Convert the piece hash into hexadecimal format for output
                    std::ostringstream oss;
                    for (unsigned char c : piece_hash) {
                        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
                    }

                    // Print the hexadecimal hash of the piece
                    std::cout << oss.str() << std::endl;
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