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

#include "item/core/ItemStack.hpp"
#include "resource/ResourceLocation.hpp"
#include "world/blockentity/core/LockableBlockEntity.hpp"
#include <functional>
#include <memory>

namespace mc {

class Player;
class ServerPlayer;
class IWorld;

namespace loot {
class LootTableManager;
}

namespace blockentity {

/**
 * @brief 可填充战利品表的容器方块实体基类
 *
 * 继承自 LockableBlockEntity，为容器提供战利品表自动填充功能。
 * 当首次访问容器内容时，自动从战利品表生成物品（延迟填充）。
 *
 * 工作原理:
 * 1. 结构生成时调用 setLootTable() 设置 lootTable 和 lootTableSeed
 * 2. 首次访问容器内容（isEmpty/getItem/setItem/openContainer等）时自动填充
 * 3. 填充后清除 lootTable 标记，避免重复填充（幂等性）
 *
 * 延迟填充机制（_unpackLootTable）:
 * - 所有容器内容访问方法在操作前调用 _unpackLootTable(nullptr)
 * - openContainer 调用 _unpackLootTable(player)，传入玩家上下文（幸运值等）
 * - _unpackLootTable 使用 const_cast 实现逻辑上的 const 语义，
 *   因为填充战利品是缓存初始化而非逻辑状态变更
 *
 * 参考: net.minecraft.RandomizableContainerBlockEntity
 *
 * 子类:
 * - ChestEntity（箱子）
 * - TrappedChestEntity（陷阱箱）
 * - BarrelEntity（木桶）
 * - DispenserBlockEntity（发射器/投掷器）
 * - ShulkerBoxEntity（潜影盒）
 */
class LootableContainerBlockEntity : public LockableBlockEntity {
public:
    // ========== 战利品表接口 ==========

    /**
     * @brief 检查是否有战利品表
     * @return 如果设置了战利品表返回true
     */
    [[nodiscard]] bool hasLootTable() const noexcept { return m_hasLootTable; }

    /**
     * @brief 获取战利品表资源位置
     * @return 战利品表资源位置
     */
    [[nodiscard]] const ResourceLocation& getLootTable() const noexcept { return m_lootTable; }

    /**
     * @brief 获取战利品表种子
     * @return 种子值
     */
    [[nodiscard]] i64 getLootTableSeed() const noexcept { return m_lootTableSeed; }

    /**
     * @brief 设置战利品表
     *
     * 结构生成时调用此方法设置战利品表。
     * 玩家首次访问容器内容时，物品将从战利品表生成。
     *
     * @param lootTable 战利品表资源位置
     * @param seed 随机种子（通常使用结构生成的随机数）
     */
    void setLootTable(const ResourceLocation& lootTable, i64 seed);

    /**
     * @brief 检查是否需要填充战利品
     * @return 如果设置了战利品表但尚未填充返回true
     */
    [[nodiscard]] bool needsLootFill() const noexcept { return m_hasLootTable; }

    // ========== 打开权限检查 ==========

    /**
     * @brief 检查玩家是否可以打开容器
     *
     * 重写 LockableBlockEntity::canOpen，增加观察者模式限制：
     * 当战利品表尚未填充时，观察者模式玩家不能打开容器，
     * 防止观察者触发战利品生成。
     *
     * @param player 玩家（可为nullptr）
     * @param heldItem 手持物品（用于钥匙匹配）
     * @return 如果可以打开返回true
     */
    [[nodiscard]] bool canOpen(const Player* player, const ItemStack& heldItem) const override;

    // ========== 容器访问重写（自动触发 _unpackLootTable）==========

    /**
     * @brief 检查容器是否为空
     *
     * 在检查物品之前会先触发战利品表填充（_unpackLootTable），
     * 确保未填充的战利品表不会被误判为空容器。
     * 参考: net.minecraft.RandomizableContainerBlockEntity.isEmpty()
     */
    [[nodiscard]] bool isEmpty() const override;

    /**
     * @brief 清空容器
     *
     * 在清空之前会先触发战利品表填充（_unpackLootTable），
     * 确保清空时战利品已被正确生成（而非丢失）。
     * 参考: net.minecraft.RandomizableContainerBlockEntity.clearContent()
     */
    void clearContainer() override;

    /**
     * @brief 玩家打开容器
     *
     * 当战利品表尚未填充时，观察者模式玩家不能打开容器。
     * 打开容器时会触发战利品表自动填充（带玩家上下文，包含幸运值等信息）。
     */
    void openContainer(Player* player) override;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    /**
     * @brief 从 NBT 加载战利品表引用与种子
     *
     * 读取 "LootTable"（string）与 "LootTableSeed"（long）键。
     * - "LootTable" 存在时设置 m_hasLootTable = true 并重置 m_lootFilled，
     *   使后续容器访问触发延迟填充。
     * - "LootTableSeed" 缺失时默认为 0（表示使用随机种子）。
     *
     * 子类（ChestEntity/BarrelEntity 等）应在其 loadFromNBT 重写中调用
     * 本方法以复用战利品表加载逻辑。
     *
     * @param tag NBT 复合标签
     * @return 是否成功
     */
    bool loadFromNBT(const nbt::CompoundTag& tag) override;

