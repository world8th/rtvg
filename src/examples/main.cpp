//#pragma once

//#define VMA_IMPLEMENTATION
//#define RADX_IMPLEMENTATION

#undef small
#define small char
#undef small
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "../base/appRenderer.hpp"
#undef small

// initial renderer
//const uint32_t canvasWidth = 640, canvasHeight = 360; // 
const uint32_t canvasWidth = 1920, canvasHeight = 1080;
const bool enableSuperSampling = true;
std::shared_ptr<rnd::Renderer> renderer = nullptr;

// main program with handling
int main(int argc, char** argv) {

    // initialize API
    if (!glfwInit() || !glfwVulkanSupported()) exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // initialize renderer
    renderer = std::make_shared<rnd::Renderer>();
    renderer->Arguments(argc, argv);
    renderer->Init(canvasWidth, canvasHeight, enableSuperSampling); // init GLFW window there
    renderer->InitRayTracing();
    renderer->InitPipeline();
    renderer->InitCommands();


    // looping rendering
    while (!glfwWindowShouldClose(renderer->window)) {
        glfwPollEvents();
        renderer->Draw();
        renderer->HandleData();
    };

    glfwDestroyWindow(renderer->window);
    glfwTerminate();
    return 0;
};
