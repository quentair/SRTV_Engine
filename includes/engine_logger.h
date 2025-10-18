#pragma once

#include "logger.h"

namespace srtv_engine {

class EngineLogger {
  public:

	static void init();

	inline static Logger& getLogger(){ return *_engineLogger; }

  private:

	EngineLogger() = delete;

	inline static const std::unique_ptr<Logger> _engineLogger = std::make_unique<Logger>();

	// delete copy constructor and copy assignment operator
	EngineLogger(const EngineLogger&) = delete;
	EngineLogger& operator= (const EngineLogger&) = delete;
};

#define ENGINE_LOG_TRACE(x) EngineLogger::getLogger().logTrace(x)

#ifdef _DEBUG 
#define ENGINE_LOG_DEBUG(x) EngineLogger::getLogger().logDebug(x)
#else
#define DEBUG_ENGINE_WARNING(x)
#endif

#define ENGINE_LOG_INFO(x) EngineLogger::getLogger().logInfo(x)

#define ENGINE_LOG_WARNING(x) EngineLogger::getLogger().logWarning(x)

#define ENGINE_LOG_ERROR(x) EngineLogger::getLogger().logError(x)

#define ENGINE_LOG_FATAL(x) EngineLogger::getLogger().logFatal(x)

} // namespace srtv_engine