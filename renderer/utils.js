import fs from "fs";
import nvk from "nvk";
import { GLSL } from "nvk-essentials";
import { performance } from "perf_hooks";
import glm from "gl-matrix";

(()=>{
    Object.assign(this, nvk);
    Object.assign(this, glm);
    this.win = new VulkanWindow({ width: 1920, height: 1080, title: "RTX Vector Graphics" });
    this.result = null;
    this.device = new VkDevice();
    this.instance = new VkInstance();
    this.surface = new VkSurfaceKHR();
    this.swapchain = null;
    this.pipelineLayout = new VkPipelineLayout();
    this.renderPass = new VkRenderPass();
    this.pipeline = new VkPipeline();
    this.cmdPool = new VkCommandPool();
    this.queue = new VkQueue();
    this.vertShaderModule = null;
    this.fragShaderModule = null;
    this.semaphoreImageAvailable = new VkSemaphore();
    this.semaphoreRenderingDone = new VkSemaphore();

    

    this.readBinaryFile = (path) => {
        return new Uint8Array(fs.readFileSync(path, null));
    };

    this.createShaderModule = (shaderSrc, shaderModule) => {
        let shaderModuleInfo = new VkShaderModuleCreateInfo();
        shaderModuleInfo.pCode = shaderSrc;
        shaderModuleInfo.codeSize = shaderSrc.byteLength;
        result = vkCreateShaderModule(device, shaderModuleInfo, null, shaderModule);
        ASSERT_VK_RESULT(result);
        return shaderModule;
    };


    this.createInstance = () => {
        // app info
        let appInfo = new VkApplicationInfo();
        appInfo.pApplicationName = "Hello!";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
    
        // create info
        let createInfo = new VkInstanceCreateInfo();
        createInfo.pApplicationInfo = appInfo;
    
        let instanceExtensions = win.getRequiredInstanceExtensions();
        createInfo.enabledExtensionCount = instanceExtensions.length;
        createInfo.ppEnabledExtensionNames = instanceExtensions;
        createInfo.enabledLayerCount = 0;
    
        // validation layers
        let validationLayers = [];
        createInfo.enabledLayerCount = validationLayers.length;
        createInfo.ppEnabledLayerNames = validationLayers;
    
        result = vkCreateInstance(createInfo, null, instance);
        ASSERT_VK_RESULT(result);
    };
    
    this.createWindowSurface = () => {
        result = win.createSurface(instance, null, surface);
        ASSERT_VK_RESULT(result);
    };
    
    this.createPhysicalDevice = () => {
        let deviceCount = { $:0 };
        vkEnumeratePhysicalDevices(instance, deviceCount, null);
        if (deviceCount.$ <= 0) console.error("Error: No render devices available!");
    
        let devices = [...Array(deviceCount.$)].map(() => new VkPhysicalDevice());
        result = vkEnumeratePhysicalDevices(instance, deviceCount, devices);
        ASSERT_VK_RESULT(result);
    
        // auto pick first found device
        this.physicalDevice = devices[0];
    
        let deviceFeatures = new VkPhysicalDeviceFeatures();
        vkGetPhysicalDeviceFeatures(physicalDevice, deviceFeatures);
    
        let deviceProperties = new VkPhysicalDeviceProperties();
        vkGetPhysicalDeviceProperties(physicalDevice, deviceProperties);
    
        console.log(`Using device: ${deviceProperties.deviceName}`);
    
        let deviceMemoryProperties = new VkPhysicalDeviceMemoryProperties();
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, deviceMemoryProperties);
    
        let queueFamilyCount = { $: 0 };
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, queueFamilyCount, null);
    
        let queueFamilies = [...Array(queueFamilyCount.$)].map(() => new VkQueueFamilyProperties());
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, queueFamilyCount, queueFamilies);
    
        createSurfaceCapabilities();
    
        let surfaceFormatCount = { $: 0 };
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, surfaceFormatCount, null);
        let surfaceFormats = [...Array(surfaceFormatCount.$)].map(() => new VkSurfaceFormatKHR());
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, surfaceFormatCount, surfaceFormats);
    
        let presentModeCount = { $: 0 };
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, presentModeCount, null);
        let presentModes = new Int32Array(presentModeCount.$);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, presentModeCount, presentModes);
    
        let surfaceSupport = { $: false };
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, surface, surfaceSupport);
        if (!surfaceSupport) console.error(`No surface creation support!`);
    };
    
    this.createLogicalDevice = () => {
        let deviceQueueInfo = new VkDeviceQueueCreateInfo();
        deviceQueueInfo.queueFamilyIndex = 0;
        deviceQueueInfo.queueCount = 1;
        deviceQueueInfo.pQueuePriorities = new Float32Array([1.0, 1.0, 1.0, 1.0]);
    
        let deviceExtensions = [
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
        ];
    
        let usedFeatures = new VkPhysicalDeviceFeatures();
        usedFeatures.samplerAnisotropy = true;
    
        let deviceInfo = new VkDeviceCreateInfo();
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = [deviceQueueInfo];
        deviceInfo.enabledExtensionCount = deviceExtensions.length;
        deviceInfo.ppEnabledExtensionNames = deviceExtensions;
        deviceInfo.pEnabledFeatures = usedFeatures;
    
        result = vkCreateDevice(physicalDevice, deviceInfo, null, device);
        ASSERT_VK_RESULT(result);
    };
    
    this.createQueue = () => {
        vkGetDeviceQueue(device, 0, 0, queue);
    };
    
    this.createSwapchain = () => {
        let imageExtent = new VkExtent2D();
        imageExtent.width = win.width;
        imageExtent.height = win.height;
    
        let swapchainInfo = new VkSwapchainCreateInfoKHR();
        swapchainInfo.surface = surface;
        swapchainInfo.minImageCount = 3;
        swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainInfo.imageExtent = imageExtent;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = swapchain || null;
    
        swapchain = new VkSwapchainKHR();
        result = vkCreateSwapchainKHR(device, swapchainInfo, null, swapchain);
        ASSERT_VK_RESULT(result);
    };
    
    this.createSwapchainImageViews = () => {
        let amountOfImagesInSwapchain = { $: 0 };
        vkGetSwapchainImagesKHR(device, swapchain, amountOfImagesInSwapchain, null);
        let swapchainImages = [...Array(amountOfImagesInSwapchain.$)].map(() => new VkImage());
    
        result = vkGetSwapchainImagesKHR(device, swapchain, amountOfImagesInSwapchain, swapchainImages);
        ASSERT_VK_RESULT(result);
    
        let imageViews = [...Array(amountOfImagesInSwapchain.$)].map(() => new VkImageView());
        for (let ii = 0; ii < amountOfImagesInSwapchain.$; ++ii) {
        let components = new VkComponentMapping();
        components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        let subresourceRange = new VkImageSubresourceRange();
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;
        let imageViewInfo = new VkImageViewCreateInfo();
        imageViewInfo.image = swapchainImages[ii];
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        imageViewInfo.components = components;
        imageViewInfo.subresourceRange = subresourceRange;
        result = vkCreateImageView(device, imageViewInfo, null, imageViews[ii])
        ASSERT_VK_RESULT(result);
        };
        swapchainImageViews = imageViews;
        swapchainImageCount = amountOfImagesInSwapchain.$;
    };
    
    this.createRenderPass = () => {
        let attachmentDescription = new VkAttachmentDescription();
        attachmentDescription.flags = 0;
        attachmentDescription.format = VK_FORMAT_B8G8R8A8_UNORM;
        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
        let attachmentReference = new VkAttachmentReference();
        attachmentReference.attachment = 0;
        attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
        let subpassDescription = new VkSubpassDescription();
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = null;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = [attachmentReference];
        subpassDescription.pResolveAttachments = null;
        subpassDescription.pDepthStencilAttachment = null;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = null;
    
        let subpassDependency = new VkSubpassDependency();
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.srcAccessMask = 0;
        subpassDependency.dstAccessMask = (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        subpassDependency.dependencyFlags = 0;
    
        let renderPassInfo = new VkRenderPassCreateInfo();
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = [attachmentDescription];
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = [subpassDescription];
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = [subpassDependency];
    
        result = vkCreateRenderPass(device, renderPassInfo, null, renderPass);
        ASSERT_VK_RESULT(result);
    };

    this.createFramebuffers = () => {
        framebuffers = [...Array(swapchainImageCount)].map(() => new VkFramebuffer());
        for (let ii = 0; ii < swapchainImageCount; ++ii) {
            let framebufferInfo = new VkFramebufferCreateInfo();
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = [swapchainImageViews[ii]];
            framebufferInfo.width = win.width;
            framebufferInfo.height = win.height;
            framebufferInfo.layers = 1;
            result = vkCreateFramebuffer(device, framebufferInfo, null, framebuffers[ii]);
            ASSERT_VK_RESULT(result);
        };
    };

    this.createCommandPool = () => {
        let cmdPoolInfo = new VkCommandPoolCreateInfo();
        cmdPoolInfo.flags = 0;
        cmdPoolInfo.queueFamilyIndex = 0;
        
        result = vkCreateCommandPool(device, cmdPoolInfo, null, cmdPool);
        ASSERT_VK_RESULT(result);
    };

    this.createCommandBuffers = () => {
        let cmdBufferAllocInfo = new VkCommandBufferAllocateInfo();
        cmdBufferAllocInfo.commandPool = cmdPool;
        cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferAllocInfo.commandBufferCount = swapchainImageCount;
        cmdBuffers = [...Array(swapchainImageCount)].map(() => new VkCommandBuffer());
        
        result = vkAllocateCommandBuffers(device, cmdBufferAllocInfo, cmdBuffers);
        ASSERT_VK_RESULT(result);
    };

    this.createDescriptorPool = () => {
        let descriptorPoolSize = new VkDescriptorPoolSize();
        descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorPoolSize.descriptorCount = 1;
      
        let samplerDescriptorPoolSize = new VkDescriptorPoolSize();
        samplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerDescriptorPoolSize.descriptorCount = 1;
      
        let descriptorPoolInfo = new VkDescriptorPoolCreateInfo();
        descriptorPoolInfo.maxSets = 1;
        descriptorPoolInfo.poolSizeCount = 2;
        descriptorPoolInfo.pPoolSizes = [descriptorPoolSize, samplerDescriptorPoolSize];
        result = vkCreateDescriptorPool(device, descriptorPoolInfo, null, descriptorPool);
        ASSERT_VK_RESULT(result);
    };
      

})();