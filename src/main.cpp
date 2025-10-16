#include <iostream>

#include "engine.h"
#include "engine_logger.h"

int main(int argc, char* argv[]) {

	srtv_engine::EngineLogger::init();

	srtv_engine::ENGINE_LOG_TRACE("TRACE_TEST");

	srtv_engine::ENGINE_LOG_DEBUG("DEBUG_TEST");

	srtv_engine::ENGINE_LOG_INFO("INFO_TEST");

	srtv_engine::ENGINE_LOG_WARNING("WARNING_TEST");

	srtv_engine::ENGINE_LOG_ERROR("ERROR_TEST");

	srtv_engine::ENGINE_LOG_FATAL("FATAL_TEST");

	srtv_engine::Engine engine{};

	if (!engine.init()) {
		std::cerr << "Engine initialisation failed." << std::endl;
	}

	engine.run();

	engine.cleanup();

	return 0;
}