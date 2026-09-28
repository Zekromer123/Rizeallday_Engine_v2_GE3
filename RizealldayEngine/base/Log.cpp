#include "base/Log.h"

void Log::logOutput(const std::string& message)
{
	OutputDebugStringA(message.c_str());
	if (logFile.is_open())
	{
		logFile << message << std::endl;
	}
}

void Log::saveLogs()
{
	std::filesystem::create_directory("logs");
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	std::string logFilePath = std::string("logs/") + dateString + ".log";
	logFile.open(logFilePath);
}

Log::~Log()
{
	if (logFile.is_open())
	{
		logFile.close();
	}
}
