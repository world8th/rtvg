//#pragma once

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define VTE_RENDERER_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define RADX_IMPLEMENTATION

#undef small
#define small char
#undef small
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "../base/appRenderer.hpp"
#include <svgpp/svgpp.hpp>
#include <mapbox/earcut.hpp>
#undef small

#include <glm/gtc/matrix_transform.hpp>

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

        args::ValueFlag<int32_t> reflLV(parser, "reflection-level", "Level of reflections", { 'R' });
        args::ValueFlag<int32_t> trnsLV(parser, "transparency-level", "Level of transparency", { 'T' });

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
        //if (modelInput == "") { std::cerr << "No model found :(" << std::endl; glfwTerminate(); exit(1); };

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

        //cameraController = std::make_shared<CameraController>();
        //cameraController->canvasSize = (glm::uvec2*) & this->windowWidth;
        //cameraController->eyePos = &this->eyePos;
        //cameraController->upVector = &this->upVector;
        //cameraController->viewVector = &this->viewVector;

        // create VK instance
        auto instance = appBase->createInstance();
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
        shaderPack = shaderPrefix + "intrusive/" + getShaderDir(devProperties.vendorID);

        // create radix sort application (RadX C++)
        physicalHelper = std::make_shared<radx::PhysicalDeviceHelper>(appBase->getPhysicalDevice(0));
        device = std::make_shared<radx::Device>()->initialize(appBase->createDevice(false, shaderPack, true), physicalHelper);
        appBase->createRenderpass(device);


        // create image output
        const auto SuperSampling = enableSuperSampling ? 2.0 : 1.0; // super sampling image
        this->canvasWidth = this->windowWidth * SuperSampling;
        this->canvasHeight = this->windowHeight * SuperSampling;

    };


    struct VkGeometryInstance {
        //float transform[12];
        glm::mat3x4 transform;
        uint32_t instanceId : 24;
        uint32_t mask : 8;
        uint32_t instanceOffset : 24;
        uint32_t flags : 8;
        uint64_t accelerationStructureHandle;
    };


    void Renderer::InitCommands() {
        


    };






    static inline auto makePipelineStageInfo(vk::Device device, std::string fpath = "", const char* entry = "main", vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eRaygenNV) {
        std::vector<uint32_t> code = radx::readBinary(fpath);

        auto spi = vk::PipelineShaderStageCreateInfo{};
        //spi.module = {};
        spi.flags = {};
        radx::createShaderModuleIntrusive(device, code, spi.module);
        spi.pName = entry;
        spi.stage = stage;
        spi.pSpecializationInfo = {};
        return spi;
    };





    void Renderer::InitRayTracing() {

        // get memory size and set max element count
        vk::DeviceSize memorySize = 1024 * 1024 * 32;
        {
            vmaDeviceBuffer = std::make_shared<radx::VmaAllocatedBuffer>(this->device, memorySize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer | vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eRayTracingNV | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_GPU_ONLY);
            vmaToHostBuffer = std::make_shared<radx::VmaAllocatedBuffer>(this->device, memorySize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_TO_CPU);
            vmaHostBuffer = std::make_shared<radx::VmaAllocatedBuffer>(this->device, memorySize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
        };

        // buffer helpers
        auto vertexHostVector = radx::Vector<glm::vec3>(vmaHostBuffer, 1024 * sizeof(glm::vec3), 0);
        auto indiceHostVector = radx::Vector<uint32_t>(vmaHostBuffer, 1024 * sizeof(uint32_t), vertexHostVector.range());
        auto instanceHostVector = radx::Vector<VkGeometryInstance>(vmaHostBuffer, 1024 * sizeof(VkGeometryInstance), indiceHostVector.offset()+indiceHostVector.range());
        auto scratchHostVector = radx::Vector<uint32_t>(vmaHostBuffer, 1024 * 1024, instanceHostVector.offset() + instanceHostVector.range());
        auto handlesHostVector = radx::Vector<uint8_t>(vmaHostBuffer, 1024 * SGHZ, scratchHostVector.offset() + scratchHostVector.range());
        rtHandleVector = handlesHostVector;

        // 
        //auto vertexDeviceVector = radx::Vector<glm::vec3>(vmaDeviceBuffer, 1024 * sizeof(glm::vec3), 0);
        //auto indiceDeviceVector = radx::Vector<uint32_t>(vmaDeviceBuffer, 1024 * sizeof(uint32_t), vertexDeviceVector.range());






        { // Templates Creation

            // Initial scene (planned to replace)
            using Point = std::array<float, 2>;

            // make polygons
            std::vector<std::vector<Point>> pgeometries;
            pgeometries.push_back({ {100, 0}, {100, 100}, {0, 100}, {0, 0} });
            pgeometries.push_back({ {75, 25}, {75, 75}, {25, 75}, {25, 25} });

            // get indices 
            std::vector<uint32_t>  indices = mapbox::earcut<uint32_t>(pgeometries);
            std::vector<glm::vec3> vertice = {};

            // merge into VEC3 array 
            float fdepth = 0.f;
            const float dp = (fdepth += 0.001f);
            for (auto& I : pgeometries) {
                for (auto& p : I) { vertice.push_back({ p[0],p[1],dp }); }
            }



            // 
            memcpy(vertexHostVector.data(), vertice.data(), vertice.size() * sizeof(glm::vec3));
            memcpy(indiceHostVector.data(), indices.data(), indices.size() * sizeof(uint32_t));


            // triangles
            vk::GeometryNV rtg = {};
            rtg.geometryType = vk::GeometryTypeNV::eTriangles;
            rtg.geometry.triangles.indexCount = indices.size();
            rtg.geometry.triangles.indexData = indiceHostVector;
            rtg.geometry.triangles.indexOffset = indiceHostVector.offset();
            rtg.geometry.triangles.indexType = vk::IndexType::eUint32;
            rtg.geometry.triangles.vertexCount = vertice.size();
            rtg.geometry.triangles.vertexStride = sizeof(glm::vec3);
            rtg.geometry.triangles.vertexData = vertexHostVector;
            rtg.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
            rtg.geometry.triangles.vertexOffset = vertexHostVector.offset();


            // instances
            vk::AccelerationStructureInfoNV dAS = {};
            dAS.flags = vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace;
            dAS.type = vk::AccelerationStructureTypeNV::eBottomLevel;
            dAS.geometryCount = 1;
            dAS.pGeometries = &rtg;

            // create acceleration structure object
            vk::AccelerationStructureCreateInfoNV cAS = {};
            cAS.info = dAS;
            vk::AccelerationStructureNV acceleration = vk::Device(*device).createAccelerationStructureNV(cAS);
            
            // get memory requirements
            vk::AccelerationStructureMemoryRequirementsInfoNV memv = {};
            memv.accelerationStructure = acceleration;
            memv.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eObject;
            vk::MemoryRequirements2 objectMem = vk::Device(*device).getAccelerationStructureMemoryRequirementsNV(memv);

            // allocated memory 
            VmaAllocation allocation = {}; VmaAllocationInfo allocationInfo = {};
            VmaAllocationCreateInfo vmac = {};
            vmac.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            vmaAllocateMemory(*device, (VkMemoryRequirements*)&objectMem.memoryRequirements, &vmac, &allocation, &allocationInfo);

            // bind memory
            vk::BindAccelerationStructureMemoryInfoNV bs = {};
            bs.accelerationStructure = acceleration;
            bs.memory = allocationInfo.deviceMemory;
            bs.memoryOffset = allocationInfo.offset;
            vk::Device(*device).bindAccelerationStructureMemoryNV({ bs });

            // build structure
            radx::submitOnce(*device, appBase->queue, appBase->commandPool, [&](VkCommandBuffer cmd) {
                vk::CommandBuffer(cmd).buildAccelerationStructureNV(dAS, nullptr, 0, false, acceleration, nullptr, scratchHostVector, scratchHostVector.offset());
                radx::commandBarrier(cmd);
            });

            // push to instances array
            accelerationTemplates.push_back(acceleration);
        }

        { // Instances Creation

            // create instances
            VkGeometryInstance ginstance = {};
            ginstance.instanceId = 0;
            ginstance.instanceOffset = 0;
            ginstance.mask = 0xFF;
            ginstance.flags = VkGeometryInstanceFlagBitsNV::VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_NV;
            ginstance.transform = glm::mat3x4(1.f);
            vk::Device(*device).getAccelerationStructureHandleNV(accelerationTemplates[0], sizeof(uint64_t), &ginstance.accelerationStructureHandle);
            instanceHostVector.map()[0] = ginstance;

            ginstance.transform = glm::mat3x4(glm::transpose(glm::translate(glm::mat4(1.f), glm::vec3(50, 50, 0.001f))));
            instanceHostVector.map()[1] = ginstance;

            // instances
            vk::AccelerationStructureInfoNV dAS = {};
            dAS.flags = vk::BuildAccelerationStructureFlagBitsNV::ePreferFastBuild | vk::BuildAccelerationStructureFlagBitsNV::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsNV::eAllowCompaction;
            dAS.type = vk::AccelerationStructureTypeNV::eTopLevel;
            dAS.instanceCount = 2;

            // create acceleration structure object
            vk::AccelerationStructureCreateInfoNV cAS = {};
            cAS.info = dAS;
            vk::AccelerationStructureNV acceleration = vk::Device(*device).createAccelerationStructureNV(cAS);

            // get memory requirements
            vk::AccelerationStructureMemoryRequirementsInfoNV memv = {};
            memv.accelerationStructure = acceleration;
            memv.type = vk::AccelerationStructureMemoryRequirementsTypeNV::eObject;
            vk::MemoryRequirements2KHR objectMem = vk::Device(*device).getAccelerationStructureMemoryRequirementsNV(memv);

            // allocated memory 
            VmaAllocation allocation = {}; VmaAllocationInfo allocationInfo = {};
            VmaAllocationCreateInfo vmac = {};
            vmac.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            vmaAllocateMemory(*device, (VkMemoryRequirements*)& objectMem.memoryRequirements, &vmac, &allocation, &allocationInfo);

            // bind memory
            vk::BindAccelerationStructureMemoryInfoNV bs = {};
            bs.accelerationStructure = acceleration;
            bs.memory = allocationInfo.deviceMemory;
            bs.memoryOffset = allocationInfo.offset;
            vk::Device(*device).bindAccelerationStructureMemoryNV(bs);

            // build structure
            radx::submitOnce(*device, appBase->queue, appBase->commandPool, [&](VkCommandBuffer cmd) {
                vk::CommandBuffer(cmd).buildAccelerationStructureNV(dAS, instanceHostVector, instanceHostVector.offset(), false, acceleration, nullptr, scratchHostVector, scratchHostVector.offset());
                radx::commandBarrier(cmd);
            });

            // push to instances array
            //accelerationTemplates.push_back(acceleration);
            accelerationScene = acceleration;
        }

        { // Descriptor Layout 
            const auto pbindings = vk::DescriptorBindingFlagBitsEXT::ePartiallyBound | vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind | vk::DescriptorBindingFlagBitsEXT::eVariableDescriptorCount | vk::DescriptorBindingFlagBitsEXT::eUpdateUnusedWhilePending;
            const auto vkfl = vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT().setPBindingFlags(&pbindings);
            const auto vkpi = vk::DescriptorSetLayoutCreateInfo().setPNext(&vkfl);

            const std::vector<vk::DescriptorSetLayoutBinding> _bindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll),
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll),
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eAll),
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eAccelerationStructureNV, 1, vk::ShaderStageFlagBits::eAll),
            };
            inputDescriptorLayout = vk::Device(*device).createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo(vkpi).setPBindings(_bindings.data()).setBindingCount(_bindings.size()));
        }

        {
            std::vector<vk::DescriptorSetLayout> dsLayouts = { vk::DescriptorSetLayout(inputDescriptorLayout) };

            // create pipeline layout
            auto dsc = vk::Device(*device).allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(*device).setPSetLayouts(&dsLayouts[0]).setDescriptorSetCount(1));
            inputDescriptorSet = dsc[0];

            // acceleration structure write descriptor
            vk::WriteDescriptorSetAccelerationStructureNV acx;
            acx.pAccelerationStructures = &accelerationScene;
            acx.accelerationStructureCount = 1;

            auto writeTmpl = vk::WriteDescriptorSet(inputDescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer);
            std::vector<vk::WriteDescriptorSet> writes = {
                vk::WriteDescriptorSet(writeTmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&vk::DescriptorBufferInfo(vertexHostVector)),
                vk::WriteDescriptorSet(writeTmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&vk::DescriptorBufferInfo(indiceHostVector)),
                vk::WriteDescriptorSet(writeTmpl).setDescriptorType(vk::DescriptorType::eAccelerationStructureNV).setDstBinding(3).setPNext(&acx)
            };
            vk::Device(*device).updateDescriptorSets(writes, {});
        };





        std::vector<vk::RayTracingShaderGroupCreateInfoNV> groups = {
            vk::RayTracingShaderGroupCreateInfoNV().setGeneralShader(0u).setClosestHitShader(~0u).setAnyHitShader(~0u).setIntersectionShader(~0u).setType(vk::RayTracingShaderGroupTypeNV::eGeneral),
            vk::RayTracingShaderGroupCreateInfoNV().setGeneralShader(~0u).setClosestHitShader(1u).setAnyHitShader(~0u).setIntersectionShader(~0u).setType(vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup),
            vk::RayTracingShaderGroupCreateInfoNV().setGeneralShader(2u).setClosestHitShader(~0u).setAnyHitShader(~0u).setIntersectionShader(~0u).setType(vk::RayTracingShaderGroupTypeNV::eGeneral),
        };

        // XPEH TB
        std::vector<vk::PipelineShaderStageCreateInfo> stages = {
            makePipelineStageInfo(*device, shaderPack + "/rtrace/rtrace.rgen.spv", "main", vk::ShaderStageFlagBits::eRaygenNV),
            makePipelineStageInfo(*device, shaderPack + "/rtrace/handle.rchit.spv", "main", vk::ShaderStageFlagBits::eClosestHitNV),
            makePipelineStageInfo(*device, shaderPack + "/rtrace/bgfill.rmiss.spv", "main", vk::ShaderStageFlagBits::eMissNV),
        };

        // 
        vk::PipelineLayoutCreateInfo lpc;
        lpc.pSetLayouts = &inputDescriptorLayout;
        lpc.setLayoutCount = 1;

        // 
        vk::RayTracingPipelineCreateInfoNV rpv;
        rpv.groupCount = groups.size();
        rpv.stageCount = stages.size();
        rpv.pGroups = groups.data();
        rpv.pStages = stages.data();
        rpv.maxRecursionDepth = 16;
        rpv.layout = vk::Device(*device).createPipelineLayout(lpc);
        rtPipeline = vk::Device(*device).createRayTracingPipelineNV(*device, rpv);

        // 
        rtPipelineLayout = rpv.layout;

        // get rt handles
        vk::Device(*device).getRayTracingShaderGroupHandlesNV(rtPipeline, 0, groups.size(), rtHandleVector.size()*SGHZ, rtHandleVector.data());
    };


    void Renderer::InitPipeline() {
        // create pipeline
        vk::Pipeline trianglePipeline = {};
        {
            vk::PipelineLayoutCreateInfo lpc;
            lpc.pSetLayouts = &inputDescriptorLayout;
            lpc.setLayoutCount = 1;
            rsPipelineLayout = vk::Device(*device).createPipelineLayout(lpc);

            // pipeline stages
            std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {
                vk::PipelineShaderStageCreateInfo().setModule(radx::createShaderModule(*device, radx::readBinary(shaderPack + "/render/render.vert.spv"))).setPName("main").setStage(vk::ShaderStageFlagBits::eVertex),
                vk::PipelineShaderStageCreateInfo().setModule(radx::createShaderModule(*device, radx::readBinary(shaderPack + "/render/render.frag.spv"))).setPName("main").setStage(vk::ShaderStageFlagBits::eFragment)
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
                .setLayout(rsPipelineLayout)
                .setRenderPass(appBase->renderpass)
                .setBasePipelineIndex(0)
                .setPMultisampleState(&vk::PipelineMultisampleStateCreateInfo().setRasterizationSamples(vk::SampleCountFlagBits::e1))
                .setPDynamicState(&vk::PipelineDynamicStateCreateInfo().setPDynamicStates(dynamicStates.data()).setDynamicStateCount(dynamicStates.size()))
                .setPTessellationState(&tesselationState));
        };

        { // write descriptors for showing texture
            outputImage = std::make_shared<radx::VmaAllocatedImage>(device, vk::ImageViewType::e2D, vk::Format::eR32G32B32A32Sfloat, appBase->applicationWindow.surfaceSize, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage);

            vk::SamplerCreateInfo samplerInfo = {};
            samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.minFilter = vk::Filter::eLinear;
            samplerInfo.magFilter = vk::Filter::eLinear;
            samplerInfo.compareEnable = false;
            auto sampler = vk::Device(*device).createSampler(samplerInfo); // create sampler

            // desc texture texture
            auto imageDesc = vk::DescriptorImageInfo(*outputImage);//.setSampler(sampler);

            // submit as secondary
            radx::submitOnce(*device, appBase->queue, appBase->commandPool, [&](VkCommandBuffer cmd) {
                VkImageMemoryBarrier img_barrier = {};
                img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                img_barrier.image = outputImage->image;
                img_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                img_barrier.newLayout = VkImageLayout(outputImage->layout);
                img_barrier.srcAccessMask = 0;
                img_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                img_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                img_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                img_barrier.subresourceRange = outputImage->srange;

                vk::CommandBuffer(cmd).pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, {vk::ImageMemoryBarrier(img_barrier)});
            });

            auto writeTmpl = vk::WriteDescriptorSet(inputDescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer);
            std::vector<vk::WriteDescriptorSet> writes = {
                vk::WriteDescriptorSet(writeTmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(2).setPImageInfo(&imageDesc),
            };
            vk::Device(*device).updateDescriptorSets(writes, {});


            // update descriptors
            //vk::Device(*device).updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
            //    vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstBinding(0).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageImage).setPImageInfo(&imageDesc),
            //}, nullptr);
        };

        currentContext = std::make_shared<vte::GraphicsContext>();
        { // create graphic context
            auto context = currentContext;

            // create graphics context
            context->queueFamilyIndex = appBase->queueFamilyIndex;
            context->queue = appBase->queue;
            context->commandPool = appBase->commandPool;
            context->device = device;
            context->pipeline = trianglePipeline;
            context->descriptorPool = *device;
            context->descriptorSets = {inputDescriptorSet};
            context->pipelineLayout = rsPipelineLayout;

            // create framebuffers by size
            context->renderpass = appBase->renderpass;
            context->swapchain = appBase->createSwapchain(device);
            context->framebuffers = appBase->createSwapchainFramebuffer(device, context->swapchain, context->renderpass);
        };
    };



    static inline vk::Result cmdRaytracingBarrierNV(vk::CommandBuffer cmdBuffer) {
        VkMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.pNext = nullptr;
        memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            1, &memoryBarrier,
            0, nullptr,
            0, nullptr);
        return vk::Result::eSuccess;
    };

    void Renderer::Draw() {
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
                commandBuffer = radx::createCommandBuffer(*currentContext->device, currentContext->commandPool, false, false); // do reference of cmd buffer
                commandBuffer.beginRenderPass(vk::RenderPassBeginInfo(currentContext->renderpass, currentContext->framebuffers[currentBuffer].frameBuffer, renderArea, clearValues.size(), clearValues.data()), vk::SubpassContents::eInline);
                commandBuffer.setViewport(0, std::vector<vk::Viewport> { viewport });
                commandBuffer.setScissor(0, std::vector<vk::Rect2D> { renderArea });
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, currentContext->pipeline);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentContext->pipelineLayout, 0, currentContext->descriptorSets, nullptr);
                commandBuffer.draw(4, 1, 0, 0);
                commandBuffer.endRenderPass();
                commandBuffer.end();
            };

            // create ray-tracing command (i.e. render vector graphics as is)
            vk::CommandBuffer& rtCmdBuf = this->rtCmdBuf;
            if (!rtCmdBuf) {
                rtCmdBuf = radx::createCommandBuffer(*currentContext->device, currentContext->commandPool, true, false);
                rtCmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingNV, rtPipeline);
                rtCmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV, rtPipelineLayout, 0, currentContext->descriptorSets, nullptr);
                rtCmdBuf.traceRaysNV(
                    rtHandleVector, rtHandleVector.offset() + 0ull * SGHZ,
                    rtHandleVector, rtHandleVector.offset() + 2ull * SGHZ, SGHZ,
                    rtHandleVector, rtHandleVector.offset() + 1ull * SGHZ, SGHZ,
                    {}, 0ull, 0ull,
                    appBase->size().width, appBase->size().height, 1u);
                cmdRaytracingBarrierNV(rtCmdBuf);
                rtCmdBuf.end();
            };

            // submit as secondary
            radx::submitOnce(*device, appBase->queue, appBase->commandPool, [&](VkCommandBuffer cmd) {
                vk::CommandBuffer(cmd).executeCommands({ rtCmdBuf });
            });


            // create render submission 
            std::vector<vk::Semaphore>
                waitSemaphores = { currentContext->framebuffers[n_semaphore].semaphore },
                signalSemaphores = { currentContext->framebuffers[c_semaphore].semaphore };
            std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

            // 
            std::array<vk::CommandBuffer, 1> XPEH = { commandBuffer };
            auto smbi = vk::SubmitInfo()
                .setPCommandBuffers(XPEH.data()).setCommandBufferCount(XPEH.size())
                .setPWaitDstStageMask(waitStages.data()).setPWaitSemaphores(waitSemaphores.data()).setWaitSemaphoreCount(waitSemaphores.size())
                .setPSignalSemaphores(signalSemaphores.data()).setSignalSemaphoreCount(signalSemaphores.size());

            // submit command once
            radx::submitCmd(*currentContext->device, currentContext->queue, { commandBuffer }, smbi);

            // delete command buffer 
            //{ currentContext->queue->device->logical.freeCommandBuffers(currentContext->queue->commandPool, { commandBuffer }); commandBuffer = nullptr; };
        };

        // present for displaying of this image
        vk::Queue(currentContext->queue).presentKHR(vk::PresentInfoKHR(
            1, &currentContext->framebuffers[c_semaphore].semaphore,
            1, &currentContext->swapchain,
            &currentBuffer, nullptr
        ));
    };


    void Renderer::HandleData() {
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
