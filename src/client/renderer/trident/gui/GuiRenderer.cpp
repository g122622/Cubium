/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "GuiRenderer.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "client/renderer/util/ShaderPath.hpp"
#include "client/ui/Glyph.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::gui {

namespace {

// 从文件加载SPIR-V着色器
std::vector<u8> loadShaderFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        spdlog::error("Failed to open shader file: {}", path.string());
        return {};
    }

    u64 fileSize = static_cast<u64>(file.tellg());
    std::vector<u8> code(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(fileSize));
    file.close();

    spdlog::info("Loaded GUI shader: {} ({} bytes)", path.string(), fileSize);
    return code;
}

VkShaderModule createShaderModuleHelper(VkDevice device, const std::vector<u8>& code)
{
    if (code.empty()) {
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        spdlog::error("Failed to create shader module");
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

} // namespace

GuiRenderer::GuiRenderer() = default;

GuiRenderer::~GuiRenderer()
{
    destroy();
}

Result<void> GuiRenderer::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkRenderPass renderPass,
    VkSampleCountFlagBits sampleCount)
{
    if (device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "VkDevice is null");
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;

    // 创建描述符布局
    auto result = _createDescriptors();
    if (!result.success()) {
        return result.error();
    }

    // 创建管线布局和图形管线
    result = _createPipelineLayout();
    if (!result.success()) {
        return result.error();
    }

    result = _createPipeline(renderPass, sampleCount);
    if (!result.success()) {
        return result.error();
    }

    // 创建缓冲区
    result = _createBuffers();
    if (!result.success()) {
        return result.error();
    }

    // 创建字体纹理
    result = _createFontTexture();
    if (!result.success()) {
        return result.error();
    }

    // 初始化字体渲染器
    if (m_font != nullptr) {
        result = m_fontRenderer.initialize(m_font);
        if (!result.success()) {
            return result.error();
        }
    }

    m_textureLayoutsInitialized = false;
    m_fontTextureInShaderReadLayout = false;

    m_initialized = true;
    return {};
}

void GuiRenderer::destroy()
{
    if (!m_initialized) return;

    VkDevice device = m_device;

    // 等待设备空闲
    vkDeviceWaitIdle(device);

    // 销毁纹理
    if (m_fontTextureView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_fontTextureView, nullptr);
        m_fontTextureView = VK_NULL_HANDLE;
    }
    if (m_fontTexture != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_fontTexture, nullptr);
        m_fontTexture = VK_NULL_HANDLE;
    }
    if (m_fontTextureMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_fontTextureMemory, nullptr);
        m_fontTextureMemory = VK_NULL_HANDLE;
    }

    if (m_itemPlaceholderView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_itemPlaceholderView, nullptr);
        m_itemPlaceholderView = VK_NULL_HANDLE;
    }
    if (m_itemPlaceholderTexture != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_itemPlaceholderTexture, nullptr);
        m_itemPlaceholderTexture = VK_NULL_HANDLE;
    }
    if (m_itemPlaceholderMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_itemPlaceholderMemory, nullptr);
        m_itemPlaceholderMemory = VK_NULL_HANDLE;
    }

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    // 销毁缓冲区
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }
    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (m_indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_indexBuffer, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
    }
    if (m_indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_indexBufferMemory, nullptr);
        m_indexBufferMemory = VK_NULL_HANDLE;
    }
    if (m_fontStagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_fontStagingBuffer, nullptr);
        m_fontStagingBuffer = VK_NULL_HANDLE;
    }
    if (m_fontStagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_fontStagingMemory, nullptr);
        m_fontStagingMemory = VK_NULL_HANDLE;
    }

    // 销毁管线
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    // 销毁描述符
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    m_fontRenderer.destroy();
    m_textureLayoutsInitialized = false;
    m_fontTextureInShaderReadLayout = false;
    m_initialized = false;
}

void GuiRenderer::setFont(mc::client::Font* font)
{
    m_font = font;
    if (m_initialized && m_font != nullptr) {
        const auto initResult = m_fontRenderer.initialize(m_font);
        if (initResult.failed()) {
            spdlog::warn("Failed to reinitialize GUI font renderer: {}", initResult.error().toString());
        }
        m_needsTextureUpdate = true;
    }
}

