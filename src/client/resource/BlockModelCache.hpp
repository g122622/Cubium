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

#include "client/renderer/MeshTypes.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace mc {

/**
 * @brief 方块模型缓存
 *
 * 连接 BlockRegistry 和 ResourceManager，缓存 BlockState -> BlockAppearance 映射。
 *
 * 使用示例:
 * @code
 * // 初始化
 * ResourceManager rm;
 * rm.addResourcePack(pack);
 * rm.loadAllResources();
 *
 * BlockModelCache cache;
 * cache.initialize(rm);
 *
 * // 获取方块外观
 * const BlockState* state = BlockRegistry::instance().getBlock(1)->defaultState();
 * const BlockAppearance* appearance = cache.getBlockAppearance(state);
 *
 * // 资源包变更后重建缓存
 * cache.rebuild();
 * @endcode
 */
class BlockModelCache {
public:
    /**
     * @brief 默认构造函数
     */
    BlockModelCache() = default;

    /**
     * @brief 析构函数
     */
    ~BlockModelCache() = default;

    // 禁止拷贝
    BlockModelCache(const BlockModelCache&) = delete;
    BlockModelCache& operator=(const BlockModelCache&) = delete;

    // 允许移动
    BlockModelCache(BlockModelCache&&) = default;
    BlockModelCache& operator=(BlockModelCache&&) = default;

    // ========================================================================
    // 初始化和重建
    // ========================================================================

    /**
     * @brief 初始化缓存
     *
     * 从 ResourceManager 加载所有方块的外观数据，构建状态 ID 到外观的映射。
     *
     * @param resourceManager 资源管理器
     * @return 是否成功
     */
    [[nodiscard]] bool initialize(ResourceManager& resourceManager);

    /**
     * @brief 重建缓存
     *
     * 当资源包变更后调用，重新加载所有方块外观。
     *
     * @param resourceManager 资源管理器
     * @return 是否成功
     */
    [[nodiscard]] bool rebuild(ResourceManager& resourceManager);

    /**
     * @brief 清除缓存
     */
    void clear();

    /**
     * @brief 检查缓存是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    // ========================================================================
    // 外观查询
    // ========================================================================

    /**
     * @brief 根据方块状态获取外观
     *
     * @param state 方块状态指针
     * @return 方块外观指针，找不到返回 nullptr
     */
    [[nodiscard]] const BlockAppearance* getBlockAppearance(const BlockState* state) const;

    /**
     * @brief 根据状态 ID 获取外观
     *
     * @param stateId 方块状态 ID
     * @return 方块外观指针，找不到返回 nullptr
     */
    [[nodiscard]] const BlockAppearance* getBlockAppearance(u32 stateId) const;

    /**
     * @brief 根据方块 ID 和属性获取外观
     *
     * @param blockId 方块 ID
     * @param properties 属性字符串（如 "axis=y,facing=north"）
     * @return 方块外观指针，找不到返回 nullptr
     */
    [[nodiscard]] const BlockAppearance* getBlockAppearance(u32 blockId, const std::string& properties) const;

    /**
     * @brief 获取缺失模型外观
     *
     * 当找不到方块模型时返回的默认外观。
     *
     * @return 缺失模型外观
     */
    [[nodiscard]] const BlockAppearance* getMissingAppearance() const;

    // ========================================================================
    // 统计
    // ========================================================================

    /**
     * @brief 获取缓存的外观数量
     */
    [[nodiscard]] size_t cachedAppearanceCount() const { return m_stateCache.size(); }

    /**
     * @brief 获取当前绑定的 ResourceManager
     */
    [[nodiscard]] ResourceManager* resourceManager() const { return m_resourceManager; }

    // ========================================================================
    // 纹理区域查询（供 ChunkMesher 液体路径等使用，查 AtlasManager 的图集 regions）
    // ========================================================================

    using RegionLookup = std::function<const TextureRegion*(const ResourceLocation&)>;

    /**
     * @brief 设置纹理区域查询回调
     *
     * 由 ClientApplication 在 AtlasManager 加载完图集后注入（通常绑定到
     * AtlasManager::findSpriteWithVariant）。ChunkMesher 液体面纹理区域查询经此回调。
     *
     * @param lookup 完整纹理资源位置 → UV 区域指针，未找到返回 nullptr
     */
    void setRegionLookup(RegionLookup lookup) { m_regionLookup = std::move(lookup); }

    /**
     * @brief 查询纹理区域
     * @param textureLocation 完整纹理资源位置
     * @return 区域指针，未找到返回 nullptr
     */
    [[nodiscard]] const TextureRegion* regionLookup(const ResourceLocation& textureLocation) const;

private:
    // 状态 ID -> 外观指针的缓存
    std::unordered_map<u32, const BlockAppearance*> m_stateCache;

    // 缺失模型外观
    std::unique_ptr<BlockAppearance> m_missingAppearance;

    // 资源管理器引用
    ResourceManager* m_resourceManager = nullptr;

    // 纹理区域查询回调（查 AtlasManager 的图集 regions）
    RegionLookup m_regionLookup;

    // 是否已初始化
    bool m_initialized = false;

    /**
     * @brief 构建状态缓存
     *
     * 遍历 BlockRegistry 中所有方块状态，从 ResourceManager 获取外观并缓存。
     */
    void _buildStateCache();

    /**
     * @brief 创建缺失模型外观
     */
    void _createMissingAppearance();
};

} // namespace mc
