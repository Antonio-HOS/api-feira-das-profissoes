#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <string>

bool isValidIP(const std::string& ip);
bool isValidComPort(const std::string& port);
bool isValidConnectionTarget(const std::string& value);

#endif
