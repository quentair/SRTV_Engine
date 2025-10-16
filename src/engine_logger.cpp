#include "engine_logger.h"

namespace srtv_engine {

const std::unique_ptr<Logger> EngineLogger::_engineLogger = std::make_unique<Logger>();

void EngineLogger::init()
{
	_engineLogger->setSeverity(srtv_engine::Logger::Severity::all);

	_engineLogger->setFileWriting(true);

	_engineLogger->setLogFile("log.txt");
}

} // namespace srtv_engine