    /**
     * @brief 保存战利品表引用与种子到 NBT
     *
     * 仅在已设置战利品表且尚未填充时写入 "LootTable" / "LootTableSeed"。
     * 已填充后不写入（避免持久化已生成的物品与战利品表引用同时存在）。
     *
     * @param tag 输出 NBT 复合标签
     */
    void saveToNBT(nbt::CompoundTag& tag) const override;

protected:
    /**
     * @brief 构造函数
     * @param type 方块实体类型
     * @param pos 方块位置
     */
    LootableContainerBlockEntity(BlockEntityType type, const BlockPos& pos);

    /**
     * @brief 将容器物品列表序列化为 NBT "Items" 键
     *
     * 遍历容器中的非空槽位，将每个物品写入一个 compound_tag：
     *   - "Slot" (byte): 槽位索引
     *   - 物品字段（id/Count/tag）：由 ItemStack::toNbt 写入
     * 全部存入 compound_list_tag，键名为 "Items"。
     *
     * 调用方（子类 saveToNBT）应在确认无未解包的战利品表后调用本方法，
     * 与 MC Java 中 RandomizableContainer.saveAdditional 的互斥语义一致：
     * LootTable/LootTableSeed 与 Items 不会同时持久化。
     *
     * @param tag 输出 NBT 复合标签
     * @param inventory 要序列化的容器（通常为子类的 SimpleInventory）
     */
    void saveItemsToNBT(nbt::CompoundTag& tag, const IInventory& inventory) const;

    /**
     * @brief 从 NBT "Items" 键反序列化容器物品列表
     *
     * 读取 compound_list_tag 中的每个物品 compound，按 "Slot" 字段放置到容器。
     * 缺失 "Slot" 或超出容器范围时跳过该物品。
     * ItemStack::fromNbt 失败时跳过，不影响其余物品的加载。
     *
     * 调用前会清空容器。调用方（子类 loadFromNBT）应在确认 NBT 中不含
     * 战利品表引用后调用本方法，与 MC Java 中互斥语义一致。
     *
     * @param tag 源 NBT 复合标签
     * @param inventory 目标容器（通常为子类的 SimpleInventory）
     */
    void loadItemsFromNBT(const nbt::CompoundTag& tag, IInventory& inventory);

    /**
     * @brief 延迟填充战利品表
     *
     * 如果战利品表尚未填充，则触发填充。此方法可从 const 方法中调用，
     * 通过 const_cast 实现逻辑上的 const 语义（填充战利品是缓存初始化，
     * 而非逻辑状态变更）。
     *
     * 子类在实现 IInventory 接口方法（getItem/setItem/removeItem 等）时，
     * 应在操作前调用此方法，以确保战利品表已被填充。
     *
     * 参考: net.minecraft.RandomizableContainer.unpackLootTable(@Nullable Player)
     *
     * @param player 触发填充的玩家（可为nullptr，isEmpty/getItem等容器访问时传nullptr）
     */
    void _unpackLootTable(Player* player) const;

    /**
     * @brief 创建用于注入 SimpleInventory 的战利品表延迟填充回调
     *
     * 返回的回调可直接传给 SimpleInventory::setLootUnpackCallback()，
     * 使 SimpleInventory 的 isEmpty/getItem/setItem/removeItem/removeItemNoUpdate/clear
     * 在执行前自动触发 _unpackLootTable(nullptr)，确保所有容器访问路径都
     * 触发延迟填充。
     *
     * 子类应在构造 SimpleInventory 后调用此方法注入回调，例如:
     * @code
     * BarrelEntity::BarrelEntity(const BlockPos& pos)
     *     : LootableContainerBlockEntity(BlockEntityType::Barrel, pos)
     *     , m_inventory(BARREL_SIZE)
     * {
     *     m_inventory.setLootUnpackCallback(_makeLootUnpackCallback());
     * }
     * @endcode
     *
     * @return 返回的 std::function 持有 this 指针的引用，回调必须在*this 存活期间使用。
     *         子类移动构造/移动赋值后应重新调用此方法刷新回调（this 指针变化）。
     */
    std::function<void()> _makeLootUnpackCallback() const
    {
        return [this]() { _unpackLootTable(nullptr); };
    }

    /**
     * @brief 填充战利品
     *
     * 如果设置了战利品表且尚未填充，则从战利品表生成物品并填充到容器中。
     * 该方法已实现完整逻辑，子类无需重写。
     *
     * @param player 触发填充的玩家（可为nullptr）
     */
    void fillWithLoot(Player* player);

    /**
     * @brief 填充战利品（使用指定的战利品表管理器）
     *
     * 用于服务器环境，传入战利品表管理器。
     *
     * @param lootTableManager 战利品表管理器
     * @param player 触发填充的玩家（可为nullptr）
     * @return 是否成功填充
     */
    bool fillWithLootFromTable(loot::LootTableManager& lootTableManager, Player* player);

private:
    bool m_hasLootTable = false;       ///< 是否设置了战利品表
    ResourceLocation m_lootTable;      ///< 战利品表资源位置
    i64 m_lootTableSeed = 0;           ///< 战利品表种子
    mutable bool m_lootFilled = false; ///< 是否已填充（mutable 用于 const 方法）
};

} // namespace blockentity
} // namespace mc
