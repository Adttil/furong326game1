#pragma once
#include <print>
#include <exception>
#include <vector>
#include <span>
#include <ranges>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

#include <3rd party/stb_image/stb_image.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <senluo/geo.hpp>

namespace adttil
{
    using Vec2 = senluo::geo::vec<2>;
    using Vec3 = senluo::geo::vec<3>;
    using Vec4 = senluo::geo::vec<4>;

    using Coord2 = senluo::geo::vec<2, size_t>;
    using Coord3 = senluo::geo::vec<3, size_t>;
    using Coord4 = senluo::geo::vec<4, size_t>;

    using Color32 = senluo::geo::vec<4, unsigned char>;

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

            set_and_check(result, create_frames());
            OptianalGuard _{ result, [&]{ destroy_frames(); } };

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

            // // Setup Dear ImGui style
             ImGui::StyleColorsDark();
            // //ImGui::StyleColorsLight();

            // Setup Platform/Renderer backends
            ImGui_ImplGlfw_InitForVulkan(window_, true);
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = instance_;
            init_info.PhysicalDevice = physical_device_;
            init_info.Device = device_;
            init_info.QueueFamily = queue_family_;
            init_info.Queue = queue_;
            init_info.PipelineCache = nullptr;
            init_info.DescriptorPool = descriptor_pool_;
            init_info.Subpass = 0;
            init_info.MinImageCount = 2;
            init_info.ImageCount = image_count_;
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            init_info.Allocator = allocator_;
            init_info.CheckVkResultFn = check_vk_result;
            ImGui_ImplVulkan_Init(&init_info, render_pass_);
        }

        ~Renderer() noexcept
        {
            vkDeviceWaitIdle(device_);

            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            destroy_frames();
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

        void new_frame() const
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        void frame_render(ImVec4 clear_color)
        {
            ImGui::Render();
            ImDrawData* draw_data = ImGui::GetDrawData();

            VkResult err;
        
            VkSemaphore image_acquired_semaphore  = frames_[semaphore_index_].image_acquired_semaphore;
            VkSemaphore render_complete_semaphore = frames_[semaphore_index_].render_complete_semaphore;
            err = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &frame_index_);
            if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
            {
                //g_SwapChainRebuild = true;
                return;
            }
            check_vk_result(err);
        
            auto& fd = frames_[frame_index_];
            {
                err = vkWaitForFences(device_, 1, &fd.fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
                check_vk_result(err);
            
                err = vkResetFences(device_, 1, &fd.fence);
                check_vk_result(err);
            }
            {
                err = vkResetCommandPool(device_, fd.command_pool, 0);
                check_vk_result(err);
                VkCommandBufferBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                err = vkBeginCommandBuffer(fd.command_buffer, &info);
                check_vk_result(err);
            }
            {
                VkRenderPassBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                info.renderPass = render_pass_;
                info.framebuffer = fd.framebuffer;
                info.renderArea.extent.width = width_;
                info.renderArea.extent.height = height_;
                info.clearValueCount = 1;

                VkClearValue clear_value = {};
                clear_value.color.float32[0] = clear_color.x * clear_color.w;
                clear_value.color.float32[1] = clear_color.y * clear_color.w;
                clear_value.color.float32[2] = clear_color.z * clear_color.w;
                clear_value.color.float32[3] = clear_color.w;
                info.pClearValues = &clear_value;

                vkCmdBeginRenderPass(fd.command_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
            }
        
            // Record dear imgui primitives into command buffer
            ImGui_ImplVulkan_RenderDrawData(draw_data, fd.command_buffer);
        
            // Submit command buffer
            vkCmdEndRenderPass(fd.command_buffer);
            {
                VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                VkSubmitInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                info.waitSemaphoreCount = 1;
                info.pWaitSemaphores = &image_acquired_semaphore;
                info.pWaitDstStageMask = &wait_stage;
                info.commandBufferCount = 1;
                info.pCommandBuffers = &fd.command_buffer;
                info.signalSemaphoreCount = 1;
                info.pSignalSemaphores = &render_complete_semaphore;
            
                err = vkEndCommandBuffer(fd.command_buffer);
                check_vk_result(err);
                err = vkQueueSubmit(queue_, 1, &info, fd.fence);
                check_vk_result(err);
            }

            //present
            VkPresentInfoKHR info = {};
            info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &render_complete_semaphore;
            info.swapchainCount = 1;
            info.pSwapchains = &swapchain_;
            info.pImageIndices = &frame_index_;
            err = vkQueuePresentKHR(queue_, &info);
            if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
            {
                //g_SwapChainRebuild = true;
                return;
            }
            check_vk_result(err);
            semaphore_index_ = (semaphore_index_ + 1) % image_count_; // Now we can use the next set of semaphores
        }

    private:
        struct Frame
        {
            VkImageView     backbuffer_view;
            VkFramebuffer   framebuffer;
            VkCommandPool   command_pool;
            VkCommandBuffer command_buffer;
            VkFence         fence;
            
            VkSemaphore     image_acquired_semaphore;
            VkSemaphore     render_complete_semaphore;
        };


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

        VkResult create_frames()
        {
            VkResult result = VK_SUCCESS;
            OptianalGuard _{ result, [&]{ destroy_frames(); } };

            VkImageViewCreateInfo view_info = {};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = surface_format_.format;
            view_info.components.r = VK_COMPONENT_SWIZZLE_R;
            view_info.components.g = VK_COMPONENT_SWIZZLE_G;
            view_info.components.b = VK_COMPONENT_SWIZZLE_B;
            view_info.components.a = VK_COMPONENT_SWIZZLE_A;
            VkImageSubresourceRange image_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            view_info.subresourceRange = image_range;

            VkFramebufferCreateInfo buff_info = {};
            buff_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            buff_info.renderPass = render_pass_;
            buff_info.attachmentCount = 1;
            buff_info.width = width_;
            buff_info.height = height_;
            buff_info.layers = 1;
            
            VkSemaphoreCreateInfo sem_info = {};
            sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            
            frames_.reserve(image_count_);
            for(const VkImage backbuffer :  backbuffers_ | std::views::take(image_count_))
            {
                Frame frame;
                view_info.image = backbuffer;
                set_and_check(result, vkCreateImageView(device_, &view_info, allocator_, &frame.backbuffer_view));
                OptianalGuard _{ result, [&]{ vkDestroyImageView(device_, frame.backbuffer_view, allocator_); } };

                buff_info.pAttachments = &frame.backbuffer_view;
                set_and_check(result, vkCreateFramebuffer(device_, &buff_info, allocator_, &frame.framebuffer));
                OptianalGuard _{ result, [&]{ vkDestroyFramebuffer(device_, frame.framebuffer, allocator_); } };

                {
                    VkCommandPoolCreateInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    info.flags = 0;
                    info.queueFamilyIndex = queue_family_;
                    set_and_check(result, vkCreateCommandPool(device_, &info, allocator_, &frame.command_pool));
                }
                OptianalGuard _{ result, [&]{ vkDestroyCommandPool(device_, frame.command_pool, allocator_); } };

                {
                    VkCommandBufferAllocateInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    info.commandPool = frame.command_pool;
                    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    info.commandBufferCount = 1;
                    set_and_check(result, vkAllocateCommandBuffers(device_, &info, &frame.command_buffer));
                }
                OptianalGuard _{ result, [&]{ vkFreeCommandBuffers(device_, frame.command_pool, 1, &frame.command_buffer); } };

                {
                    VkFenceCreateInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                    set_and_check(result, vkCreateFence(device_, &info, allocator_, &frame.fence));
                }
                OptianalGuard _{ result, [&]{ vkDestroyFence(device_, frame.fence, allocator_); } };

                set_and_check(result, vkCreateSemaphore(device_, &sem_info, allocator_, &frame.image_acquired_semaphore));
                OptianalGuard _{ result, [&]{ vkDestroySemaphore(device_, frame.image_acquired_semaphore, allocator_); } };

                set_and_check(result, vkCreateSemaphore(device_, &sem_info, allocator_, &frame.render_complete_semaphore));

                frames_.push_back(frame);
            }

            return result;
        }

        void destroy_frames() noexcept
        {
            for(const auto[img_view, frame_buff, cmd_pool, cmd_buff, fence, img_acq, rnd_cpl] : frames_)
            {
                vkDestroySemaphore(device_, rnd_cpl, allocator_);
                vkDestroySemaphore(device_, img_acq, allocator_);
                vkDestroyFence(device_, fence, allocator_);
                vkFreeCommandBuffers(device_, cmd_pool, 1, &cmd_buff);
                vkDestroyCommandPool(device_, cmd_pool, allocator_);
                vkDestroyFramebuffer(device_, frame_buff, allocator_);
                vkDestroyImageView(device_, img_view, allocator_);
            }
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

        std::vector<Frame> frames_;
        uint32_t frame_index_ = 0;
        uint32_t semaphore_index_ = 0;
    };

    class BitmapView
    {
    public:
        constexpr BitmapView(Color32* data, Coord2 size, size_t row_align) noexcept
        : data_{ data }
        , size_{ size }
        , row_align_{ row_align }
        {}

        constexpr BitmapView(Color32* data, Coord2 size) noexcept
        : BitmapView{ data, size, size.x() }
        {}

        constexpr Color32* data()const noexcept
        {
            return data_;
        }

        constexpr Coord2 size()const noexcept
        {
            return size_;
        }

        constexpr size_t width()const noexcept
        {
            return size_.x();
        }

        constexpr size_t height()const noexcept
        {
            return size_.y();
        }

        constexpr size_t count()const noexcept
        {
            return width() * height();
        }

        constexpr size_t row_align()const noexcept
        {
            return row_align_;
        }

        constexpr Color32& operator[](size_t i)const noexcept
        {
            return data_[i % row_align_ + i / row_align_ * row_align_];
        }

        constexpr Color32& operator[](Coord2 coord)const noexcept
        {
            return data_[coord.x() + coord.y() * row_align_];
        }

        friend void copy(BitmapView dst, BitmapView src)
        {
            for(size_t i : std::views::iota(0uz, dst.height()))
            {
                memcpy(dst.row(i), src.row(i), dst.width() * sizeof(Color32));
            }
        }

    private:
        constexpr Color32* row(size_t i) const noexcept
        {
            return &data_[row_align_ * i];
        }

        Color32* data_;
        Coord2 size_;
        size_t row_align_;
    };

    class AnimManager
    {
    public:
        AnimManager(const char* anim_folder_path)
        {
            namespace fs = std::filesystem;

            auto files = fs::directory_iterator(anim_folder_path)
                        | std::views::filter([](auto& file){ return file.path().extension() == ".png" ;})
                        | std::views::transform([](auto& file){ return file.path().string(); })
                        | std::ranges::to<std::vector>();

            uint32_t width = 0;
            uint32_t height = 0;
            for (const auto& file : files) 
            {
                int w, h, c;
                if(not stbi_info(file.c_str(), &w, &h, &c))
                {
                    continue;
                }
                width = std::max(width, (uint32_t)w);
                height += h;
            }

            std::vector<Color32> textures_data(width * height);
            uint32_t offset = 0;
            for (const auto& file : files) 
            {
                int w, h, c;
                auto pixels = (Color32*)stbi_load(file.c_str(), &w, &h, &c, 4);
                if(not pixels)
                {
                    continue;
                }
                Coord2 size{ (size_t)w, (size_t)h };
                copy(BitmapView{ textures_data.data() + offset, size, width }, BitmapView{ pixels, size });
                offset += size.y() * width;
            }


        }

    private:
        std::unordered_map<std::string, uint32_t> textures;
        
    };
}