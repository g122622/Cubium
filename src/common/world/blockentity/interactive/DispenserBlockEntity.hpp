#pragma once

#include "world/blockentity/core/LockableBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <random>

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
    mutable std::mt19937 m_rng;
};

} // namespace blockentity
} // namespace mc
