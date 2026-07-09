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

#include "util/math/random/Random.hpp"
#include "world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc {
namespace blockentity {

/**
 * @brief 发射器/投掷器方块实体基类
 *
 * 提供9格物品存储和随机选择物品发射/投掷的功能。
 * 继承自 LootableContainerBlockEntity 以支持战利品表填充。
 */
class DispenserBlockEntity : public LootableContainerBlockEntity {
public:
    /**
     * @brief 构造函数
     * @param type 方块实体类型
     * @param pos 位置
     */
    DispenserBlockEntity(BlockEntityType type, const BlockPos& pos);

    // ========== BlockEntity 接口 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    /**
     * @brief 从 NBT 加载（结构模板 / 客户端同步）
     *
     * 调用基类处理战利品表引用后，若无未解包的战利品表则加载容器物品列表。
     * LootTable 与 Items 互斥，与 MC Java RandomizableContainer 一致。
     */
    bool loadFromNBT(const nbt::CompoundTag& tag) override;

    /**
     * @brief 保存到 NBT（结构模板 / 客户端同步）
     *
     * 调用基类处理战利品表引用后，若无未解包的战利品表则保存容器物品列表。
     */
    void saveToNBT(nbt::CompoundTag& tag) const override;

    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 容器接口 ==========

    [[nodiscard]] i32 getContainerSize() const override { return INVENTORY_SIZE; }
    void clearContainer() override;

    // ========== IInventory 接口 ==========

    // 注：getInventory() 返回 SimpleInventory*，SimpleInventory 已通过
    // setLootUnpackCallback() 注入战利品表延迟填充回调，因此所有通过
    // getInventory()->getItem/setItem/removeItem/clear 等路径访问容器内容
    // 都会自动触发 _unpackLootTable(nullptr)。这与 MC Java 中
    // RandomizableContainerBlockEntity 的行为一致。
    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }

    // ========== 发射器特有方法 ==========

    /**
     * @brief 随机选择一个非空槽位
     * @return 非空槽位索引，如果没有返回 -1
     */
    [[nodiscard]] i32 getRandomSlot();

    /**
     * @brief 获取发射槽位（储水池采样算法）
     *
     * 选择第一个非空槽位，然后以 1/n 概率替换（n为已遍历的非空槽位数）。
     * 这确保每个非空槽位被选中的概率相等。
     *
     * @return 非空槽位索引，如果没有返回 -1
     */
    [[nodiscard]] i32 getDispenseSlot();

    /**
     * @brief 向库存添加物品
     *
     * 查找第一个空槽位，将整个物品放入该槽位。
     * 不尝试与现有堆叠合并。
     *
     * @param stack 要添加的物品堆
     * @return 物品被放入的槽位索引，如果没有空槽位返回 -1
     */
    i32 addItemStack(const ItemStack& stack);

    // ========== 战利品表接口 ==========

    // 注：hasLootTable(), getLootTable(), getLootTableSeed(), setLootTable(), needsLootFill()
    // fillWithLoot() 继承自 LootableContainerBlockEntity，无需重写

protected:
    /// 库存大小（9格）
    static constexpr i32 INVENTORY_SIZE = 9;

    /**
     * @brief 获取默认显示名称
     */
    [[nodiscard]] std::string getDefaultName() const override { return "container.dispenser"; }

    /// 库存
    SimpleInventory m_inventory;

    /// 随机数生成器
    mutable math::Random m_rng;
};

} // namespace blockentity
} // namespace mc