void GuiRenderer::beginFrame(f64 screenW, f64 screenH)
{
    m_screenWidth = screenW;
    m_screenHeight = screenH;
    m_vertices.clear();
    m_indices.clear();
    m_inFrame = true;
}

void GuiRenderer::prepareFrame(VkCommandBuffer commandBuffer)
{
    // 首次使用时初始化纹理布局
    if (!m_textureLayoutsInitialized) {
        _initializeTextureLayouts(commandBuffer);
        m_textureLayoutsInitialized = true;
    }

    // 在渲染通道外更新字体纹理
    if (m_needsTextureUpdate && m_font != nullptr) {
        _updateFontTexture(commandBuffer);
        m_needsTextureUpdate = false;
    }
}

void GuiRenderer::render(VkCommandBuffer commandBuffer)
{
    if (m_vertices.empty() || m_indices.empty()) return;

    // 上传顶点和索引数据（使用HOST_VISIBLE内存，可以在任何地方调用）
    _uploadBufferData(commandBuffer);

    // 绑定管线
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    // 设置推送常量（屏幕尺寸）
#ifdef __APPLE__
    struct PushConstants {
        f32 screenWidth;
        f32 screenHeight;
        f32 padding[2];
    } pc{static_cast<f32>(m_screenWidth), static_cast<f32>(m_screenHeight), {0.0f, 0.0f}};
#else
    struct PushConstants {
        f64 screenWidth;
        f64 screenHeight;
        f64 padding[2];
    } pc{m_screenWidth, m_screenHeight, {0.0, 0.0}};
#endif

    vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

    // 绑定描述符集（字体纹理）
    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

    // 绑定顶点缓冲
    VkDeviceSize vertexOffset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer, &vertexOffset);

    // 绑定索引缓冲
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // 绘制
    vkCmdDrawIndexed(commandBuffer, static_cast<u32>(m_indices.size()), 1, 0, 0, 0);
}

f64 GuiRenderer::drawText(const std::string& text, f64 x, f64 y, u32 color, bool shadow)
{
    if (m_font == nullptr) return 0.0;

    m_fontRenderer.beginBatch();
    f64 width;
    if (shadow) {
        width = m_fontRenderer.addTextWithShadow(text, x, y, color);
    } else {
        TextStyle style;
        style.color = color;
        style.shadow = false;
        width = m_fontRenderer.addText(text, x, y, style);
    }
    m_fontRenderer.endBatch();

    // 复制顶点和索引数据
    const auto& textVertices = m_fontRenderer.vertices();
    const auto& textIndices = m_fontRenderer.indices();

    u32 baseIndex = static_cast<u32>(m_vertices.size());
    m_vertices.insert(m_vertices.end(), textVertices.begin(), textVertices.end());

    for (u32 idx : textIndices) {
        m_indices.push_back(baseIndex + idx);
    }

    return width;
}

f64 GuiRenderer::drawTextCentered(const std::string& text, f64 x, f64 y, u32 color)
{
    f64 width = getTextWidth(text);
    return drawText(text, x - width * 0.5, y, color, true);
}

f64 GuiRenderer::getTextWidth(const std::string& text)
{
    if (m_font == nullptr) return 0.0;
    return m_fontRenderer.getTextWidth(text);
}

u32 GuiRenderer::getFontHeight() const
{
    return m_fontRenderer.getFontHeight();
}

void GuiRenderer::setFontScale(f64 scale)
{
    m_fontRenderer.setScale(static_cast<f32>(scale));
}

f64 GuiRenderer::getFontScale() const
{
    return m_fontRenderer.scale();
}

