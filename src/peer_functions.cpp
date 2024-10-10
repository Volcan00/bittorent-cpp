#include "PeerFunctions.h"

// Function to convert a hexadecimal string to a binary (byte) string
std::string hex_to_binary(const std::string& hex) {
    std::string binary;  // String to hold the resulting binary (byte) data
    
    // Loop through the input hexadecimal string, processing 2 characters (1 byte) at a time
    for(size_t i = 0; i < hex.length(); i += 2) {
        // Extract 2-character substring (representing 1 byte in hex)
        std::string byte_string = hex.substr(i, 2);
        
        // Convert the 2-character hex string to a single byte (char) using std::stoi
        // The '16' specifies that the input string is in base 16 (hexadecimal)
        char byte = static_cast<char>(std::stoi(byte_string, nullptr, 16));
        
        // Append the resulting byte to the binary string
        binary.push_back(byte);
    }
    
    // Return the binary (byte) string
    return binary;
}

// Helper function to URL-encode the info_hash and other parameters
std::string url_encode(const std::string& value) {
    CURL *curl = curl_easy_init();
    std::string result = "";

    if(curl) {
        char *output = curl_easy_escape(curl, value.c_str(), value.length());
        if (output) {
            result = output;
            curl_free(output);
        }
        curl_easy_cleanup(curl);
    }

    return result;
}

//Callback function to capture the response data
static size_t write_callback(void* content, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append(static_cast<char*>(content), total_size);
    return total_size;
}

// Function to send the GET request to the tracker
void send_tracker_get_request(const std::string& tracker_url, const std::string info_hash, const std::string& peer_id, int port, int64_t uploaded, 
                              int64_t downloaded, int64_t left, bool compact) {
    // Initialize libcurl
    CURL *curl = curl_easy_init();

    std::string response; // String to strore response

    if(curl) {
        // Construct the URL with all the required parameters
        std::ostringstream url;
        url << tracker_url
            << "?info_hash=" << url_encode(hex_to_binary(info_hash))
            << "&peer_id=" << peer_id
            << "&port=" << port
            << "&uploaded=" << uploaded
            << "&downloaded=" << downloaded
            << "&left=" << left
            << "&compact=" << compact;

        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

        //Set the callback function to capture response into the 'response' string
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Send the request and capture the response
        CURLcode res = curl_easy_perform(curl);

        //Check for the errors
        if(res != CURLE_OK) {
            std::cerr << "Error in curl_easy_perform(): " << curl_easy_strerror(res) << std::endl;
        }
        else {
            int start_position = 0;
            json decoded_response = decode_bencoded_value(response, start_position);

            if(decoded_response.contains("peers")) {
                std::string peers = decoded_response["peers"].get<std::string>();

                if(peers.size() % 6 != 0) {
                    std::cerr << "Error: Invalid peers string length." << std::endl;
                    return;
                }

                    for(size_t i = 0; i < peers.size(); i += 6) {
                        std::string ip = std::to_string(static_cast<uint8_t>(peers[i])) + "." +
                                         std::to_string(static_cast<uint8_t>(peers[i + 1])) + "." +
                                         std::to_string(static_cast<uint8_t>(peers[i + 2])) + "." +
                                         std::to_string(static_cast<uint8_t>(peers[i + 3]));

                        uint16_t port = (static_cast<uint8_t>(peers[i + 4]) << 8) | static_cast<uint8_t>(peers[i + 5]);

                        std::cout << ip << ":" << port << std::endl;
                    }
            }
            else {
                std::cerr << "Error: Missing 'peers' key in dictionary." << std::endl;
            }
        }

        curl_easy_cleanup(curl);
    }
    else {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }
}