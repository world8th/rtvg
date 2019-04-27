//#pragma once

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define VTE_RENDERER_IMPLEMENTATION

#include "../base/appRenderer.hpp"

namespace rnd {

    const uint32_t blockWidth = 8, blockheight = 8;

     void Renderer::Arguments(int argc, char** argv) {
        args::ArgumentParser parser("This is a test rendering program.", "");
        args::HelpFlag help(parser, "help", "Available flags", { 'h', "help" });
        args::ValueFlag<int32_t> computeflag(parser, "compute-device-id", "Vulkan compute device (UNDER CONSIDERATION)", { 'c' });
        args::ValueFlag<int32_t> deviceflag(parser, "graphics-device-id", "Vulkan graphics device to use (also should support compute)", { 'g' });
        args::ValueFlag<float> scaleflag(parser, "scale", "Scaling of model object", { 's' });
        args::ValueFlag<std::string> directoryflag(parser, "directory", "Directory of resources", { 'd' });
        args::ValueFlag<std::string> shaderflag(parser, "shaders", "Used SPIR-V shader pack", { 'p' });
        args::ValueFlag<std::string> bgflag(parser, "background", "Environment background", { 'b' });
        args::ValueFlag<std::string> modelflag(parser, "model", "Model to view (planned multiple models support)", { 'm' });
        args::Flag forceCompute(parser, "force-compute", "Force enable compute shader backend", { 'F' });

        args::ValueFlag<int32_t> reflLV(parser, "reflection-level", "Level of reflections", {'R'});
        args::ValueFlag<int32_t> trnsLV(parser, "transparency-level", "Level of transparency", {'T'});
        
        try { parser.ParseCLI(argc, argv); }
        catch (args::Help) { std::cout << parser; glfwTerminate(); exit(1); };

        // read arguments
        if (deviceflag) gpuID = args::get(deviceflag);
        if (shaderflag) shaderPrefix = args::get(shaderflag);
        if (bgflag) bgTexName = args::get(bgflag);
        if (modelflag) modelInput = args::get(modelflag);
        if (scaleflag) modelScale = args::get(scaleflag);
        if (directoryflag) directory = args::get(directoryflag);
        if (help) { std::cout << parser; glfwTerminate(); } // required help or no arguments
        if (modelInput == "") { std::cerr << "No model found :(" << std::endl; glfwTerminate(); exit(1); };

        if (reflLV) reflectionLevel = args::get(reflLV);
        if (trnsLV) transparencyLevel = args::get(trnsLV);
        //if (forceCompute) enableAdvancedAcceleration = !args::get(forceCompute);
    };