void GuiRenderer::fillRect(f64 x, f64 y, f64 width, f64 height, u32 color)
{
    u32 baseIndex = static_cast<u32>(m_vertices.size());

    // 四个顶点
    // 注意：使用负UV作为"纯色矩形"标记，片段着色器将跳过纹理采样。
    // 否则会错误地使用字体纹理alpha，导致准星/背景矩形不可见。
    // 纯色矩形使用槽位0（字体槽位），但不会采样纹理
    constexpr f64 SOLID_RECT_UV = -1.0;
    m_vertices.emplace_back(x, y, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_ATLAS_SLOT);                  // 左上
    m_vertices.emplace_back(x + width, y, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_ATLAS_SLOT);          // 右上
    m_vertices.emplace_back(x + width, y + height, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_ATLAS_SLOT); // 右下
    m_vertices.emplace_back(x, y + height, SOLID_RECT_UV, SOLID_RECT_UV, color, FONT_ATLAS_SLOT);         // 左下

    // 两个三角形
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);
}

void GuiRenderer::drawTexturedRect(f64 x, f64 y, f64 width, f64 height, f64 u0, f64 v0, f64 u1, f64 v1, u32 color)
{
    // 默认使用物品图集槽位
    drawTexturedRect(x, y, width, height, u0, v0, u1, v1, color, ITEM_ATLAS_SLOT);
}

void GuiRenderer::drawTexturedRect(
    f64 x, f64 y, f64 width, f64 height, f64 u0, f64 v0, f64 u1, f64 v1, u32 color, u8 atlasSlot)
{
    u32 baseIndex = static_cast<u32>(m_vertices.size());

    // 四个顶点，设置纹理坐标和图集槽位
    m_vertices.emplace_back(x, y, u0, v0, color, atlasSlot);                  // 左上
    m_vertices.emplace_back(x + width, y, u1, v0, color, atlasSlot);          // 右上
    m_vertices.emplace_back(x + width, y + height, u1, v1, color, atlasSlot); // 右下
    m_vertices.emplace_back(x, y + height, u0, v1, color, atlasSlot);         // 左下

    // 两个三角形
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);
}

void GuiRenderer::fillGradientRect(f64 x, f64 y, f64 width, f64 height, u32 colorTop, u32 colorBottom)
{
    u32 baseIndex = static_cast<u32>(m_vertices.size());

    // 四个顶点，顶部和底部不同颜色
    constexpr f64 SOLID_RECT_UV = -1.0;
    m_vertices.emplace_back(x, y, SOLID_RECT_UV, SOLID_RECT_UV, colorTop, FONT_ATLAS_SLOT);                     // 左上
    m_vertices.emplace_back(x + width, y, SOLID_RECT_UV, SOLID_RECT_UV, colorTop, FONT_ATLAS_SLOT);             // 右上
    m_vertices.emplace_back(x + width, y + height, SOLID_RECT_UV, SOLID_RECT_UV, colorBottom, FONT_ATLAS_SLOT); // 右下
    m_vertices.emplace_back(x, y + height, SOLID_RECT_UV, SOLID_RECT_UV, colorBottom, FONT_ATLAS_SLOT);         // 左下

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);
}

void GuiRenderer::fillGradientRectHorizontal(f64 x, f64 y, f64 width, f64 height, u32 colorLeft, u32 colorRight)
{
    u32 baseIndex = static_cast<u32>(m_vertices.size());

    // 四个顶点，左侧和右侧不同颜色
    constexpr f64 SOLID_RECT_UV = -1.0;
    m_vertices.emplace_back(x, y, SOLID_RECT_UV, SOLID_RECT_UV, colorLeft, FONT_ATLAS_SLOT);                   // 左上
    m_vertices.emplace_back(x + width, y, SOLID_RECT_UV, SOLID_RECT_UV, colorRight, FONT_ATLAS_SLOT);          // 右上
    m_vertices.emplace_back(x + width, y + height, SOLID_RECT_UV, SOLID_RECT_UV, colorRight, FONT_ATLAS_SLOT); // 右下
    m_vertices.emplace_back(x, y + height, SOLID_RECT_UV, SOLID_RECT_UV, colorLeft, FONT_ATLAS_SLOT);          // 左下

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);
}

