#pragma once

#include "util/math/random/Random.hpp"
#include "world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"

namespace mc {

namespace loot {
class LootTableManager;
}

namespace blockentity {

/**
 * @brief 发射器/投掷器方块实体基类
 *
 * 提供9格物品存储和随机选择物品发射/投掷的功能。
 * 继承自 LootableContainerBlockEntity 以支持战利品表填充。
 *
 * 参考: net.minecraft.tileentity.DispenserTileEntity
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
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 容器接口 ==========

    [[nodiscard]] i32 getContainerSize() const override { return INVENTORY_SIZE; }
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
     * MC 1.16.5: 在选择槽位前会先填充战利品表
     *
     * @return 非空槽位索引，如果没有返回 -1
     */
    [[nodiscard]] i32 getDispenseSlot();

    /**
     * @brief 向库存添加物品
     *
     * MC 1.16.5 实现：查找第一个空槽位，将整个物品放入该槽位。
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
