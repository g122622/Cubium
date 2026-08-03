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

#include "common/core/Types.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "util/math/random/Random.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <array>
#include <memory>
#include <optional>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class Player;
class ItemStack;

namespace crafting {
class CampfireCookingRecipe;
}

namespace blockentity {

/**
 * @brief 营火方块实体
 *
 * 营火可以同时烹饪最多4个食物物品。
 * 当点燃时，每个物品会逐渐烹饪完成并掉落。
 *
 * 状态属性：
 * - 4个物品槽位
 * - 每个槽位的烹饪时间
 * - 每个槽位的总烹饪时间
 */
class CampfireBlockEntity : public ContainerBlockEntity {
public:
    /// 营火槽位数量
    static constexpr i32 SLOT_COUNT = 4;

    /// 默认烹饪时间（tick）= 600 tick = 30秒
    static constexpr i32 DEFAULT_COOK_TIME = 600;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit CampfireBlockEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~CampfireBlockEntity() override = default;

    // ========== BlockEntity 接口 ==========

    /**
     * @brief 每tick更新
     * @param world 所在世界
     *
     * 点燃时烹饪食物，熄灭时冷却食物。
     */
    void tick(IWorld& world) override;

    /**
     * @brief 检查是否需要tick
     * @return 始终返回true，营火需要持续更新
     */
    [[nodiscard]] bool needsTick() const noexcept override { return true; }

    /**
     * @brief 从JSON加载数据
     * @param data JSON数据
     * @return 是否成功
     */
    bool load(const nlohmann::json& data) override;

    /**
     * @brief 保存数据到JSON
     * @param data 输出JSON数据
     */
    void save(nlohmann::json& data) const override;

    /**
     * @brief 创建副本
     * @return 副本的unique_ptr
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== ContainerBlockEntity 接口 ==========

    /**
     * @brief 获取容器
     * @return 容器指针
     */
    [[nodiscard]] IInventory* getInventory() noexcept override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const noexcept override { return &m_inventory; }

    /**
     * @brief 获取容器大小
     * @return 4个槽位
     */
    [[nodiscard]] i32 getContainerSize() const noexcept override { return SLOT_COUNT; }

    // ========== 营火特有方法 ==========

    /**
     * @brief 查找匹配的营火烹饪配方
     * @param stack 输入物品
     * @return 匹配的配方和烹饪时间，如果没有返回空
     *
     * 仅在有空槽位时查找配方。
     */
    [[nodiscard]] std::optional<std::pair<const crafting::CampfireCookingRecipe*, i32>> findMatchingRecipe(
        const ItemStack& stack) const;

    /**
     * @brief 添加物品开始烹饪
     * @param stack 物品堆（会被分割出1个）
     * @param cookTime 烹饪时间（tick）
     * @return 是否成功添加
     *
     * 找到第一个空槽位添加物品。
     * 创造模式传入副本，生存模式传入原物品。
     */
    bool addItem(ItemStack& stack, i32 cookTime);

    /**
     * @brief 掉落所有物品
     * @param world 世界引用
     *
     * 在营火被熄灭或破坏时调用。
     * 在营火位置生成物品实体。
     */
    void dropAllItems(IWorld& world);

    /**
     * @brief 清空所有槽位
     */
    void clear();

    /**
     * @brief 获取指定槽位的烹饪进度
     * @param slot 槽位索引 (0-3)
     * @return 烹饪进度 (0.0 - 1.0)，如果槽位为空返回0
     */
    [[nodiscard]] f32 getCookProgress(i32 slot) const noexcept;

    /**
     * @brief 获取指定槽位的烹饪时间
     * @param slot 槽位索引 (0-3)
     * @return 已烹饪时间（tick）
     */
    [[nodiscard]] i32 getCookTime(i32 slot) const noexcept { return _isValidSlot(slot) ? m_cookTimes[slot] : 0; }

    /**
     * @brief 获取指定槽位的总烹饪时间
     * @param slot 槽位索引 (0-3)
     * @return 总烹饪时间（tick）
     */
    [[nodiscard]] i32 getCookTimeTotal(i32 slot) const noexcept
    {
        return _isValidSlot(slot) ? m_cookTimesTotal[slot] : 0;
    }

private:
    /**
     * @brief 烹饪食物并掉落
     * @param world 世界引用
     *
     * 点燃时每tick调用，更新烹饪进度并掉落完成的食物。
     */
    void _cookAndDrop(IWorld& world);

    /**
     * @brief 冷却烹饪进度
     *
     * 熄灭时每tick调用，烹饪进度会缓慢下降。
     */
    void _coolDown();

    /**
     * @brief 标记方块实体已更改并通知客户端
     */
    void _inventoryChanged();

    /**
     * @brief 验证槽位索引
     * @param slot 槽位索引
     * @return 如果有效返回true
     */
    [[nodiscard]] bool _isValidSlot(i32 slot) const noexcept { return slot >= 0 && slot < SLOT_COUNT; }

    /// 物品库存（4个槽位）
    SimpleInventory m_inventory;

    /// 每个槽位的已烹饪时间
    std::array<i32, SLOT_COUNT> m_cookTimes{};

    /// 每个槽位的总烹饪时间
    std::array<i32, SLOT_COUNT> m_cookTimesTotal{};

    /// 随机数生成器
    mutable math::Random m_rng;
};

} // namespace blockentity
} // namespace mc
