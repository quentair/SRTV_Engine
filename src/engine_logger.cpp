#include "engine_logger.h"

namespace srtv_engine {

void EngineLogger::init()
{
	_engineLogger->setSeverity(srtv_engine::Logger::Severity::all);

	_engineLogger->setFileWriting(true);

	_engineLogger->setLogFile("log.txt");
}

} // namespace srtv_engine