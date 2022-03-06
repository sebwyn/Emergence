#pragma once

#include "30_multisampling.hpp"

class Renderer {
  public:
    Renderer(){

        /*glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                               nullptr);

        //window must be defined
        initVulkan();

        std::cout << extensionCount << " extensions supported\n";
        */
    }

    ~Renderer(){
        /*
        destroyVulkan();

        glfwDestroyWindow(window);
        glfwTerminate();
        */
    }

    bool update() {

        application.run();
        /*
        if (glfwWindowShouldClose(window)) {
            return false;
        }

        glfwPollEvents();

        return true;
        */

        return false;
    }


  private:
    
    /*void initVulkan(){
        instance = new VulkanState(window);
        instance->printExtensions();
    }

    void destroyVulkan(){
        delete instance;
    }   
    */

    //GLFWwindow *window;

    //{VulkanState* instance;
    HelloTriangleApplication application;
};