     void Renderer::Init(uint32_t windowWidth, uint32_t windowHeight, bool enableSuperSampling) {
        // create GLFW window
        this->windowWidth = windowWidth, this->windowHeight = windowHeight;
        this->window = glfwCreateWindow(windowWidth, windowHeight, "vRt early test", NULL, NULL);
        if (!this->window) { glfwTerminate(); exit(EXIT_FAILURE); }

        // get DPI
        glfwGetWindowContentScale(this->window, &guiScale, nullptr);
        glfwSetWindowSize(this->window, this->realWidth = windowWidth * guiScale, this->realHeight = windowHeight * guiScale); // set real size of window

        // create vulkan and ray tracing instance
        appBase = std::make_shared<vte::ComputeFramework>();
        cameraController = std::make_shared<CameraController>();
        cameraController->canvasSize = (glm::uvec2*)&this->windowWidth;
        cameraController->eyePos = &this->eyePos;
        cameraController->upVector = &this->upVector;
        cameraController->viewVector = &this->viewVector;

        instance = appBase->createInstance();
        if (!instance) { glfwTerminate(); exit(EXIT_FAILURE); }

        // get physical devices
        //auto physicalDevices = instance.enumeratePhysicalDevices();
        //if (physicalDevices.size() < 0) { glfwTerminate(); std::cerr << "Vulkan does not supported, or driver broken." << std::endl; exit(0); }

        // choice device
        //if (gpuID >= physicalDevices.size()) { gpuID = physicalDevices.size() - 1; }
        //if (gpuID < 0 || gpuID == -1) gpuID = 0;

        // create surface and get format by physical device
		auto gpu = appBase->getPhysicalDevice(gpuID);
        appBase->createWindowSurface(this->window, this->realWidth, this->realHeight, title);
        appBase->format(appBase->getSurfaceFormat(gpu));

        // set GLFW callbacks
        glfwSetMouseButtonCallback(this->window, &Shared::MouseButtonCallback);
        glfwSetCursorPosCallback(this->window, &Shared::MouseMoveCallback);
        glfwSetKeyCallback(this->window, &Shared::KeyCallback);

        // write physical device name
        vk::PhysicalDeviceProperties devProperties = gpu.getProperties();
        std::cout << "Current Device: ";
        std::cout << devProperties.deviceName << std::endl;
        std::cout << devProperties.vendorID << std::endl;

        // create combined device object
        shaderPack = shaderPrefix + "shaders/" + getShaderDir(devProperties.vendorID);

		// create radix sort application (RadX C++)
		physicalHelper = std::make_shared<radx::PhysicalDeviceHelper>(appBase->getPhysicalDevice(0));
		device = std::make_shared<radx::Device>()->initialize(appBase->createDevice(false, shaderPack, true), physicalHelper);
		program = std::make_shared<radx::Radix>(), program->initialize(device);
		//radixSort = std::make_shared<radx::Sort<radx::Radix>>(), radixSort->initialize(device, program, elementCount);
		inputInterface = std::make_shared<radx::InputInterface>(device);
		renderpass = appBase->createRenderpass(device);


        // create image output
        const auto SuperSampling = enableSuperSampling ? 2.0 : 1.0; // super sampling image
        this->canvasWidth  = this->windowWidth  * SuperSampling;
        this->canvasHeight = this->windowHeight * SuperSampling;

    };

     void Renderer::InitCommands() {

    };


     void Renderer::InitRayTracing() {

    };


