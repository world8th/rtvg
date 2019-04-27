#pragma once 
#include "radx_core.hpp"
#include <iostream>

namespace radx {

    template <typename T>
    static inline auto sgn(const T& val) { return (T(0) < val) - (val < T(0)); }

    template<class T = uint64_t>
    static inline T tiled(const T& sz, const T& gmaxtile) {
        // return (int32_t)ceil((double)sz / (double)gmaxtile);
        return sz <= 0 ? 0 : (sz / gmaxtile + sgn(sz % gmaxtile));
    }

    template <class T>
    static inline auto strided(const size_t& sizeo) { return sizeof(T) * sizeo; }

    // read binary (for SPIR-V)
    static inline auto readBinary(const std::string& filePath ) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
        std::vector<uint32_t> data = {};
        if (file.is_open()) {
            std::streampos size = file.tellg();
            data.resize(tiled(size_t(size), sizeof(uint32_t)));
            file.seekg(0, std::ios::beg);
            file.read((char *)data.data(), size);
            file.close();
        } else {
            std::cerr << "Failure to open " + filePath << std::endl;
        }
        return data;
    };

    // read source (unused)
    static inline auto readSource(const std::string& filePath, bool lineDirective = false ) {
        std::string content = "";
        std::ifstream fileStream(filePath, std::ios::in);
        if (!fileStream.is_open()) {
            std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl; return content;
        }
        std::string line = "";
        while (!fileStream.eof()) {
            std::getline(fileStream, line);
            if (lineDirective || line.find("#line") == std::string::npos) content.append(line + "\n");
        }
        fileStream.close();
        return content;
    };

    static inline auto makeShaderModuleInfo(const std::vector<uint32_t>& code) {
        auto smi = vk::ShaderModuleCreateInfo{};
        smi.pCode = (uint32_t *)code.data();
        smi.codeSize = code.size()*4;
        smi.flags = {};
        return smi;
    };

    // create shader module
    static inline auto createShaderModuleIntrusive(const vk::Device& device, const std::vector<uint32_t>& code, vk::ShaderModule& hndl) {
        return (hndl = device.createShaderModule(makeShaderModuleInfo(code)));
    };

    static inline auto createShaderModule(const vk::Device& device, const std::vector<uint32_t>& code) {
        auto sm = vk::ShaderModule{}; return createShaderModuleIntrusive(device, code, sm); return sm;
    };

    // create shader module
    static inline auto makeComputePipelineStageInfo(const vk::Device& device, const std::vector<uint32_t>& code, const char * entry = "main") {
        auto spi = vk::PipelineShaderStageCreateInfo{};
        spi.flags = {};
        createShaderModuleIntrusive(device, code, spi.module);
        spi.pName = entry;
        spi.stage = vk::ShaderStageFlagBits::eCompute;
        spi.pSpecializationInfo = {};
        return spi;
    };

    // create compute pipelines
    static inline auto createCompute(const vk::Device& device, const vk::PipelineShaderStageCreateInfo& spi, const vk::PipelineLayout& layout, const vk::PipelineCache& cache = {}) {
        auto cmpi = vk::ComputePipelineCreateInfo{};
        cmpi.flags = {};
        cmpi.layout = layout;
        cmpi.stage = spi;
        cmpi.basePipelineIndex = -1;
        return device.createComputePipeline(cache, cmpi);
    };

    // create compute pipelines
    static inline auto createCompute(const vk::Device& device, const std::vector<uint32_t>& code, const vk::PipelineLayout& layout, const vk::PipelineCache& cache = {}) {
        return createCompute(device, makeComputePipelineStageInfo(device, code), layout, cache);
    };

    // create compute pipelines
    static inline auto createCompute(const vk::Device& device, const std::string& path, const vk::PipelineLayout& layout, const vk::PipelineCache& cache = {}) {
        return createCompute(device, readBinary(path), layout, cache);
    };

    // general command buffer pipeline barrier
    static inline void commandBarrier(const vk::CommandBuffer& cmdBuffer) {
        VkMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.pNext = nullptr;
        memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; //| VK_ACCESS_TRANSFER_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
        cmdBuffer.pipelineBarrier(
            //vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eComputeShader, {}, { memoryBarrier }, {}, {});
    };

};
