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

#include "BlockItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/world/block/Block.hpp"
#include <cstddef>
#include <functional>
#include <unordered_map>

namespace mc {

/**
 * @brief 方块物品注册表
 *
 * 管理方块与物品的映射关系。
 * 每个 BlockItem 在注册时会自动注册到 ItemRegistry。
 *
 * 参考: net.minecraft.item.ItemBLOCK_REGISTRY
 *
 * 用法示例:
 * @code
 * // 初始化方块物品
 * BlockItemRegistry::instance().initializeVanillaBlockItems();
 *
 * // 获取方块对应的物品
 * const BlockItem* item = BlockItemRegistry::instance().getBlockItem(stoneBlock);
 * if (item) {
 *     // 放置方块
 * }
 *
 * // 检查物品是否为方块物品
 * if (BlockItemRegistry::instance().isBlockItem(someItem)) {
 *     // ...
 * }
 * @endcode
 */
class BlockItemRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static BlockItemRegistry& instance() noexcept;

    // 禁止拷贝
    BlockItemRegistry(const BlockItemRegistry&) = delete;
    BlockItemRegistry& operator=(const BlockItemRegistry&) = delete;

    /**
     * @brief 注册方块物品
     *
     * 创建一个 BlockItem 并注册到 ItemRegistry 和 BlockItemRegistry。
     *
     * @param blockId 方块ID
     * @param item 物品指针（所有权转移）
     */
    void registerBlockItem(const Block& block, BlockItem& item);

    /**
     * @brief 根据方块ID获取方块物品
     * @param blockId 方块ID
     * @return 方块物品指针，不存在返回 nullptr
     */
    [[nodiscard]] const BlockItem* getBlockItem(u32 blockId) const noexcept;

    /**
     * @brief 根据物品ID获取方块物品
     * @param itemId 物品ID
     * @return 方块物品指针，不存在返回 nullptr
     */
    [[nodiscard]] const BlockItem* getBlockItemByItemId(ItemId itemId) const noexcept;

    /**
     * @brief 根据方块获取方块物品
     * @param block 方块引用
     * @return 方块物品指针，不存在返回 nullptr
     */
    [[nodiscard]] const BlockItem* getBlockItem(const Block& block) const noexcept;

    /**
     * @brief 根据物品获取对应方块
     * @param itemId 物品ID
     * @return 方块指针，如果不是方块物品返回 nullptr
     */
    [[nodiscard]] const Block* getBlock(ItemId itemId) const noexcept;

    /**
     * @brief 检查物品是否为方块物品
     * @param item 物品指针
     * @return 是否为方块物品
     */
    [[nodiscard]] bool isBlockItem(const Item* item) const noexcept;

    /**
     * @brief 检查物品ID是否为方块物品
     * @param itemId 物品ID
     * @return 是否为方块物品
     */
    [[nodiscard]] bool isBlockItem(ItemId itemId) const noexcept;

    /**
     * @brief 遍历所有方块物品
     * @param callback 回调函数
     */
    void forEachBlockItem(std::function<void(const BlockItem&)> callback) const;

    /**
     * @brief 获取方块物品数量
     */
    [[nodiscard]] size_t size() const noexcept { return m_blockToItem.size(); }

    /**
     * @brief 清空注册表
     *
     * 注意：这不会从 ItemRegistry 中移除物品。
     */
    void clear() noexcept;

    /**
     * @brief 初始化原版方块物品
     *
     * 注册所有原版方块对应的物品。
     */
    void initializeVanillaBlockItems();

private:
    BlockItemRegistry() = default;
    ~BlockItemRegistry() = default;

    // 方块ID -> BlockItem
    std::unordered_map<u32, const BlockItem*> m_blockToItem;

    // 物品ID -> 方块
    std::unordered_map<ItemId, const Block*> m_itemToBlock;

    // 物品ID -> BlockItem（快速查找）
    std::unordered_map<ItemId, const BlockItem*> m_itemIdToBlockItem;

    bool m_initialized = false;
};

} // namespace mc
