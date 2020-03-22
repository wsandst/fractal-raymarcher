#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

#define NOMINMAX
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "stb_image_write.h"

#include "Renderer.h"
#include "InputHandler.h"

const int windowWidth = 1280;
const int windowHeight = 720;

int main(int argc, char** argv) 
{
	//Renderer setup
	bool quit = false;

	Renderer renderer(windowWidth, windowHeight);
	InputHandler inputHandler(renderer, renderer.camera);

	int frameCount = 0;
	
	renderer.renderType = renderer.Raymarcher;
	renderer.init();

	auto begin = std::chrono::system_clock::now();

	//Main render/event loop
	while (!quit) {
		if (inputHandler.windowContext) {
			renderer.render();

			//FPS counter
			auto now = std::chrono::system_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - begin).count() > 2000)
			{
				std::cout << "Frame took " << renderer.deltaTime << " ms (" << 1000 / renderer.deltaTime << " fps)" << "\n";
				begin = std::chrono::system_clock::now();
			}

			frameCount++;
		}
		if (inputHandler.exit)
		{
			quit = true;
		}
		inputHandler.handleInput(renderer.deltaTime);
		renderer.updateDeltatime();
	}

	renderer.close();
	return 0;
}