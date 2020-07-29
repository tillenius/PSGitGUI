#pragma once

#include <string>

class VisualStudioInterop {
public:
    static std::wstring envdte;

    static bool OpenInVS(const std::string & filename, int line);
    static bool ExecuteCommand(const std::string & command);
    static std::wstring GetPathOfActiveDocument();
};
