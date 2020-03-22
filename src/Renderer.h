#pragma once
#define GLEW_STATIC

#include "GL/glew.h"

#include "SDL.h"
#include "SDL_opengl.h"

#include "glm/glm.hpp"

#include "Shader.h"
#include "ComputeShader.h"
#include "Camera.h"
#include "VBOWrapper.h"
#include "Mesh.h"

#include <vector>
#include <unordered_map>

class Renderer {
public:
	const char* windowTitle = "OpenGL Raytracer using Compute Shaders";
	int windowWidth = 1280, windowHeight = 720;
	int minimizedWidth = 1280, minimizedHeight = 720;
	int screenResHeight, screenResWidth;
	bool isFullScreen = false;
	bool borderlessFullScreen = false;
	const int MSAALevel = 2; //MS Antialiasing: 1 for none, 2 for 4x, 4 for 16x

	//Raymarching variables
	float bailout = 2.0, power = 8.0, iterations = 400, maxRaySteps = 500;

	enum RenderType {Raymarcher, Rasterizer};
	RenderType renderType = Raymarcher;

    float deltaTime = 0.0f;
    Camera camera;

    Renderer();

	Renderer( int windowWidth, int windowHeight);

    ~Renderer();

    void updateDeltatime();

    void render();

	void drawRasterizer();

	void initRasterizer();

	//Functions related to drawing a screen texture from a byte array
	void drawPathTracer();

    void updateScreenTexture(unsigned char *byteArray, int width = 1024, int height = 1024);

    void initPathTracer();

	void initSkyBox();

    void init();

    void close();

	void loadVBOs(std::vector<Mesh>& meshes);

	void toggleFullscreen();

	void updateResolution();

	void resizeWindow(int width, int height);

	void requestShaderReload();

private:
    Shader screenTextureShader, rasterizerShader;
	ComputeShader rayMarcherComputeShader;
    SDL_Window *window;
	std::vector<GeometryVBO> geometryVBOs;

    SDL_GLContext glContext;
    unsigned int VAO, VBO, textureOutput, skyboxTex;
    float lastFrame = 0.0f;

    void initOpenGL();

    void initSDL();

};

