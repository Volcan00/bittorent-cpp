#include "DecodeFunctions.h"

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