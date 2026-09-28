#pragma once
#include <string>
#include <windows.h>

class ConvertStrings
{
public:
    std::wstring ConvertString(const std::string & str);
    std::string ConvertString(const std::wstring& str);
};