#pragma once

#include <stdint.h>
#include <string>
#include <mutex>

#include <fmt/color.h>

#ifdef _WIN32
#define DEBUG_BREAK() __debugbreak()
#elif __linux__
#define DEBUG_BREAK() __builtin_debugtrap()
#elif __APPLE__
#define DEBUG_BREAK() __builtin_trap()
#endif

namespace srtv_engine
{

class Logger {
  public:

	enum class Severity : uint8_t {
		all = 0,
		trace,
		debug,
		info,
		warning,
		error,
		fatal,
		off
	};

	Logger() = default;

	virtual ~Logger() {
		freeFile();
	}

	void setSeverity(Severity severity)
	{
		_actualSeverity = severity;
	}

	void setLogFile(std::string filePath);

	void setFileWriting(bool value)
	{
		_writingToFile = value;
	}

	void logTrace(std::string message)
	{
		log(Severity::trace, std::move(message));
	}

	void logDebug(std::string message)
	{
		log(Severity::debug, std::move(message));
	}

	void logInfo(std::string message)
	{
		log(Severity::info, std::move(message));
	}

	void logWarning(std::string message)
	{
		log(Severity::warning, std::move(message));
	}

	void logError(std::string message)
	{
		log(Severity::error, std::move(message));
	}

	void logFatal(std::string message)
	{
		log(Severity::fatal, std::move(message));
		DEBUG_BREAK();
		abort();
	}

  private:

	Severity _actualSeverity{ Severity::all };

	std::mutex _logMutex;
	bool _writingToFile{ false };
	std::FILE* _logFile;

	// delete copy constructor and copy assignment operator
	Logger(const Logger&) = delete;
	Logger& operator= (const Logger&) = delete;

	void freeFile();

	std::string severityToString(Severity severity) const;

	const fmt::color severityToColor(Severity severity) const;

	void log(Severity severity, std::string message);

	void logToConsole(std::string severityString, const fmt::color backgroundColor, std::string message);

	void logToFile(std::string severityString, std::string message);
};
} // namespace srtv_engine