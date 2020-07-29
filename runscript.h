#pragma once

#include <functional>
#include <string>

void exec(const std::wstring & cwd, const std::wstring & cmd, std::function<void(const char * data, DWORD size)> callback);
