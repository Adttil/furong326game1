#include <print>

#define VULKAN_DEBUG
#include <renderer/renderer.hpp>

int main()
{    
    adttil::Renderer renderer{};
    
    while (not renderer.should_close())
    {
        renderer.poll_events();
    }
}