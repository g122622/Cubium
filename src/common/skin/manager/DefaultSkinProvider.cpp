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
    spdlog::info("DefaultSkinProvider initialized");
    return {};
}

Result<void> DefaultSkinProvider::_loadBuiltinSkins()
{
    // TODO: 实现从资源文件加载真实皮肤数据
    // 当前使用简化的 fallback 实现，生成空的 64x64 RGBA 数据占位
    // 生产环境应该从 resources/textures/entity/steve.png 和 alex.png 加载

    m_steveData.resize(SKIN_DATA_SIZE, 0);
    m_alexData.resize(SKIN_DATA_SIZE, 0);

    return {};
}

ResourceLocation DefaultSkinProvider::getDefaultSkin(const std::array<u8, 16>& uuid) const noexcept
{
    if (getDefaultSkinType(uuid) == SkinType::Slim) {
        return m_alexLocation;
    }
    return m_steveLocation;
}

SkinType DefaultSkinProvider::getDefaultSkinType(const std::array<u8, 16>& uuid) const noexcept
{
    return getDefaultSkinTypeForUUID(uuid);
}

bool DefaultSkinProvider::isDefaultSkin(const ResourceLocation& location) const noexcept
{
    return location == m_steveLocation || location == m_alexLocation;
}

} // namespace mc::skin
