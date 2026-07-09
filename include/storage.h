#pragma once

#include <string>

std::string storageRead(const std::string& key);
void        storageWrite(const std::string& key, const std::string& text);
