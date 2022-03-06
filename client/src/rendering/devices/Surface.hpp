#pragma once

#include "Instance.hpp"

class Surface {
  public:
    Surface(Instance &instance, GLFWwindow *window,
            const VkAllocationCallbacks *pAllocator = nullptr)
        : instance(instance) {
        if (glfwCreateWindowSurface(instance.get(), window, pAllocator,
                                    &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    ~Surface() { vkDestroySurfaceKHR(instance.get(), surface, pAllocator); }

    VkSurfaceKHR &get() { return surface; }

  private:
    Instance &instance;
    const VkAllocationCallbacks *pAllocator;

    VkSurfaceKHR surface;
};
