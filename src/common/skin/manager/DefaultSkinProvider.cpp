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

#include "DefaultSkinProvider.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <spdlog/spdlog.h>

namespace mc::skin {

// 皮肤尺寸常量
constexpr size_t SKIN_WIDTH = 64;
constexpr size_t SKIN_HEIGHT = 64;
constexpr size_t SKIN_CHANNELS = 4; // RGBA
constexpr size_t SKIN_DATA_SIZE = SKIN_WIDTH * SKIN_HEIGHT * SKIN_CHANNELS;

Result<void> DefaultSkinProvider::initialize()
{
    if (m_initialized) {
        return {};
    }

    auto result = _loadBuiltinSkins();
    if (!result.success()) {
        spdlog::warn(
            "DefaultSkinProvider: Failed to load builtin skins, using fallback: {}", result.error().toString());
        // 不返回错误，使用 fallback 数据
    }

    m_initialized = true;
    spdlog::info("DefaultSkinProvider initialized with {} default skins", DEFAULT_SKIN_COUNT);
    return {};
}

Result<void> DefaultSkinProvider::_loadBuiltinSkins()
{
    // TODO: 实现从资源文件加载真实皮肤数据
    // 当前使用简化的 fallback 实现，生成空的 64x64 RGBA 数据占位
    // 生产环境应该从 resources/textures/entity/player/{slim|wide}/{name}.png 加载

    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        m_skinData[i].resize(SKIN_DATA_SIZE, 0);
    }

    return {};
}

ResourceLocation DefaultSkinProvider::getDefaultSkin(const std::array<u8, 16>& uuid) const noexcept
{
    const DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(uuid);
    return variant.textureLocation();
}

SkinType DefaultSkinProvider::getDefaultSkinType(const std::array<u8, 16>& uuid) const noexcept
{
    const DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(uuid);
    return variant.skinType;
}

ResourceLocation DefaultSkinProvider::getSkinLocation(size_t variantIndex) const noexcept
{
    if (variantIndex >= DEFAULT_SKIN_COUNT) {
        variantIndex = 6; // 回退到 slim/steve（规范默认皮肤）
    }
    const auto& variants = getDefaultSkinVariants();
    return variants[variantIndex].textureLocation();
}

ResourceLocation DefaultSkinProvider::getCanonicalDefaultSkinLocation() const noexcept
{
    return getCanonicalDefaultSkin().textureLocation();
}

const std::vector<u8>& DefaultSkinProvider::getSkinData(size_t variantIndex) const noexcept
{
    if (variantIndex >= DEFAULT_SKIN_COUNT) {
        variantIndex = 6; // 回退到 slim/steve
    }
    return m_skinData[variantIndex];
}

bool DefaultSkinProvider::isDefaultSkin(const ResourceLocation& location) const noexcept
{
    const auto& variants = getDefaultSkinVariants();
    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        if (location == variants[i].textureLocation()) {
            return true;
        }
    }
    return false;
}

} // namespace mc::skin
