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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <array>
#include <vector>

namespace mc::skin {

/**
 * @brief 默认皮肤提供者
 *
 * 提供内置的默认皮肤（Steve 和 Alex）。
 *
 * 默认皮肤：
 * - Steve: 宽手臂模型，textures/entity/player/wide/steve.png
 * - Alex: 窄手臂模型，textures/entity/player/slim/alex.png
 *
 * 默认皮肤选择算法：
 * - 根据 UUID 哈希的最低位决定
 * - (UUID.hashCode() & 1) == 1 -> Alex
 * - (UUID.hashCode() & 1) == 0 -> Steve
 */
class DefaultSkinProvider {
public:
    /**
     * @brief 初始化默认皮肤
     *
     * 加载内置的 Steve 和 Alex 皮肤数据。
     *
     * @return 成功或错误
     */
    Result<void> initialize();

    /**
     * @brief 获取默认皮肤 ResourceLocation
     * @param uuid 玩家UUID
     * @return 皮肤位置（Steve 或 Alex）
     */
    [[nodiscard]] ResourceLocation getDefaultSkin(const std::array<u8, 16>& uuid) const noexcept;

    /**
     * @brief 获取默认皮肤类型
     * @param uuid 玩家UUID
     * @return 皮肤类型（Default 或 Slim）
     */
    [[nodiscard]] SkinType getDefaultSkinType(const std::array<u8, 16>& uuid) const noexcept;

    /**
     * @brief 获取 Steve 皮肤位置
     */
    [[nodiscard]] ResourceLocation getSteveSkin() const noexcept { return m_steveLocation; }

    /**
     * @brief 获取 Alex 皮肤位置
     */
    [[nodiscard]] ResourceLocation getAlexSkin() const noexcept { return m_alexLocation; }

    /**
     * @brief 获取 Steve 皮肤 PNG 数据
     * @return PNG 数据（64x64）
     */
    [[nodiscard]] const std::vector<u8>& getSteveSkinData() const noexcept { return m_steveData; }

    /**
     * @brief 获取 Alex 皮肤 PNG 数据
     * @return PNG 数据（64x64）
     */
    [[nodiscard]] const std::vector<u8>& getAlexSkinData() const noexcept { return m_alexData; }

    /**
     * @brief 检查 ResourceLocation 是否为默认皮肤
     * @param location 资源位置
     * @return 是否为默认皮肤
     */
    [[nodiscard]] bool isDefaultSkin(const ResourceLocation& location) const noexcept;

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

private:
    /**
     * @brief 加载内置皮肤数据
     *
     * 如果资源文件不存在，使用硬编码的简单皮肤。
     */
    Result<void> _loadBuiltinSkins();

    ResourceLocation m_steveLocation{"minecraft:textures/entity/player/wide/steve.png"};
    ResourceLocation m_alexLocation{"minecraft:textures/entity/player/slim/alex.png"};

    std::vector<u8> m_steveData; // Steve 皮肤 PNG 数据
    std::vector<u8> m_alexData;  // Alex 皮肤 PNG 数据

    bool m_initialized = false;
};

} // namespace mc::skin
