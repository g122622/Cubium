#pragma once

#include "world/blockentity/core/LockableBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "util/math/random/Random.hpp"

namespace mc {
namespace blockentity {

/**
 * @brief 发射器/投掷器方块实体基类
 *
 * 提供9格物品存储和随机选择物品发射/投掷的功能。
 *
 * 参考: net.minecraft.tileentity.DispenserTileEntity
 */
class DispenserBlockEntity : public LockableBlockEntity {
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

    // ========== 容器接口 ==========

    [[nodiscard]] i32 getContainerSize() const override { return INVENTORY_SIZE; }
    [[nodiscard]] bool isEmpty() const override;
    void clearContainer() override;

    // ========== IInventory 接口 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }

    // ========== 发射器特有方法 ==========

    /**
     * @brief 随机选择一个非空槽位
     * @return 非空槽位索引，如果没有返回 -1
     */
    [[nodiscard]] i32 getRandomSlot();

    /**
     * @brief 获取发射槽位（MC储水池采样算法）
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
     * 尝试将物品添加到库存中的空槽位或与现有堆叠合并。
     *
     * @param stack 要添加的物品堆
     * @return 剩余未添加的物品堆（如果全部添加成功则为空）
     */
    ItemStack addItemStack(ItemStack stack);

    /**
     * @brief 设置战利品表
     * @param lootTable 战利品表ID
     * @param seed 随机种子
     */
    void setLootTable(const String& lootTable, u64 seed = 0);

    /**
     * @brief 获取战利品表ID
     */
    [[nodiscard]] const String& getLootTable() const { return m_lootTable; }

    /**
     * @brief 获取战利品表种子
     */
    [[nodiscard]] u64 getLootTableSeed() const { return m_lootTableSeed; }

    /**
     * @brief 是否有战利品表
     */
    [[nodiscard]] bool hasLootTable() const { return !m_lootTable.empty(); }

protected:
    /// 库存大小（9格）
    static constexpr i32 INVENTORY_SIZE = 9;

    /**
     * @brief 获取默认显示名称
     */
    [[nodiscard]] String getDefaultName() const override { return "container.dispenser"; }

    /// 库存
    SimpleInventory m_inventory;

    /// 随机数生成器
    mutable math::Random m_rng;

    /// 战利品表ID（用于生成内容）
    String m_lootTable;

    /// 战利品表随机种子
    u64 m_lootTableSeed = 0;
};

} // namespace blockentity
} // namespace mc
