#pragma once

#include "entity/inventory/Container.hpp"
#include "entity/inventory/IInventory.hpp"
#include <memory>

namespace mc {

class PlayerInventory;

namespace blockentity {

/**
 * @brief 熔炉容器
 *
 * 管理熔炉GUI的槽位布局。
 *
 * 槽位布局：
 * - 熔炉输入槽：1格（顶部）
 * - 熔炉燃料槽：1格（中部）
 * - 熔炉输出槽：1格（底部）
 * - 玩家背包：27格主背包 + 9格快捷栏
 *
 * 参考: net.minecraft.inventory.container.FurnaceContainer
 */
class FurnaceContainer : public Container {
public:
    /// 熔炉输入槽索引
    static constexpr i32 SLOT_INPUT = 0;
    /// 熔炉燃料槽索引
    static constexpr i32 SLOT_FUEL = 1;
    /// 熔炉输出槽索引
    static constexpr i32 SLOT_OUTPUT = 2;
    /// 熔炉槽位数量
    static constexpr i32 FURNACE_SLOTS = 3;

    /// 熔炉槽位起始Y位置
    static constexpr i32 FURNACE_SLOT_Y = 17;
    /// 玩家背包起始Y位置
    static constexpr i32 PLAYER_INV_Y = 84;
    /// 快捷栏Y位置
    static constexpr i32 HOTBAR_Y = 142;
    /// 槽位宽度
    static constexpr i32 SLOT_SIZE = 18;

    // ========== 构造函数 ==========

    /**
     * @brief 构造熔炉容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param furnaceInventory 熔炉背包
     */
    FurnaceContainer(ContainerId id,
                     PlayerInventory* playerInventory,
                     IInventory* furnaceInventory);

    /**
     * @brief 析构函数
     */
    ~FurnaceContainer() override = default;

    // ========== 属性访问 ==========

    /**
     * @brief 获取熔炉背包
     */
    [[nodiscard]] IInventory* getFurnaceInventory() const { return m_furnaceInventory; }

    // ========== 快速移动 ==========

protected:
    ItemStack doQuickMove(i32 slotIndex, ItemStack cursorItem) override;

private:
    /**
     * @brief 初始化槽位布局
     * @param playerInventory 玩家背包
     */
    void initSlots(PlayerInventory* playerInventory);

    IInventory* m_furnaceInventory;  ///< 熔炉背包
};

} // namespace blockentity
} // namespace mc
