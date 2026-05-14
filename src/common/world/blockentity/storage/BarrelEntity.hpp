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

#include "world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 木桶方块实体
 *
 * 木桶是一种存储容器，特点：
 * - 27格物品存储（与箱子相同）
 * - 可以在上方有方块时打开（与箱子不同）
 * - 没有双箱合并功能
 * - 可以面向任意六个方向放置
 * - 打开时改变方块状态
 * - 支持战利品表填充（继承自 LootableContainerBlockEntity）
 *
 * 参考: net.minecraft.tileentity.BarrelTileEntity
 */
class BarrelEntity : public LootableContainerBlockEntity {
public:
    /// 木桶容量（27格）
    static constexpr i32 BARREL_SIZE = 27;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit BarrelEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~BarrelEntity() override;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return BARREL_SIZE; }

    // ========== 木桶特有接口 ==========

    /**
     * @brief 获取打开计数
     * @return 当前打开的玩家数量
     */
    using ContainerBlockEntity::getOpenCount;

    /**
     * @brief 玩家打开木桶
     * @param player 打开木桶的玩家（可为nullptr）
     */
    void openContainer(Player* player) override;

    /**
     * @brief 玩家关闭木桶
     * @param player 关闭木桶的玩家（可为nullptr）
     */
    void closeContainer(Player* player) override;

    /**
     * @brief 计算红石比较器信号
     * @param world 世界引用
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorSignal(IWorld& world) const;

    // ========== 战利品表接口 ==========

    // 注：hasLootTable(), getLootTable(), getLootTableSeed(), setLootTable(), needsLootFill()
    // fillWithLoot() 继承自 LootableContainerBlockEntity，无需重写

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    [[nodiscard]] std::string getDefaultName() const override { return "container.barrel"; }

private:
    /**
     * @brief 更新方块状态（OPEN属性）
     * @param world 世界引用
     * @param open 是否打开
     */
    void updateBlockState(IWorld& world, bool open);

    SimpleInventory m_inventory; ///< 27格物品存储
    i32 m_ticksSinceSync = 0;    ///< 同步计数器
};

} // namespace blockentity
} // namespace mc
