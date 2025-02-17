#include <print>
#include <renderer/renderer.hpp>

int main()
{
    std::println("hello");
    adttil::Window window{};
    const auto window_required_extensions = window.GetRequiredInstanceExtensions();
    adttil::DescriptorPool device{ window_required_extensions };
}