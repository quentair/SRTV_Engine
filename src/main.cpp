#include "engine.h"
#include <iostream>

void main(void) {
	
	Engine engine{};

	if (!engine.init()) {
		std::cerr << "Engine initialisation failed." << std::endl;
	}

	engine.run();

	engine.cleanup();
}