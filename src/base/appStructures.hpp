#pragma once

#include "appUtils.hpp"

namespace NSM {

    // application surface format information structure
    //struct SurfaceFormat : public std::enable_shared_from_this<SurfaceFormat> {
    struct SurfaceFormat {
        vk::Format colorFormat = {};
        vk::Format depthFormat = {};
        vk::Format stencilFormat = {};
        vk::ColorSpaceKHR colorSpace = {};
        vk::FormatProperties colorFormatProperties = {};
    };

    // framebuffer with command buffer and fence
    struct Framebuffer : public std::enable_shared_from_this<Framebuffer> {
        vk::Framebuffer frameBuffer = {};
        vk::CommandBuffer commandBuffer = {}; // terminal command (barrier)
        vk::Fence waitFence = {};
        vk::Semaphore semaphore = {};
    };

    // vertex layout
    struct VertexLayout : public std::enable_shared_from_this<VertexLayout> {
        std::vector<vk::VertexInputBindingDescription> inputBindings = {};
        std::vector<vk::VertexInputAttributeDescription> inputAttributes = {};
    };

    // context for rendering (can be switched)
    struct GraphicsContext : public std::enable_shared_from_this<GraphicsContext> {
		std::shared_ptr<radx::Device> device;  // used device by context
		uint32_t queueFamilyIndex = 0;
		vk::Queue queue = {};
        vk::SwapchainKHR swapchain = {};          // swapchain state
		vk::CommandPool commandPool = {};
        vk::Pipeline pipeline = {};               // current pipeline
        vk::PipelineLayout pipelineLayout = {};
        vk::PipelineCache pipelineCache = {};
        vk::DescriptorPool descriptorPool = {};   // current descriptor pool
        vk::RenderPass renderpass = {};
        std::vector<vk::DescriptorSet> descriptorSets = {}; // descriptor sets
        std::vector<Framebuffer> framebuffers = {};         // swapchain framebuffers
    };
};
