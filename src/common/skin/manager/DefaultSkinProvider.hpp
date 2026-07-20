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
#include "common/resource/pack/IResourcePack.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <array>
#include <vector>

namespace mc::skin {

/**
 * @brief 默认皮肤提供者
 *
 * 提供 MC 1.21.1 的 18 种内置默认皮肤（9 slim + 9 wide）。
 * 通过 UUID 哈希选择默认皮肤，与 MC Java 版 DefaultPlayerSkin 一致。
 *
 * 默认皮肤选择算法：
 * - index = Math.floorMod(UUID.hashCode(), 18)
 * - 索引 0-8:  slim 变体 (alex, ari, efe, kai, makena, noor, steve, sunny, zuri)
 * - 索引 9-17: wide 变体 (alex, ari, efe, kai, makena, noor, steve, sunny, zuri)
 *
 * 皮肤名称列表：alex, ari, efe, kai, makena, noor, steve, sunny, zuri
 * 纹理路径格式：minecraft:textures/entity/player/{slim|wide}/{name}.png
 *
 * 生命周期约束：必须在 initialize() 调用之前通过 setResourcePacks 注入资源包列表，
 * 否则 _loadBuiltinSkins 会因无资源包可用而回退到零像素占位数据。
 * 查找策略：按资源包优先级反向遍历（后添加的优先），与纹理/模型加载一致。
 */
class DefaultSkinProvider {
public:
    /**
     * @brief 初始化默认皮肤
     *
     * 加载内置的 18 种默认皮肤数据。需要先通过 setResourcePacks 注入资源包列表。
     *
     * @return 成功或错误
     */
    Result<void> initialize();

    /**
     * @brief 获取默认皮肤 ResourceLocation
     *
     * 根据 UUID 哈希从 18 种默认皮肤中选择。
     *
     * @param uuid 玩家UUID
     * @return 皮肤位置
     */
    [[nodiscard]] ResourceLocation getDefaultSkin(const std::array<u8, 16>& uuid) const noexcept;

    /**
     * @brief 获取默认皮肤类型
     * @param uuid 玩家UUID
     * @return 皮肤类型（Default 或 Slim）
     */
    [[nodiscard]] SkinType getDefaultSkinType(const std::array<u8, 16>& uuid) const noexcept;

    /**
     * @brief 获取指定变体的 ResourceLocation
     * @param variantIndex 变体索引 (0-17)
     * @return 皮肤位置
     */
    [[nodiscard]] ResourceLocation getSkinLocation(size_t variantIndex) const noexcept;

    /**
     * @brief 获取规范默认皮肤（无 UUID 上下文时的回退）
     *
     * 返回 slim/steve（索引 6），与 MC 源码一致。
     *
     * @return 皮肤位置
     */
    [[nodiscard]] ResourceLocation getCanonicalDefaultSkinLocation() const noexcept;

    /**
     * @brief 获取 Steve (wide) 皮肤位置（向后兼容）
     */
    [[nodiscard]] ResourceLocation getSteveSkin() const noexcept { return getSkinLocation(15); }

    /**
     * @brief 获取 Alex (slim) 皮肤位置（向后兼容）
     */
    [[nodiscard]] ResourceLocation getAlexSkin() const noexcept { return getSkinLocation(0); }

    /**
     * @brief 获取 Steve 皮肤 RGBA 像素数据（向后兼容）
     * @return 64x64 RGBA 像素数据（共 16384 字节）
     */
    [[nodiscard]] const std::vector<u8>& getSteveSkinData() const noexcept { return m_skinData[15]; }

    /**
     * @brief 获取 Alex 皮肤 RGBA 像素数据（向后兼容）
     * @return 64x64 RGBA 像素数据（共 16384 字节）
     */
    [[nodiscard]] const std::vector<u8>& getAlexSkinData() const noexcept { return m_skinData[0]; }

    /**
     * @brief 获取指定变体的皮肤 RGBA 像素数据
     * @param variantIndex 变体索引 (0-17)
     * @return 64x64 RGBA 像素数据（共 16384 字节）
     */
    [[nodiscard]] const std::vector<u8>& getSkinData(size_t variantIndex) const noexcept;

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

    // ========== 资源包设置 ==========

    /**
     * @brief 设置资源包列表（用于从资源包加载默认皮肤 PNG 纹理）
     *
     * 必须在 initialize() 调用之前设置，否则 _loadBuiltinSkins 将无法读取
     * `textures/entity/player/{slim|wide}/{name}.png` 资源，回退到零像素占位数据。
     * 列表顺序为添加顺序（低→高优先级），查找时反向遍历（后添加的优先），
     * 与 ResourceManager 的纹理加载惯例一致。
     *
     * @param resourcePacks 资源包指针列表（非所有权，调用方保证生命周期）
     */
    void setResourcePacks(std::vector<IResourcePack*> resourcePacks) { m_resourcePacks = std::move(resourcePacks); }

    /**
     * @brief 获取已设置的资源包列表
     */
    [[nodiscard]] const std::vector<IResourcePack*>& resourcePacks() const noexcept { return m_resourcePacks; }

private:
    /**
     * @brief 加载内置皮肤数据
     *
     * 遍历 18 个默认皮肤变体，从资源包列表按优先级读取 PNG 字节，
     * 用 stb_image 解码为 RGBA 像素后存入 m_skinData。
     * 单个变体加载失败仅记录警告并跳过，整体不失败（保持 initialize 容错语义）。
     */
    Result<void> _loadBuiltinSkins();

    /**
     * @brief 从资源包列表按优先级加载单个皮肤变体的 PNG 纹理
     *
     * @param variant 默认皮肤变体
     * @return 解码后的 RGBA 像素数据，所有资源包都未命中时返回空 vector
     */
    [[nodiscard]] std::vector<u8> _loadSkinFromResourcePack(const DefaultSkinVariant& variant) const;

    /// 18 种默认皮肤的 RGBA 像素数据（索引与 DefaultSkinVariant::index 一致）
    std::array<std::vector<u8>, DEFAULT_SKIN_COUNT> m_skinData;

    /// 资源包指针列表（非所有权，添加顺序=优先级从低到高），用于加载默认皮肤 PNG 纹理
    std::vector<IResourcePack*> m_resourcePacks;

    bool m_initialized = false;
};

} // namespace mc::skin
