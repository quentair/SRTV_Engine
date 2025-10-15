#include "engine.h"
#include <iostream>

int main(int argc, char* argv[]) {
	
	Engine engine{};

	if (!engine.init()) {
		std::cerr << "Engine initialisation failed." << std::endl;
	}

	engine.run();

	engine.cleanup();

	return 0;
}