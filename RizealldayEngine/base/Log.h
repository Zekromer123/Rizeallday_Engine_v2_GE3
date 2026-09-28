#pragma once
#include "Windows.h"
#include <string>
#include <format>
#include <filesystem>
#include <fstream>
#include <chrono>

class Log
{
public:
	static Log* GetInstance() {static Log instance; return &instance;}
	void logOutput(const std::string& message);
	void saveLogs();
	~Log();

private:
	Log() = default;
	std::ofstream logFile;
};

