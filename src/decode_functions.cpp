#include "DecodeFunctions.h"

std::string read_torrent_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    if(!file.is_open()) {
        std::cerr << "Error: Could not open the file " << filename << std::endl;
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    file.close();

    return content;
}

json decode_bencoded_string(const std::string &encoded_value, int &start_position)
{
    size_t colon_index = encoded_value.find(':', start_position);

    if (colon_index != std::string::npos)
    {
        std::string number_string = encoded_value.substr(start_position, colon_index);
        int64_t number = std::atoll(number_string.c_str());

        if(colon_index + 1 + number > encoded_value.size()) {
            throw std::runtime_error("Invalid encoded string length: " + encoded_value);
        }

        std::string str = encoded_value.substr(colon_index + 1, number);
        start_position = number + colon_index + 1;

        return json(str);
    }
    else
    {
        throw std::runtime_error("Invalid encoded string format: " + encoded_value);
    }
}

json decode_encoded_integer(const std::string &encoded_value, int &start_position)
{
    int end_position = encoded_value.find('e', start_position);

    if (end_position != std::string::npos)
    {
        ++start_position;

        std::string number_string = encoded_value.substr(start_position, end_position - 1);
        int64_t number = std::atoll(number_string.c_str());

        start_position = end_position + 1;

        return json(number);
    }
    else
    {
        throw std::runtime_error("Invalid encoded integer format: " + encoded_value);
    }
}

json decode_bencoded_list(const std::string &encoded_value, int &start_position)
{
    ++start_position;

    json result = json::array();

    while (start_position < encoded_value.size() && encoded_value[start_position] != 'e')
    {
        result += decode_bencoded_value(encoded_value, start_position);
    }

    if (encoded_value[start_position] == 'e') {
        ++start_position;
    } 
    else {
        throw std::runtime_error("Invalid end of list: " + encoded_value);
    }

    return result;
}

json decode_bencoded_dictionary(const std::string &encoded_value, int &start_position)
{
    ++start_position;

    json result = json::object();

    while (start_position < encoded_value.size() && encoded_value[start_position] != 'e')
    {
        json key = decode_bencoded_value(encoded_value, start_position);
        json value = decode_bencoded_value(encoded_value, start_position);

        if (key.is_string()) {
            result[key.get<std::string>()] = value;
        } else {
            throw std::runtime_error("Invalid dictionary key type: " + encoded_value);
        }
    }

    if (encoded_value[start_position] == 'e') {
        ++start_position;
    }
    else
    {
        throw std::runtime_error("Invalid end of dictionary: " + encoded_value);
    }

    return result;
}

json decode_bencoded_value(const std::string &encoded_value, int &start_position)
{
    int encoded_value_len = static_cast<int>(encoded_value.length());

    if (std::isdigit(encoded_value[start_position]))
    {
        // Example: "5:hello" -> "hello"
        return decode_bencoded_string(encoded_value, start_position);
    }
    else if (encoded_value[start_position] == 'i')
    {
        // Example "i42e" -> 42
        return decode_encoded_integer(encoded_value, start_position);
    }
    else if (encoded_value[start_position] == 'l')
    {
        // Example "l5:helloi52ee" -> ["hello", 52]
        return decode_bencoded_list(encoded_value, start_position);
    }
    else if (encoded_value[start_position] == 'd')
    {
        return decode_bencoded_dictionary(encoded_value, start_position);
    }
    else
    {
        throw std::runtime_error("Unhandled encoded value: " + encoded_value);
    }
}