void GuiRenderer::drawRect(f64 x, f64 y, f64 width, f64 height, u32 color)
{
    // 上边
    fillRect(x, y, width, 1.0, color);
    // 下边
    fillRect(x, y + height - 1.0, width, 1.0, color);
    // 左边
    fillRect(x, y, 1.0, height, color);
    // 右边
    fillRect(x + width - 1.0, y, 1.0, height, color);
}

Result<void> GuiRenderer::_createPipelineLayout()
{
    VkDevice device = m_device;

    // 推送常量范围
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
#ifdef __APPLE__
    pushConstantRange.size = sizeof(f32) * 4; // screenWidth, screenHeight, padding
#else
    pushConstantRange.size = sizeof(f64) * 4; // screenWidth, screenHeight, padding
#endif

    // 管线布局创建信息
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create pipeline layout");
    }

    return {};
}

Result<void> GuiRenderer::_createPipeline(VkRenderPass renderPass, VkSampleCountFlagBits sampleCount)
{
    VkDevice device = m_device;

    const auto vertPath = resolveShaderPath("gui.vert.spv");
    const auto fragPath = resolveShaderPath("gui.frag.spv");

    if (vertPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve GUI vertex shader: gui.vert.spv");
    }
    if (fragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve GUI fragment shader: gui.frag.spv");
    }

    // 加载SPIR-V着色器
    auto vertCode = loadShaderFile(vertPath);
    auto fragCode = loadShaderFile(fragPath);

    if (vertCode.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to load GUI vertex shader");
    }
    if (fragCode.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to load GUI fragment shader");
    }

    // 创建着色器模块
    VkShaderModule vertShaderModule = createShaderModuleHelper(m_device, vertCode);
    VkShaderModule fragShaderModule = createShaderModuleHelper(m_device, fragCode);

    if (vertShaderModule == VK_NULL_HANDLE) {
        return Error(ErrorCode::InitializationFailed, "Failed to create vertex shader module");
    }
    if (fragShaderModule == VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to create fragment shader module");
    }

    // 着色器阶段
    VkPipelineShaderStageCreateInfo vertStageInfo = {};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo = {};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    // 顶点输入描述
    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(GuiVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // 位置属性
    VkVertexInputAttributeDescription positionAttr = {};
    positionAttr.binding = 0;
    positionAttr.location = 0;
#ifdef __APPLE__
    positionAttr.format = VK_FORMAT_R32G32_SFLOAT;
#else
    positionAttr.format = VK_FORMAT_R64G64_SFLOAT;
#endif
    positionAttr.offset = offsetof(GuiVertex, x);

    // 纹理坐标属性
    VkVertexInputAttributeDescription texCoordAttr = {};
    texCoordAttr.binding = 0;
    texCoordAttr.location = 1;
#ifdef __APPLE__
    texCoordAttr.format = VK_FORMAT_R32G32_SFLOAT;
#else
    texCoordAttr.format = VK_FORMAT_R64G64_SFLOAT;
#endif
    texCoordAttr.offset = offsetof(GuiVertex, u);

    // 颜色属性
    VkVertexInputAttributeDescription colorAttr = {};
    colorAttr.binding = 0;
    colorAttr.location = 2;
    colorAttr.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttr.offset = offsetof(GuiVertex, color);

    // 图集槽位属性
    VkVertexInputAttributeDescription atlasSlotAttr = {};
    atlasSlotAttr.binding = 0;
    atlasSlotAttr.location = 3;
    atlasSlotAttr.format = VK_FORMAT_R8_UINT;
    atlasSlotAttr.offset = offsetof(GuiVertex, atlasSlot);

    VkVertexInputAttributeDescription attributeDescs[] = {positionAttr, texCoordAttr, colorAttr, atlasSlotAttr};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 4; // position, texCoord, color, atlasSlot
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪（动态设置）
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // GUI不剔除
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = sampleCount;

    // 深度测试（GUI禁用）
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    // 颜色混合（启用alpha混合）
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 动态状态（视口和裁剪）
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // 图形管线创建
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    // 清理着色器模块
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create graphics pipeline");
    }

    return {};
}
Result<void> GuiRenderer::_createDescriptors()
{
    VkDevice device = m_device;

    // 描述符集布局（16个采样器：字体、物品、14个GUI图集）
    // 槽位 0: 字体纹理 (R8)
    // 槽位 1: 物品纹理图集 (RGBA)
    // 槽位 2-15: GUI纹理图集 (RGBA)
    constexpr u32 MAX_SAMPLERS = 16;
    std::array<VkDescriptorSetLayoutBinding, MAX_SAMPLERS> bindings = {};

    for (u32 i = 0; i < MAX_SAMPLERS; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = MAX_SAMPLERS;
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor set layout");
    }

    // 描述符池
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = MAX_SAMPLERS; // 16个采样器

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor pool");
    }

    // 分配描述符集
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate descriptor set");
    }

    return {};
}

