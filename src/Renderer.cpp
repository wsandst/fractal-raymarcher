#include "Renderer.h"

Renderer::Renderer()
{
}

Renderer::Renderer(int windowWidth, int windowHeight)
{
	this->windowWidth = windowWidth;
	this->windowHeight = windowHeight;
}

Renderer::~Renderer()
{
}

void Renderer::updateDeltatime()
{
	float currentFrame = SDL_GetTicks();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}

void Renderer::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	camera.calculateProjectionMatrix();
	camera.calculateViewMatrix();
	camera.getCornerRays();

	switch (renderType)
	{
		case Raymarcher: drawPathTracer(); break;
		case Rasterizer: drawRasterizer(); break;
	}

	SDL_GL_SwapWindow(window);
}

void Renderer::drawRasterizer()
{
	rasterizerShader.use();

	for (auto& it : geometryVBOs) { //Draw chunks
		glm::mat4 matrix = glm::mat4(camera.getProjectionMatrix() * camera.getViewMatrix() * it.translation);
		rasterizerShader.setMat4("matrix", matrix);
		glBindVertexArray(it.VAO);
		glDrawArrays(GL_TRIANGLES, 0, it.size);
	}
}

void Renderer::initRasterizer()
{
	rasterizerShader = Shader("shaders/geometry.vert", "shaders/geometry.frag");
}

void Renderer::drawPathTracer()
{
	rayMarcherComputeShader.use();

	int framebufferTexLocation = glGetUniformLocation(rayMarcherComputeShader.ID, "framebuffer");
	int skyBoxTexLocation = glGetUniformLocation(rayMarcherComputeShader.ID, "skybox");
	glUniform1i(framebufferTexLocation, 0);
	glUniform1i(skyBoxTexLocation, 1);
	//Set corner ray uniforms to give the program the view
	rayMarcherComputeShader.setVec3("eye", camera.getPosition());
	rayMarcherComputeShader.setVec3("ray00", camera.ray00);
	rayMarcherComputeShader.setVec3("ray10", camera.ray10);
	rayMarcherComputeShader.setVec3("ray01", camera.ray01);
	rayMarcherComputeShader.setVec3("ray11", camera.ray11);

	//Raymarching variables
	rayMarcherComputeShader.setFloat("BAILOUT", bailout);
	rayMarcherComputeShader.setFloat("POWER", power);
	rayMarcherComputeShader.setFloat("ITERATIONS", iterations);
	rayMarcherComputeShader.setFloat("MAX_RAY_STEPS", maxRaySteps);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureOutput);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

	 // launch compute shaders!
	glDispatchCompute((GLuint)windowWidth*MSAALevel, (GLuint)windowHeight*MSAALevel, 1);
	
	// make sure writing to image has finished before read
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glActiveTexture(GL_TEXTURE0);
	screenTextureShader.use();
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

void Renderer::init()
{
	camera = Camera(windowWidth*MSAALevel, windowHeight*MSAALevel);

	initSDL();
	initOpenGL();
	switch (renderType)
	{
	case Raymarcher: initPathTracer(); break;
	case Rasterizer: initRasterizer(); break;
	}
}

void Renderer::initOpenGL()
{
	glewExperimental = GL_TRUE;
	glewInit();
	
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.3f, 0.4f, 1.0f);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	


	//Wireframes
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	//Antialiasing
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
	glEnable(GL_MULTISAMPLE);

	glViewport(0, 0, windowWidth, windowHeight);
}

void Renderer::initSDL()
{
	if (SDL_Init(SDL_INIT_VIDEO), 0) {
		std::cout << "SDL could not initialize! SDL_Error:" << SDL_GetError() << std::endl;
	}

	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	screenResHeight = DM.h;
	screenResWidth = DM.w;

	//Antialiasing
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, MSAALevel*MSAALevel);
	glEnable(GL_MULTISAMPLE);

	window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	glContext = SDL_GL_CreateContext(window);

	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_ShowCursor(0);

	SDL_GL_SetSwapInterval(1);
}

void Renderer::initPathTracer()
{
	//Screen Texture
	//stbi_set_flip_vertically_on_load(true);
	screenTextureShader = Shader("shaders/screentexture.vert", "shaders/screentexture.frag");
	rayMarcherComputeShader = ComputeShader("shaders/raymarcher.comp");

	//Compute shader
	// dimensions of the image
	glGenTextures(1, &textureOutput);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureOutput);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth*MSAALevel, windowHeight*MSAALevel, 0, GL_RGBA, GL_FLOAT,
		NULL);
	glBindImageTexture(0, textureOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	initSkyBox();
}

void Renderer::initSkyBox()
{
	stbi_set_flip_vertically_on_load(false);
	std::vector <Texture> skyBoxTextures = { FileHandler::loadImage("skybox/front.tga"), FileHandler::loadImage("skybox/back.tga"), FileHandler::loadImage("skybox/up.tga"), FileHandler::loadImage("skybox/down.tga"),
		FileHandler::loadImage("skybox/right.tga"), FileHandler::loadImage("skybox/left.tga") };
	glGenTextures(1, &skyboxTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

	for (unsigned int i = 0; i < skyBoxTextures.size(); i++)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_SRGB, skyBoxTextures[i].width, skyBoxTextures[i].height, 0, GL_RGB, GL_UNSIGNED_BYTE, skyBoxTextures[i].data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Renderer::requestShaderReload()
{
	switch (renderType)
	{
	case Raymarcher: rayMarcherComputeShader.reload(); break;
	case Rasterizer: rasterizerShader.reload(); break;
	}
}

void Renderer::close()
{
	SDL_Quit();
}

void Renderer::loadVBOs(std::vector<Mesh>& meshes)
{
	for (auto &mesh : meshes)
	{
		geometryVBOs.push_back(GeometryVBO(mesh.pos, mesh.vertices));
	}
}

void Renderer::toggleFullscreen() {

	isFullScreen = !isFullScreen;

	if (isFullScreen)
	{
		minimizedWidth = windowWidth;
		minimizedHeight = windowHeight;
		SDL_SetWindowSize(window, screenResWidth, screenResHeight);
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		windowWidth = screenResWidth;
		windowHeight = screenResHeight;

	}
	else
	{
		SDL_SetWindowFullscreen(window, 0);
		SDL_SetWindowSize(window, minimizedWidth, minimizedHeight);
		windowWidth = minimizedWidth;
		windowHeight = minimizedHeight;
	}

	updateResolution();
}

void Renderer::updateResolution()
{

	glViewport(0, 0, windowWidth, windowHeight);

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, textureOutput);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth*MSAALevel, windowHeight*MSAALevel, 0, GL_RGBA, GL_FLOAT,
		NULL);
	
	camera.windowWidth = windowWidth*MSAALevel;
	camera.windowHeight = windowHeight*MSAALevel;
	render();

}

void Renderer::resizeWindow(int width, int height)
{
	windowWidth = width;
	windowHeight = height;
	updateResolution();
}
