#include "logger.h"

#include <fmt/core.h>
#include <fmt/chrono.h>

#include <filesystem>

namespace srtv_engine {

void Logger::setLogFile(std::string filePath)
{
	// create log directory if needed
	std::filesystem::path directoryRelativePath = std::filesystem::path("log");
	std::filesystem::create_directory(directoryRelativePath);

	// retrieve absolute path of the log file
	std::filesystem::path logFileRelativePath = directoryRelativePath / std::filesystem::path(filePath);
	std::filesystem::path logFileAbsolutePath = std::filesystem::absolute(logFileRelativePath);

	// register file to write in it later
	filePath = logFileRelativePath.string();

	// trace message for the file change event
	std::string message = "Logger stopped to write in file : " + logFileAbsolutePath.string();

	// try to write in current file to record file change
	if (_logFile) {
		logTrace(message);
	}
	// close current file if it exists
	freeFile();

	// open new file in write only mode
	_logFile = std::fopen(filePath.c_str(), "w");

	// write in new file to record file change
	message = "Logger began to write in file : " + logFileAbsolutePath.string();
	logTrace(message);
}

std::string Logger::severityToString(Severity severity) const
{
	std::string severityString;

	switch (severity) {
		case Severity::trace:
			severityString = "TRACE";
			break;
		case Severity::debug:
			severityString = "DEBUG";
			break;
		case Severity::info:
			severityString = "INFO";
			break;
		case Severity::warning:
			severityString = "WARNING";
			break;
		case Severity::error:
			severityString = "ERROR";
			break;
		case Severity::fatal:
			severityString = "FATAL";
			break;
		default:
			severityString = "WRONG SEVERITY TYPE";
	}

	return severityString;
}

void Logger::freeFile()
{
	if (_logFile) {
		std::fclose(_logFile);
		_logFile = 0;
	}
}

const fmt::color Logger::severityToColor(Severity severity) const
{
	fmt::color color;

	switch (severity) {
	case Severity::trace:
		color = fmt::color::white;
		break;
	case Severity::debug:
		color = fmt::color::blue;
		break;
	case Severity::info:
		color = fmt::color::green;
		break;
	case Severity::warning:
		color = fmt::color::yellow;
		break;
	case Severity::error:
		color = fmt::color::red;
		break;
	case Severity::fatal:
		color = fmt::color::dark_red;
		break;
	default:
		color = fmt::color::white;
	}

	return color;
}

void Logger::log(Severity severity, std::string message)
{
	if (severity < _actualSeverity || _actualSeverity == Severity::off)
		return;

	// log to console and/or file, use a guard lock for threadsafe writes
	{
		const std::lock_guard<std::mutex> lock(_logMutex);

		logToConsole(severityToString(severity), severityToColor(severity), message);

		if (!_writingToFile)
			return;

		if (_logFile) {
			logToFile(severityToString(severity), message);
			return;
		}
	}

	// need to write to file but no file defined : warn user
	setFileWriting(false);
	logWarning("Log file not set : please set a log file with the setLogFile(\"filePath\") function");
}

void Logger::logToConsole(std::string severityString, const fmt::color backgroundColor, std::string message)
{
	auto time = std::chrono::system_clock::now();
	auto timeMilliseconds = std::chrono::time_point_cast<std::chrono::milliseconds>(time);

	fmt::print("[{:%F %H.%M.%S}] ", timeMilliseconds);
	fmt::print(bg(backgroundColor), "[{}]", severityString);
	fmt::print(" : {}\n", message);
}

void Logger::logToFile(std::string severityString, std::string message)
{
	auto time = std::chrono::system_clock::now();
	auto timeMilliseconds = std::chrono::time_point_cast<std::chrono::milliseconds>(time);

	fmt::print(_logFile, "[{:%F %H.%M.%S}] [{}] : {}\n", timeMilliseconds, severityString, message);
}

} // namespace srtv_engine