Result<void> GuiRenderer::_createBuffers()
{
    // 创建顶点缓冲（使用HOST_VISIBLE内存，以便在render pass内直接更新数据）
    auto result = _createBuffer(64 * 1024,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_vertexBuffer,
        m_vertexBufferMemory);
    if (!result.success()) {
        return result;
    }

    // 创建索引缓冲（使用HOST_VISIBLE内存）
    result = _createBuffer(128 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_indexBuffer,
        m_indexBufferMemory);
    if (!result.success()) {
        return result;
    }

    // 创建字体纹理暂存缓冲
    result = _createBuffer(256 * 256,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_fontStagingBuffer,
        m_fontStagingMemory);
    if (!result.success()) {
        return result;
    }

    // 初始清空暂存缓冲
    void* mapped = nullptr;
    VkResult mapResult = vkMapMemory(m_device, m_fontStagingMemory, 0, 256 * 256, 0, &mapped);
    if (mapResult == VK_SUCCESS && mapped != nullptr) {
        std::memset(mapped, 0, 256 * 256);
        vkUnmapMemory(m_device, m_fontStagingMemory);
    }

    return {};
}

Result<void> GuiRenderer::_createFontTexture()
{
    VkDevice device = m_device;

    // 创建字体纹理（256x256，单通道）
    constexpr u32 FONT_TEXTURE_SIZE = 256;

    // 创建图像
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = FONT_TEXTURE_SIZE;
    imageInfo.extent.height = FONT_TEXTURE_SIZE;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &m_fontTexture);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create font texture image");
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_fontTexture, &memRequirements);

    auto memTypeResult = _findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!memTypeResult.success()) {
        return memTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_fontTextureMemory);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate font texture memory");
    }

    vkBindImageMemory(device, m_fontTexture, m_fontTextureMemory, 0);

    // 创建图像视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_fontTexture;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device, &viewInfo, nullptr, &m_fontTextureView);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create font texture image view");
    }

    // 创建采样器
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sampler");
    }

    // 创建物品占位纹理（1x1 RGBA）
    VkImageCreateInfo placeholderImageInfo{};
    placeholderImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    placeholderImageInfo.imageType = VK_IMAGE_TYPE_2D;
    placeholderImageInfo.extent.width = 1;
    placeholderImageInfo.extent.height = 1;
    placeholderImageInfo.extent.depth = 1;
    placeholderImageInfo.mipLevels = 1;
    placeholderImageInfo.arrayLayers = 1;
    placeholderImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    placeholderImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    placeholderImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    placeholderImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    placeholderImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    placeholderImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    result = vkCreateImage(device, &placeholderImageInfo, nullptr, &m_itemPlaceholderTexture);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create placeholder texture image");
    }

    vkGetImageMemoryRequirements(device, m_itemPlaceholderTexture, &memRequirements);

    memTypeResult = _findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!memTypeResult.success()) {
        return memTypeResult.error();
    }

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_itemPlaceholderMemory);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate placeholder texture memory");
    }

    vkBindImageMemory(device, m_itemPlaceholderTexture, m_itemPlaceholderMemory, 0);

    VkImageViewCreateInfo placeholderViewInfo{};
    placeholderViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    placeholderViewInfo.image = m_itemPlaceholderTexture;
    placeholderViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    placeholderViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    placeholderViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    placeholderViewInfo.subresourceRange.baseMipLevel = 0;
    placeholderViewInfo.subresourceRange.levelCount = 1;
    placeholderViewInfo.subresourceRange.baseArrayLayer = 0;
    placeholderViewInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device, &placeholderViewInfo, nullptr, &m_itemPlaceholderView);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create placeholder texture image view");
    }

    // 更新描述符集
    VkDescriptorImageInfo fontDescImageInfo{};
    fontDescImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    fontDescImageInfo.imageView = m_fontTextureView;
    fontDescImageInfo.sampler = m_sampler;

    VkDescriptorImageInfo placeholderDescImageInfo{};
    placeholderDescImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    placeholderDescImageInfo.imageView = m_itemPlaceholderView;
    placeholderDescImageInfo.sampler = m_sampler;

    VkWriteDescriptorSet descriptorWrites[2] = {};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &fontDescImageInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &placeholderDescImageInfo;

    vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);

    m_needsTextureUpdate = true;
    return {};
}

