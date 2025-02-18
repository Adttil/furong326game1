#pragma once
#include <print>
#include <exception>
#include <vector>
#include <span>
#include <ranges>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
//#include <imgui.h>

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
        std::println(/*stderr, */"[vulkan] Error: VkResult = {}\n", std::to_underlying(err));
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

    template<class TOn, class F>
    class OptianalGuard
    {
    public:
        constexpr OptianalGuard(TOn& on, F&& f)
        : on_{on}
        , fn_{ std::move(f) }
        { }

        constexpr ~OptianalGuard()noexcept
        {
            if(on_) fn_();
        }

    private:
        TOn& on_;
        F fn_;
    };

    class NoMoveable
    {
    public:
        constexpr NoMoveable() = default;
        NoMoveable(NoMoveable&&) = delete;
    };

    class Renderer : NoMoveable
    {
    public:
        Renderer(const VkAllocationCallbacks* allocator = nullptr)
        : allocator_{ allocator }
        {
            glfwSetErrorCallback(glfw_error_callback);
            if (!glfwInit())
            {
                print_and_throw("glfw init faild");
            }

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            window_ = glfwCreateWindow(1280, 720, "furong326game1", nullptr, nullptr);
            if (!glfwVulkanSupported())
            {
                print_and_throw("glfw init faild");
            }

            VkResult result;

            set_and_check(result, create_instance());
            OptianalGuard _{ result, [&]{ destroy_instance(); } };

            set_and_check(result, create_device());
            OptianalGuard _{ result, [&]{ destroy_device(); } };

            set_and_check(result, create_descriptor_pool());
            OptianalGuard _{ result, [&]{ destroy_descriptor_pool(); } };

            set_and_check(result, create_surface());
            OptianalGuard _{ result, [&]{ destroy_surface(); } };

            set_and_check(result, create_swapchain());
            OptianalGuard _{ result, [&]{ destroy_swapchain(); } };

            set_and_check(result, create_render_pass());
            OptianalGuard _{ result, [&]{ destroy_render_pass(); } };
        }

        ~Renderer() noexcept
        {
            destroy_render_pass();
            destroy_swapchain();
            destroy_surface();
            destroy_descriptor_pool();
            destroy_device();
            destroy_instance();

            glfwDestroyWindow(window_);
            glfwTerminate();
        }

        bool should_close() const
        {
            return glfwWindowShouldClose(window_);
        }

        void poll_events() const
        {
            glfwPollEvents();
        }

    private:

        VkResult create_instance()
        {
            VkInstanceCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        
            // Enumerate available extensions
            uint32_t properties_count;
            std::vector<VkExtensionProperties> properties;
            vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
            properties.resize(properties_count);
            VkResult err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data());
            if(err) return err;
        
            const auto available_extensions = properties
                | std::views::transform([](const VkExtensionProperties& p){ return p.extensionName; });

            uint32_t extensions_count = 0;
            auto instance_extensions = std::span<const char*>{ glfwGetRequiredInstanceExtensions(&extensions_count), extensions_count }
                | std::ranges::to<std::vector>();

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
            return vkCreateInstance(&create_info, allocator_, &instance_);
        }

        VkResult create_device()
        {
            physical_device_ = select_physical_device(instance_);

            uint32_t count;
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, nullptr);
            std::vector<VkQueueFamilyProperties> queues(count);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, queues.data());
            
            auto iter = std::ranges::find_if(queues | std::views::enumerate, [](const auto& pair){
                auto[i, p] = pair; 
                return p.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            });
            if(iter.index() >= count)
            {
                print_and_throw("graphic queue not find");
            }
            queue_family_ = iter.index();

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
            VkResult err = vkCreateDevice(physical_device_, &create_info, allocator_, &device_);
            if(err) return err;
            vkGetDeviceQueue(device_, queue_family_, 0, &queue_);
            return err;
        }

        VkResult create_descriptor_pool()
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
            return vkCreateDescriptorPool(device_, &pool_info, allocator_, &descriptor_pool_);
        }

        VkResult create_surface()
        {
            VkResult err = glfwCreateWindowSurface(instance_, window_, allocator_, &surface_);
            if(err) return err;
            VkBool32 res;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, queue_family_, surface_, &res);
            if (res != VK_TRUE)
            {
                vkDestroySurfaceKHR(instance_, surface_, allocator_);
                print_and_throw("no WSI support on physical device");
            }

            surface_format_ = select_surface_format();
            present_mode_ = select_present_mode();
            return err;
        }

        VkResult create_swapchain(VkSwapchainKHR old_swapchain = nullptr)
        {
            int w, h;
            glfwGetFramebufferSize(window_, &w, &h);

            int min_image_count = 2;
            if(min_image_count == 0)
            {
                min_image_count = min_image_count_by_present_mode(present_mode_);
            }

            VkSwapchainCreateInfoKHR info = {};
            info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            info.surface = surface_;
            info.minImageCount = min_image_count;
            info.imageFormat = surface_format_.format;
            info.imageColorSpace = surface_format_.colorSpace;
            info.imageArrayLayers = 1;
            info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
            info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            info.presentMode = present_mode_;
            info.clipped = VK_TRUE;
            info.oldSwapchain = old_swapchain;
            VkSurfaceCapabilitiesKHR cap;
            VkResult err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &cap);
            check_vk_result(err);
            if (info.minImageCount < cap.minImageCount)
                info.minImageCount = cap.minImageCount;
            else if (cap.maxImageCount != 0 && info.minImageCount > cap.maxImageCount)
                info.minImageCount = cap.maxImageCount;

            if (cap.currentExtent.width == 0xffffffff)
            {
                info.imageExtent.width = width_ = w;
                info.imageExtent.height = height_ = h;
            }
            else
            {
                info.imageExtent.width = width_ = cap.currentExtent.width;
                info.imageExtent.height = height_ = cap.currentExtent.height;
            }
            err = vkCreateSwapchainKHR(device_, &info, allocator_, &swapchain_);
            if(err) return err;
            
            err = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count_, nullptr);
            //todo...not safe
            check_vk_result(err);
            // IM_ASSERT(wd->ImageCount >= min_image_count);
            // IM_ASSERT(wd->ImageCount < IM_ARRAYSIZE(backbuffers));
            err = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count_, backbuffers_);
            check_vk_result(err);

            return err;

            // IM_ASSERT(wd->Frames == nullptr);
            // wd->Frames = (ImGui_ImplVulkanH_Frame*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_Frame) * wd->ImageCount);
            // wd->FrameSemaphores = (ImGui_ImplVulkanH_FrameSemaphores*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_FrameSemaphores) * wd->ImageCount);
            // memset(wd->Frames, 0, sizeof(wd->Frames[0]) * wd->ImageCount);
            // memset(wd->FrameSemaphores, 0, sizeof(wd->FrameSemaphores[0]) * wd->ImageCount);
            // for (uint32_t i = 0; i < wd->ImageCount; i++)
            //     wd->Frames[i].Backbuffer = backbuffers[i];
        }

        VkResult create_render_pass()
        {
            VkAttachmentDescription attachment = {};
            attachment.format = surface_format_.format;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = true ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            VkAttachmentReference color_attachment = {};
            color_attachment.attachment = 0;
            color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_attachment;
            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            VkRenderPassCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            info.attachmentCount = 1;
            info.pAttachments = &attachment;
            info.subpassCount = 1;
            info.pSubpasses = &subpass;
            info.dependencyCount = 1;
            info.pDependencies = &dependency;
            return vkCreateRenderPass(device_, &info, allocator_, &render_pass_);
        }

        void destroy_render_pass() noexcept
        {
            vkDestroyRenderPass(device_, render_pass_, allocator_);
        }

        void destroy_swapchain() noexcept
        {
            vkDestroySwapchainKHR(device_, swapchain_, allocator_);
        }

        void destroy_surface() noexcept
        {
            vkDestroySurfaceKHR(instance_, surface_, allocator_);
        }

        void destroy_descriptor_pool() noexcept
        {
            vkDestroyDescriptorPool(device_, descriptor_pool_, allocator_);
        }

        void destroy_device() noexcept
        {
            vkDestroyDevice(device_, allocator_);
        }

        void destroy_instance() noexcept
        {
            vkDestroyInstance(instance_, allocator_);
        }

        static void set_and_check(VkResult& result, VkResult new_value)
        {
            result = new_value;
            check_vk_result(result);
        }
        
        static VkPhysicalDevice select_physical_device(VkInstance instance)
        {
            uint32_t gpu_count;
            VkResult err = vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
            check_vk_result(err);
        
            std::vector<VkPhysicalDevice> gpus(gpu_count);
            err = vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());
            check_vk_result(err);
        
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
        
        VkSurfaceFormatKHR select_surface_format() const
        {
            const VkFormat request_formats[] = { 
                VK_FORMAT_B8G8R8A8_UNORM, 
                VK_FORMAT_R8G8B8A8_UNORM, 
                VK_FORMAT_B8G8R8_UNORM, 
                VK_FORMAT_R8G8B8_UNORM 
            };
            const VkColorSpaceKHR request_color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            //IM_ASSERT(g_FunctionsLoaded && "Need to call ImGui_ImplVulkan_LoadFunctions() if IMGUI_IMPL_VULKAN_NO_PROTOTYPES or VK_NO_PROTOTYPES are set!");
        
            // Per Spec Format and View Format are expected to be the same unless VK_IMAGE_CREATE_MUTABLE_BIT was set at image creation
            // Assuming that the default behavior is without setting this bit, there is no need for separate Swapchain image and image view format
            // Additionally several new color spaces were introduced with Vulkan Spec v1.0.40,
            // hence we must make sure that a format with the mostly available color space, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, is found and used.
            uint32_t avail_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &avail_count, nullptr);
            std::vector<VkSurfaceFormatKHR> avail_format(avail_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &avail_count, avail_format.data());
        
            // First check if only one format, VK_FORMAT_UNDEFINED, is available, which would imply that any format is available
            if (avail_count == 1)
            {
                if (avail_format[0].format == VK_FORMAT_UNDEFINED)
                {
                    VkSurfaceFormatKHR ret;
                    ret.format = request_formats[0];
                    ret.colorSpace = request_color_space;
                    return ret;
                }
                else
                {
                    // No point in searching another format
                    return avail_format[0];
                }
            }
            else
            {
                // Request several formats, the first found will be used
                for (const VkFormat request_format : request_formats)
                    for (uint32_t avail_i = 0; avail_i < avail_count; avail_i++)
                        if (avail_format[avail_i].format == request_format && avail_format[avail_i].colorSpace == request_color_space)
                            return avail_format[avail_i];
            
                // If none of the requested image formats could be found, use the first available
                return avail_format[0];
            }
        }

        VkPresentModeKHR select_present_mode() const
        {
#define IMGUI_UNLIMITED_FRAME_RATE
#ifdef IMGUI_UNLIMITED_FRAME_RATE
            VkPresentModeKHR request_present_modes[] = {
                VK_PRESENT_MODE_MAILBOX_KHR, 
                VK_PRESENT_MODE_IMMEDIATE_KHR, 
                VK_PRESENT_MODE_FIFO_KHR 
            };
#else
            VkPresentModeKHR request_present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
            //IM_ASSERT(g_FunctionsLoaded && "Need to call ImGui_ImplVulkan_LoadFunctions() if IMGUI_IMPL_VULKAN_NO_PROTOTYPES or VK_NO_PROTOTYPES are set!");

            // Request a certain mode and confirm that it is available. If not use VK_PRESENT_MODE_FIFO_KHR which is mandatory
            uint32_t avail_count = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &avail_count, nullptr);
            std::vector<VkPresentModeKHR> avail_modes(avail_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &avail_count, avail_modes.data());
            //for (uint32_t avail_i = 0; avail_i < avail_count; avail_i++)
            //    printf("[vulkan] avail_modes[%d] = %d\n", avail_i, avail_modes[avail_i]);

            for (const VkPresentModeKHR request_present_mode : request_present_modes)
                for (uint32_t avail_i = 0; avail_i < avail_count; avail_i++)
                    if (request_present_mode == avail_modes[avail_i])
                        return request_present_mode;

            return VK_PRESENT_MODE_FIFO_KHR; // Always available
        }

        static int min_image_count_by_present_mode(VkPresentModeKHR present_mode)
        {
            if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return 3;
            if (present_mode == VK_PRESENT_MODE_FIFO_KHR || present_mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
                return 2;
            if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                return 1;
            return 1;
        }

        GLFWwindow* window_;

        const VkAllocationCallbacks* allocator_ = nullptr;
        VkInstance instance_;

        VkPhysicalDevice physical_device_;
        uint32_t queue_family_;
        VkDevice device_;
        VkQueue queue_;

        VkDescriptorPool descriptor_pool_;

        VkSurfaceKHR surface_;
        VkSurfaceFormatKHR surface_format_;
        VkPresentModeKHR present_mode_;

        uint32_t width_;
        uint32_t height_;
        VkSwapchainKHR swapchain_;
        uint32_t image_count_;
        VkImage backbuffers_[16] = {};

        VkRenderPass render_pass_;
    };
}