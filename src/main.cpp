#include "engine.h"
#include "logger.h"
#include <iostream>

int main(int argc, char* argv[]) {
	
	srtv_engine::Logger logger{};

	logger.setFileWriting(true);

	logger.setLogFile("log.txt");

	srtv_engine::Engine engine{};

	if (!engine.init()) {
		std::cerr << "Engine initialisation failed." << std::endl;
	}

	logger.setSeverity(srtv_engine::Logger::Severity::all);

	logger.logTrace("TRACE");

	logger.logDebug("DEBUG");

	logger.logInfo("INFO");

	logger.logError("ERROR");

	logger.logWarning("WARNING");

	logger.logFatal("FATAL");

	logger.setFileWriting(false);

	logger.logError("ERROR");

	logger.logWarning("WARNING");

	engine.run();

	engine.cleanup();

	return 0;
}