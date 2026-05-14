#pragma once

#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/IInventory.hpp"
#include <memory>

namespace mc {

class PlayerInventory;
class IInventory;

namespace blockentity {
class HopperEntity;
}

/**
 * @brief 漏斗容器
 *
 * 管理漏斗GUI的槽位布局。
 *
 * 槽位布局：
 * - 漏斗：5格，1行5列
 * - 玩家背包：27格主背包 + 9格快捷栏
 *
 * 参考: net.minecraft.inventory.container.HopperContainer
 */
class HopperContainer : public AbstractContainerMenu {
public:
    /// 漏斗槽位数量
    static constexpr i32 HOPPER_SIZE = 5;

    /// 漏斗槽位起始X位置 (MC 1.16.5: 44)
    static constexpr i32 HOPPER_SLOT_START_X = 44;
    /// 漏斗槽位Y位置 (MC 1.16.5: 20)
    static constexpr i32 HOPPER_SLOT_Y = 20;

    /// 玩家背包起始Y位置 (MC 1.16.5: 51)
    static constexpr i32 PLAYER_INV_Y = 51;

    /// 快捷栏Y位置 (MC 1.16.5: 109)
    static constexpr i32 HOTBAR_Y = 109;

    /// 槽位宽度
    static constexpr i32 SLOT_SIZE = 18;

    // ========== 构造函数 ==========

    /**
     * @brief 构造漏斗容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param hopperInventory 漏斗背包
     * @param hopperEntity 漏斗实体（可选，用于距离检查）
     */
    HopperContainer(ContainerId id,
        PlayerInventory* playerInventory,
        IInventory* hopperInventory,
        blockentity::HopperEntity* hopperEntity = nullptr);

    /**
     * @brief 析构函数
     */
    ~HopperContainer() override = default;

    // ========== 容器接口 ==========

    /**
     * @brief 检查玩家是否仍可访问漏斗
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 容器内容变化时调用
     */
    void slotsChanged(IInventory* inventory) override;

    // ========== 属性访问 ==========

    /**
     * @brief 获取漏斗背包
     */
    [[nodiscard]] IInventory* getHopperInventory() const { return m_hopperInventory; }

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    /**
     * @brief 初始化槽位布局
     * @param playerInventory 玩家背包
     */
    void initSlots(PlayerInventory* playerInventory);

    IInventory* m_hopperInventory;             ///< 漏斗背包
    blockentity::HopperEntity* m_hopperEntity; ///< 漏斗实体（可选，用于距离检查）
};

} // namespace mc