void GuiRenderer::_updateFontTexture(VkCommandBuffer commandBuffer)
{
    if (m_font == nullptr || !m_font->isValid()) return;

    const u8* pixels = m_font->atlasPixels();
    u32 size = m_font->atlasSize();

    if (pixels == nullptr || size == 0) return;

    // 上传到暂存缓冲
    void* mapped = nullptr;
    VkResult mapResult = vkMapMemory(m_device, m_fontStagingMemory, 0, size * size, 0, &mapped);
    if (mapResult != VK_SUCCESS || mapped == nullptr) {
        spdlog::error("GuiRenderer: failed to map font staging memory: {}", static_cast<i32>(mapResult));
        return;
    }
    std::memcpy(mapped, pixels, size * size);
    vkUnmapMemory(m_device, m_fontStagingMemory);

    // 转换图像布局到 TRANSFER_DST
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout =
        m_fontTextureInShaderReadLayout ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_fontTexture;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = m_fontTextureInShaderReadLayout ? VK_ACCESS_SHADER_READ_BIT : 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        m_fontTextureInShaderReadLayout ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    // 复制缓冲到图像
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {size, size, 1};

    vkCmdCopyBufferToImage(
        commandBuffer, m_fontStagingBuffer, m_fontTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换到着色器读取布局
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    m_fontTextureInShaderReadLayout = true;
}

void GuiRenderer::_initializeTextureLayouts(VkCommandBuffer commandBuffer)
{
    // 初始化字体纹理布局
    if (m_fontTexture != VK_NULL_HANDLE) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_fontTexture;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        m_fontTextureInShaderReadLayout = true;
    }

    // 初始化物品占位纹理布局
    if (m_itemPlaceholderTexture != VK_NULL_HANDLE) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_itemPlaceholderTexture;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
    }
}

void GuiRenderer::_uploadBufferData(VkCommandBuffer commandBuffer)
{
    (void)commandBuffer; // 不需要 command buffer，使用 HOST_VISIBLE 内存

    if (m_vertices.empty() && m_indices.empty()) return;

    VkDeviceSize vertexSize = m_vertices.size() * sizeof(GuiVertex);
    VkDeviceSize indexSize = m_indices.size() * sizeof(u32);

    // 若顶点数据超出缓冲容量，则以2倍所需大小重建缓冲
    if (vertexSize > m_vertexBufferSize) {
        if (m_vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
            m_vertexBuffer = VK_NULL_HANDLE;
        }
        if (m_vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
            m_vertexBufferMemory = VK_NULL_HANDLE;
        }

        auto result = _createBuffer(vertexSize * 2,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_vertexBuffer,
            m_vertexBufferMemory);
        if (!result.success()) {
            spdlog::error("GuiRenderer: failed to reallocate vertex buffer ({}B)", vertexSize);
            return;
        }
        m_vertexBufferSize = vertexSize * 2;
    }

    // 若索引数据超出缓冲容量，则以2倍所需大小重建缓冲
    if (indexSize > m_indexBufferSize) {
        if (m_indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
            m_indexBuffer = VK_NULL_HANDLE;
        }
        if (m_indexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
            m_indexBufferMemory = VK_NULL_HANDLE;
        }

        auto result = _createBuffer(indexSize * 2,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_indexBuffer,
            m_indexBufferMemory);
        if (!result.success()) {
            spdlog::error("GuiRenderer: failed to reallocate index buffer ({}B)", indexSize);
            return;
        }
        m_indexBufferSize = indexSize * 2;
    }

    // 直接映射顶点缓冲并复制数据
    void* vertexMapped = nullptr;
    VkResult mapResult = vkMapMemory(m_device, m_vertexBufferMemory, 0, vertexSize, 0, &vertexMapped);
    if (mapResult == VK_SUCCESS && vertexMapped != nullptr) {
        std::memcpy(vertexMapped, m_vertices.data(), vertexSize);
        vkUnmapMemory(m_device, m_vertexBufferMemory);
    }

    // 直接映射索引缓冲并复制数据
    void* indexMapped = nullptr;
    mapResult = vkMapMemory(m_device, m_indexBufferMemory, 0, indexSize, 0, &indexMapped);
    if (mapResult == VK_SUCCESS && indexMapped != nullptr) {
        std::memcpy(indexMapped, m_indices.data(), indexSize);
        vkUnmapMemory(m_device, m_indexBufferMemory);
    }
}

