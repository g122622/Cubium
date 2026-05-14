#pragma once

#include "resource/ResourceLocation.hpp"
#include "world/blockentity/core/LockableBlockEntity.hpp"
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
 * 当玩家首次访问容器内容时，自动从战利品表生成物品。
 *
 * 参考: net.minecraft.tileentity.LockableLootTileEntity
 *
 * 工作原理:
 * 1. 结构生成时设置 lootTable 和 lootTableSeed
 * 2. 玩家首次访问容器（isEmpty/getItem/setItem等）时自动填充
 * 3. 填充后清除 lootTable 标记，避免重复填充
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
    [[nodiscard]] bool hasLootTable() const { return m_hasLootTable; }

    /**
     * @brief 获取战利品表资源位置
     * @return 战利品表资源位置
     */
    [[nodiscard]] const ResourceLocation& getLootTable() const { return m_lootTable; }

    /**
     * @brief 获取战利品表种子
     * @return 种子值
     */
    [[nodiscard]] i64 getLootTableSeed() const { return m_lootTableSeed; }

    /**
     * @brief 设置战利品表
     *
     * 参考 MC 1.16.5: LockableLootTileEntity.setLootTable
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
    [[nodiscard]] bool needsLootFill() const { return m_hasLootTable; }

    // ========== 容器访问重写（自动触发 fillWithLoot）==========

    /**
     * @brief 检查容器是否为空
     *
     * MC 1.16.5: 在检查前自动填充战利品表
     */
    [[nodiscard]] bool isEmpty() const override;

    /**
     * @brief 玩家打开容器
     *
     * MC 1.16.5: 观察者模式玩家不能打开有战利品表的容器
     */
    void openContainer(Player* player) override;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

protected:
    /**
     * @brief 构造函数
     * @param type 方块实体类型
     * @param pos 方块位置
     */
    LootableContainerBlockEntity(BlockEntityType type, const BlockPos& pos);

    /**
     * @brief 填充战利品
     *
     * 如果设置了战利品表且尚未填充，则从战利品表生成物品并填充到容器中。
     * 该方法已实现完整逻辑，子类无需重写。
     *
     * 参考 MC 1.16.5: LockableLootTileEntity.fillWithLoot(PlayerEntity)
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
