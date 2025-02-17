#pragma once
#include <print>
#include <exception>
#include <vector>
#include <span>
#include <ranges>
#include <algorithm>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace adttil
{
    inline void glfw_error_callback(int error, const char* description)
    {
        std::println(stderr, "GLFW Error {}: {}", error, description);
    }

    inline void check_vk_result(VkResult err)
    {
        if (err == 0)
        {
            return;
        }
        std::println(stderr, "[vulkan] Error: VkResult = {}\n", std::to_underlying(err));
        if (err < 0)
        {
            throw std::exception{ "vk error" };
        }
    }

    template<class...Types>
    inline void print_and_throw(const std::format_string<Types...> msg_fmt, Types&&...args)
    {
        std::println(msg_fmt, std::forward<Types>(args)...);
        throw std::exception{};
    }

    class NoMoveable
    {
    public:
        constexpr NoMoveable() = default;
        NoMoveable(NoMoveable&&) = delete;
    };

    class Window : NoMoveable
    {
    public:
        Window()
        {
            glfwSetErrorCallback(glfw_error_callback);
            if (!glfwInit())
            {
                print_and_throw("glfw init faild");
            }

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            window_ = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+Vulkan example", nullptr, nullptr);
            if (!glfwVulkanSupported())
            {
                print_and_throw("glfw init faild");
            }
        }

        std::span<const char*> GetRequiredInstanceExtensions() const
        {
            uint32_t extensions_count = 0;
            const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
            return { glfw_extensions, extensions_count};
        }

        ~Window()
        {
            glfwDestroyWindow(window_);
        }

    private:
        GLFWwindow* window_;
    };

    class Instance : NoMoveable
    {
    public:
        Instance(std::span<const char*> extensions, const VkAllocationCallbacks* allocator = nullptr)
        : allocator_{ allocator }
        {
            VkResult err;

            VkInstanceCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        
            // Enumerate available extensions
            uint32_t properties_count;
            std::vector<VkExtensionProperties> properties;
            vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
            properties.resize(properties_count);
            err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data());
            check_vk_result(err);
        
            const auto available_extensions = properties
                | std::views::transform([](const VkExtensionProperties& p){ return p.extensionName; });

            auto instance_extensions = extensions | std::ranges::to<std::vector>();

            // Enable required extensions
            if (std::ranges::contains(available_extensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
                instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
            if (std::ranges::contains(available_extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
            {
                instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
                create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            }
#endif
        
            // Enabling validation layers
#ifdef VULKAN_DEBUG
            const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
            create_info.enabledLayerCount = 1;
            create_info.ppEnabledLayerNames = layers;
            instance_extensions.push_back("VK_EXT_debug_report");
#endif
            for (auto&& exten : instance_extensions)
            {
                std::println("{}", exten);
            }
            
            // Create Vulkan Instance
            create_info.enabledExtensionCount = (uint32_t)instance_extensions.size();
            create_info.ppEnabledExtensionNames = instance_extensions.data();
            err = vkCreateInstance(&create_info, allocator_, &instance_);
            check_vk_result(err);
        
            // Setup the debug report callback
#ifdef VULKAN_DEBUG
            auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
            IM_ASSERT(vkCreateDebugReportCallbackEXT != nullptr);
            VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
            debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debug_report_ci.pfnCallback = debug_report;
            debug_report_ci.pUserData = nullptr;
            err = vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
            check_vk_result(err);
#endif
        }

        std::vector<VkPhysicalDevice> enumerate_physical_devices() const
        {
            uint32_t gpu_count;
            VkResult err = vkEnumeratePhysicalDevices(instance_, &gpu_count, nullptr);
            check_vk_result(err);
        
            std::vector<VkPhysicalDevice> gpus;
            gpus.resize(gpu_count);
            err = vkEnumeratePhysicalDevices(instance_, &gpu_count, gpus.data());
            check_vk_result(err);
        }

        const VkAllocationCallbacks* allocator() const noexcept
        {
            return allocator_;
        }

        ~Instance() noexcept
        {
            vkDestroyInstance(instance_, allocator_);
        }
        
    private:
        const VkAllocationCallbacks* allocator_ = nullptr;
        VkInstance instance_ = {};
    };

    class Device : NoMoveable
    {
    public:
        Device(std::span<const char*> extensions, const VkAllocationCallbacks* allocator = nullptr)
        : instance_{ extensions, allocator }
        {
            {
                uint32_t count;
                vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, nullptr);
                std::vector<VkQueueFamilyProperties> queues(count);
                vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, queues.data());
                auto iter = std::ranges::find_if(queues, [](const VkQueueFamilyProperties& p){ 
                    return p.queueFlags & VK_QUEUE_GRAPHICS_BIT;
                });
                if(iter == queues.end())
                {
                    print_and_throw("graphic queue not find");
                }

                std::vector<const char*> device_extensions{ 
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME 
                };
                // Enumerate physical device extension
                uint32_t properties_count;
                vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &properties_count, nullptr);
                std::vector<VkExtensionProperties> properties(properties_count);
                vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &properties_count, properties.data());
        #ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
                if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
                    device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        #endif
        
                const float queue_priority[] = { 1.0f };
                VkDeviceQueueCreateInfo queue_info[1] = {};
                queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info[0].queueFamilyIndex = queue_family_;
                queue_info[0].queueCount = 1;
                queue_info[0].pQueuePriorities = queue_priority;
                VkDeviceCreateInfo create_info = {};
                create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
                create_info.pQueueCreateInfos = queue_info;
                create_info.enabledExtensionCount = (uint32_t)device_extensions.size();
                create_info.ppEnabledExtensionNames = device_extensions.data();
                VkResult err = vkCreateDevice(physical_device_, &create_info, allocator, &device_);
                check_vk_result(err);
                vkGetDeviceQueue(device_, queue_family_, 0, &queue_);
            }
        }

        const VkAllocationCallbacks* allocator() const noexcept
        {
            return instance_.allocator();
        }

        operator VkDevice()const noexcept
        {
            return device_;
        }

        ~Device() noexcept
        {
            vkDestroyDevice(device_, allocator());
        }

    private:
        static VkPhysicalDevice select_physical_device(const Instance& instance)
        {
            auto gpus = instance.enumerate_physical_devices();
        
            // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
            // most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
            // dedicated GPUs) is out of scope of this sample.
            for (VkPhysicalDevice& device : gpus)
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);
                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                    return device;
            }
        
            // Use first GPU (Integrated) is a Discrete one is not available.
            if (gpus.size() > 0)
                return gpus[0];
            return VK_NULL_HANDLE;
        }

        Instance instance_;
        VkPhysicalDevice physical_device_ = select_physical_device(instance_);
        std::uint32_t queue_family_;
        VkDevice device_;
        VkQueue queue_;
    };

    class DescriptorPool : NoMoveable
    {
    public:
        DescriptorPool(std::span<const char*> extensions, const VkAllocationCallbacks* allocator = nullptr)
        : device_{ extensions, allocator }
        {
            VkDescriptorPoolSize pool_sizes[] =
            {
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
            };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1;
            pool_info.poolSizeCount = (std::uint32_t)std::ranges::size(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            check_vk_result(vkCreateDescriptorPool(device_, &pool_info, allocator, &pool_));
        }

        const VkAllocationCallbacks* allocator() const noexcept
        {
            return device_.allocator();
        }

        ~DescriptorPool() noexcept
        {
            vkDestroyDescriptorPool(device_, pool_, allocator());
        }

    private:
        Device device_;
        VkDescriptorPool pool_;
    };
}