void GuiRenderer::setItemTextureAtlas(VkImageView itemView, VkSampler itemSampler)
{
    if (!m_initialized || itemView == VK_NULL_HANDLE || itemSampler == VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_device;

    // 更新 binding 1 的描述符
    VkDescriptorImageInfo itemImageInfo = {};
    itemImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    itemImageInfo.imageView = itemView;
    itemImageInfo.sampler = itemSampler;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 1;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &itemImageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}

void GuiRenderer::setGuiTextureAtlas(VkImageView guiView, VkSampler guiSampler)
{
    if (!m_initialized || guiView == VK_NULL_HANDLE || guiSampler == VK_NULL_HANDLE) {
        return;
    }

    m_guiTextureView = guiView;
    m_guiSampler = guiSampler;

    // 更新 binding 2 的描述符
    VkDescriptorImageInfo guiImageInfo = {};
    guiImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    guiImageInfo.imageView = guiView;
    guiImageInfo.sampler = guiSampler;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 2;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &guiImageInfo;

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

Result<u32> GuiRenderer::registerAtlas(const std::string& name, VkImageView view, VkSampler sampler)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "GuiRenderer not initialized");
    }

    if (view == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "ImageView or Sampler is null");
    }

    // 检查是否已注册
    auto it = m_atlasSlots.find(name);
    if (it != m_atlasSlots.end()) {
        // 更新现有图集
        u32 slot = it->second;
        _updateAtlasDescriptor(slot, view, sampler);
        return slot;
    }

    // 分配新槽位
    if (m_nextGuiSlot >= 16) { // 最大16个槽位
        return Error(ErrorCode::CapacityExceeded, "Maximum atlas slots reached. Consider consolidating atlases.");
    }

    u32 slot = m_nextGuiSlot++;
    m_atlasSlots[name] = slot;

    // 更新描述符
    _updateAtlasDescriptor(slot, view, sampler);

    spdlog::info("[GuiRenderer] Registered atlas '{}' at slot {}", name, slot);
    return slot;
}

std::optional<u32> GuiRenderer::getAtlasSlot(const std::string& name) const
{
    auto it = m_atlasSlots.find(name);
    if (it != m_atlasSlots.end()) {
        return it->second;
    }
    return std::nullopt;
}

void GuiRenderer::_updateAtlasDescriptor(u32 binding, VkImageView view, VkSampler sampler)
{
    if (m_descriptorSet == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

// ============================================================================
// Vulkan 辅助函数
// ============================================================================

Result<u32> GuiRenderer::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return mc::client::renderer::VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

Result<void> GuiRenderer::_createBuffer(VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    return mc::client::renderer::VulkanUtils::createBuffer(
        m_device, m_physicalDevice, size, usage, properties, buffer, memory);
}

} // namespace mc::client::renderer::trident::gui
