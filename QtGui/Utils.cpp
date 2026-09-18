#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <regex>

bool isValidIP(const std::string& ip)
{
    const std::regex ip_regex(
        R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(ip, ip_regex);
}

bool isValidComPort(const std::string& port)
{
    std::string upper = port;
    std::transform(upper.begin(), upper.end(), upper.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    const std::regex com_regex(R"(^COM([1-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-6])$)");
    return std::regex_match(upper, com_regex);
}

bool isValidConnectionTarget(const std::string& value)
{
    return isValidComPort(value) || isValidIP(value);
}