     void Renderer::InitPipeline() {
        // descriptor set bindings
        std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = { vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr) };
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = { vk::Device(*device).createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(1)) };

        // pipeline layout and cache
        auto pipelineLayout = vk::Device(*device).createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(descriptorSetLayouts.data()).setSetLayoutCount(descriptorSetLayouts.size()));
        auto descriptorSets = vk::Device(*device).allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(*device).setDescriptorSetCount(descriptorSetLayouts.size()).setPSetLayouts(descriptorSetLayouts.data()));
        drawDescriptorSets = {descriptorSets[0]};

        // create pipeline
        vk::Pipeline trianglePipeline = {};
        {
            // pipeline stages
            std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {
                vk::PipelineShaderStageCreateInfo().setModule(radx::createShaderModule(*device, radx::readBinary(shaderPack + "/output/render.vert.spv"))).setPName("main").setStage(vk::ShaderStageFlagBits::eVertex),
                vk::PipelineShaderStageCreateInfo().setModule(radx::createShaderModule(*device, radx::readBinary(shaderPack + "/output/render.frag.spv"))).setPName("main").setStage(vk::ShaderStageFlagBits::eFragment)
            };

            // blend modes per framebuffer targets
            std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {
                vk::PipelineColorBlendAttachmentState()
                .setBlendEnable(true)
                .setSrcColorBlendFactor(vk::BlendFactor::eOne).setDstColorBlendFactor(vk::BlendFactor::eZero).setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne).setDstAlphaBlendFactor(vk::BlendFactor::eZero).setAlphaBlendOp(vk::BlendOp::eAdd)
                .setColorWriteMask(vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA))
            };

            // dynamic states
            std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

            // create graphics pipeline
            auto tesselationState = vk::PipelineTessellationStateCreateInfo();
            auto vertexInputState = vk::PipelineVertexInputStateCreateInfo();
            auto rasterizartionState = vk::PipelineRasterizationStateCreateInfo();

            trianglePipeline = vk::Device(*device).createGraphicsPipeline(*device, vk::GraphicsPipelineCreateInfo()
                .setPStages(pipelineShaderStages.data()).setStageCount(pipelineShaderStages.size())
                .setFlags(vk::PipelineCreateFlagBits::eAllowDerivatives)
                .setPVertexInputState(&vertexInputState)
                .setPInputAssemblyState(&vk::PipelineInputAssemblyStateCreateInfo().setTopology(vk::PrimitiveTopology::eTriangleStrip))
                .setPViewportState(&vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1))
                .setPRasterizationState(&vk::PipelineRasterizationStateCreateInfo()
                    .setDepthClampEnable(false)
                    .setRasterizerDiscardEnable(false)
                    .setPolygonMode(vk::PolygonMode::eFill)
                    .setCullMode(vk::CullModeFlagBits::eBack)
                    .setFrontFace(vk::FrontFace::eCounterClockwise)
                    .setDepthBiasEnable(false)
                    .setDepthBiasConstantFactor(0)
                    .setDepthBiasClamp(0)
                    .setDepthBiasSlopeFactor(0)
                    .setLineWidth(1.f))
                .setPDepthStencilState(&vk::PipelineDepthStencilStateCreateInfo()
                    .setDepthTestEnable(false)
                    .setDepthWriteEnable(false)
                    .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                    .setDepthBoundsTestEnable(false)
                    .setStencilTestEnable(false))
                .setPColorBlendState(&vk::PipelineColorBlendStateCreateInfo()
                    .setLogicOpEnable(false)
                    .setLogicOp(vk::LogicOp::eClear)
                    .setPAttachments(colorBlendAttachments.data())
                    .setAttachmentCount(colorBlendAttachments.size()))
                .setLayout(pipelineLayout)
                .setRenderPass(renderpass)
                .setBasePipelineIndex(0)
                .setPMultisampleState(&vk::PipelineMultisampleStateCreateInfo().setRasterizationSamples(vk::SampleCountFlagBits::e1))
                .setPDynamicState(&vk::PipelineDynamicStateCreateInfo().setPDynamicStates(dynamicStates.data()).setDynamicStateCount(dynamicStates.size()))
                .setPTessellationState(&tesselationState));
        };

        { // write descriptors for showing texture
			// TODO: outputImage and images support

            /*vk::SamplerCreateInfo samplerInfo = {};
            samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.minFilter = vk::Filter::eLinear;
            samplerInfo.magFilter = vk::Filter::eLinear;
            samplerInfo.compareEnable = false;
            auto sampler = deviceQueue->device->logical.createSampler(samplerInfo); // create sampler
            auto image = outputImage;

            // desc texture texture
            vk::DescriptorImageInfo imageDesc = {};
            imageDesc.imageLayout = vk::ImageLayout(image->_layout);
            imageDesc.imageView = vk::ImageView(image->_imageView);
            imageDesc.sampler = sampler;

            // update descriptors
            deviceQueue->device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstBinding(0).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setPImageInfo(&imageDesc),
            }, nullptr);*/
        };

        currentContext = std::make_shared<vte::GraphicsContext>();
        { // create graphic context
            auto context = currentContext;

            // create graphics context
            context->device = device;
            context->pipeline = trianglePipeline;
			context->descriptorPool = *device;
            context->descriptorSets = drawDescriptorSets;
            context->pipelineLayout = pipelineLayout;

            // create framebuffers by size
            context->renderpass = renderpass;
            context->swapchain = appBase->createSwapchain(device);
            context->framebuffers = appBase->createSwapchainFramebuffer(device, context->swapchain, context->renderpass);
        };
    };

     void Renderer::Draw(){
        auto n_semaphore = currSemaphore;
        auto c_semaphore = int32_t((size_t(currSemaphore) + 1ull) % currentContext->framebuffers.size());
        currSemaphore = c_semaphore;

        // acquire next image where will rendered (and get semaphore when will presented finally)
        n_semaphore = (n_semaphore >= 0 ? n_semaphore : (currentContext->framebuffers.size() - 1));
        vk::Device(*currentContext->device).acquireNextImageKHR(currentContext->swapchain, std::numeric_limits<uint64_t>::max(), currentContext->framebuffers[n_semaphore].semaphore, nullptr, &currentBuffer);

        { // submit rendering (and wait presentation in device)
            std::vector<vk::ClearValue> clearValues = { vk::ClearColorValue(std::array<float,4>{0.2f, 0.2f, 0.2f, 1.0f}), vk::ClearDepthStencilValue(1.0f, 0) };
            auto renderArea = vk::Rect2D(vk::Offset2D(0, 0), appBase->size());
            auto viewport = vk::Viewport(0.0f, 0.0f, appBase->size().width, appBase->size().height, 0, 1.0f);

            // create command buffer (with rewrite)
            vk::CommandBuffer& commandBuffer = currentContext->framebuffers[n_semaphore].commandBuffer;
            if (!commandBuffer) {
                commandBuffer = vte::createCommandBuffer(*currentContext->device, *currentContext->device, false, false); // do reference of cmd buffer
                commandBuffer.beginRenderPass(vk::RenderPassBeginInfo(currentContext->renderpass, currentContext->framebuffers[currentBuffer].frameBuffer, renderArea, clearValues.size(), clearValues.data()), vk::SubpassContents::eInline);
                commandBuffer.setViewport(0, std::vector<vk::Viewport> { viewport });
                commandBuffer.setScissor(0, std::vector<vk::Rect2D> { renderArea });
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, currentContext->pipeline);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentContext->pipelineLayout, 0, currentContext->descriptorSets, nullptr);
                commandBuffer.draw(4, 1, 0, 0);
                commandBuffer.endRenderPass();
                commandBuffer.end();
            };

            // create render submission 
            std::vector<vk::Semaphore>
                waitSemaphores = { currentContext->framebuffers[n_semaphore].semaphore },
                signalSemaphores = { currentContext->framebuffers[c_semaphore].semaphore };
            std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

            // 
            auto smbi = vk::SubmitInfo()
                .setPWaitDstStageMask(waitStages.data()).setPWaitSemaphores(waitSemaphores.data()).setWaitSemaphoreCount(waitSemaphores.size())
                .setPCommandBuffers(&commandBuffer).setCommandBufferCount(1)
                .setPSignalSemaphores(signalSemaphores.data()).setSignalSemaphoreCount(signalSemaphores.size());

            // submit command once
            vte::submitCmd(*currentContext->device, *currentContext->device, { commandBuffer }, smbi);

            // delete command buffer 
            //{ currentContext->queue->device->logical.freeCommandBuffers(currentContext->queue->commandPool, { commandBuffer }); commandBuffer = nullptr; };
        };

        // present for displaying of this image
		vk::Queue(*currentContext->device).presentKHR(vk::PresentInfoKHR(
            1, &currentContext->framebuffers[c_semaphore].semaphore,
            1, &currentContext->swapchain,
            &currentBuffer, nullptr
        ));
    };


     void Renderer::HandleData(){
        auto tDiff = Shared::active.tDiff; // get computed time difference

        std::stringstream tDiffStream{ "" };
        tDiffStream << std::fixed << std::setprecision(2) << (tDiff);

        std::stringstream tFramerateStream{ "" };
        auto tFramerateStreamF = 1e3 / tDiff;
        tPastFramerateStreamF = tPastFramerateStreamF * 0.5 + tFramerateStreamF * 0.5;
        tFramerateStream << std::fixed << std::setprecision(0) << tPastFramerateStreamF;

        auto wTitle = "vRt : " + tDiffStream.str() + "ms / " + tFramerateStream.str() + "Hz";
        glfwSetWindowTitle(window, wTitle.c_str());
    };



};
