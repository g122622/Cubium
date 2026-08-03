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

#include "GuiAtlasRegistry.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <optional>
#include <string>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::gui {

GuiAtlasRegistry::GuiAtlasRegistry(VkDevice device)
    : m_device(device)
{
    // 初始化所有槽位为无效状态
    for (auto& slot : m_slots) {
        slot.id = 0;
        slot.imageView = VK_NULL_HANDLE;
        slot.sampler = VK_NULL_HANDLE;
        slot.name.clear();
    }
}

GuiAtlasRegistry::~GuiAtlasRegistry()
{
    // 只销毁我们创建的资源
    // VkImageView 和 VkSampler 由调用者管理
    if (m_device != VK_NULL_HANDLE) {
        if (m_ownsPool && m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }
        if (m_ownsLayout && m_descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
            m_descriptorSetLayout = VK_NULL_HANDLE;
        }
    }
}

Result<void> GuiAtlasRegistry::initialize(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool)
{

    if (m_device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "VkDevice is null");
    }

    m_descriptorSetLayout = descriptorSetLayout;
    m_descriptorPool = descriptorPool;
    m_ownsLayout = false;
    m_ownsPool = false;

    // 分配描述符集
    auto result = _allocateDescriptorSet();
    if (result.failed()) {
        return result.error();
    }

    m_initialized = true;
    spdlog::info("[GuiAtlasRegistry] Initialized with provided layout and pool");
    return {};
}

Result<u32> GuiAtlasRegistry::registerAtlas(const std::string& name, VkImageView imageView, VkSampler sampler)
{

    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "Registry not initialized");
    }

    if (imageView == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "ImageView is null");
    }

    if (sampler == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Sampler is null");
    }

    // 检查是否已注册
    auto it = m_nameToSlot.find(name);
    if (it != m_nameToSlot.end()) {
        // 更新现有图集
        u32 slotId = it->second;
        auto updateResult = updateAtlas(slotId, imageView, sampler);
        if (updateResult.failed()) {
            return updateResult.error();
        }
        spdlog::info("[GuiAtlasRegistry] Updated atlas '{}' at slot {}", name, slotId);
        return slotId;
    }

    // 分配新槽位
    if (m_nextGuiSlot >= m_slots.size()) {
        return Error(ErrorCode::CapacityExceeded,
            "No available atlas slots. Maximum " + std::to_string(MAX_GUI_ATLAS_SLOTS) + " GUI atlases supported.");
    }

    u32 slotId = m_nextGuiSlot++;
    m_slots[slotId].id = slotId;
    m_slots[slotId].imageView = imageView;
    m_slots[slotId].sampler = sampler;
    m_slots[slotId].name = name;
    m_nameToSlot[name] = slotId;
    m_atlasCount++;

    // 更新描述符
    _writeDescriptor(slotId, imageView, sampler);

    spdlog::info("[GuiAtlasRegistry] Registered atlas '{}' at slot {}", name, slotId);
    return slotId;
}

Result<void> GuiAtlasRegistry::unregisterAtlas(const std::string& name)
{
    auto it = m_nameToSlot.find(name);
    if (it == m_nameToSlot.end()) {
        return Error(ErrorCode::NotFound, "Atlas not found: " + name);
    }

    u32 slotId = it->second;
    m_slots[slotId].imageView = VK_NULL_HANDLE;
    m_slots[slotId].sampler = VK_NULL_HANDLE;
    m_slots[slotId].name.clear();
    m_nameToSlot.erase(it);
    m_atlasCount--;

    spdlog::info("[GuiAtlasRegistry] Unregistered atlas '{}' from slot {}", name, slotId);
    return {};
}

std::optional<u32> GuiAtlasRegistry::getSlotId(const std::string& name) const
{
    auto it = m_nameToSlot.find(name);
    if (it != m_nameToSlot.end()) {
        return it->second;
    }
    return std::nullopt;
}

const AtlasSlot* GuiAtlasRegistry::getAtlas(u32 slotId) const
{
    if (slotId >= m_slots.size()) {
        return nullptr;
    }
    const auto& slot = m_slots[slotId];
    if (!slot.isValid()) {
        return nullptr;
    }
    return &slot;
}

Result<void> GuiAtlasRegistry::updateAtlas(u32 slotId, VkImageView imageView, VkSampler sampler)
{
    if (slotId >= m_slots.size()) {
        return Error(ErrorCode::OutOfRange, "Invalid slot ID");
    }

    if (imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "ImageView or Sampler is null");
    }

    m_slots[slotId].imageView = imageView;
    m_slots[slotId].sampler = sampler;

    // 更新描述符
    _writeDescriptor(slotId, imageView, sampler);

    return {};
}

void GuiAtlasRegistry::updateFontTexture(VkImageView imageView, VkSampler sampler)
{
    m_slots[FONT_SLOT].id = FONT_SLOT;
    m_slots[FONT_SLOT].imageView = imageView;
    m_slots[FONT_SLOT].sampler = sampler;
    m_slots[FONT_SLOT].name = "font";

    _writeDescriptor(FONT_SLOT, imageView, sampler);
}

void GuiAtlasRegistry::updateItemAtlas(VkImageView imageView, VkSampler sampler)
{
    m_slots[ITEM_SLOT].id = ITEM_SLOT;
    m_slots[ITEM_SLOT].imageView = imageView;
    m_slots[ITEM_SLOT].sampler = sampler;
    m_slots[ITEM_SLOT].name = "item";

    _writeDescriptor(ITEM_SLOT, imageView, sampler);
}

Result<void> GuiAtlasRegistry::_createDescriptorSetLayout()
{
    // TODO: 当前仅接受外部提供的布局，此方法暂未实现
    return Error(ErrorCode::Unsupported, "Use external descriptor set layout");
}

Result<void> GuiAtlasRegistry::_allocateDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate descriptor set: " + std::to_string(result));
    }

    return {};
}

void GuiAtlasRegistry::_writeDescriptor(u32 binding, VkImageView imageView, VkSampler sampler)
{
    if (m_descriptorSet == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

} // namespace mc::client::renderer::trident::gui
