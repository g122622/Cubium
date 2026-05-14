#pragma once

#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/IInventory.hpp"
#include <memory>

namespace mc {

class PlayerInventory;

namespace blockentity {

// 前向声明
class AbstractFurnaceEntity;

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
class FurnaceContainer : public AbstractContainerMenu {
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
     * @param furnaceEntity 熔炉实体（可选，用于经验发放）
     */
    FurnaceContainer(ContainerId id,
        PlayerInventory* playerInventory,
        IInventory* furnaceInventory,
        AbstractFurnaceEntity* furnaceEntity = nullptr);

    /**
     * @brief 构造拥有熔炉背包的容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param furnaceInventoryOwner 熔炉背包所有权
     * @param furnaceEntity 熔炉实体（可选，用于经验发放）
     */
    FurnaceContainer(ContainerId id,
        PlayerInventory* playerInventory,
        std::shared_ptr<IInventory> furnaceInventoryOwner,
        AbstractFurnaceEntity* furnaceEntity = nullptr);

    /**
     * @brief 析构函数
     */
    ~FurnaceContainer() override = default;

    // ========== 属性访问 ==========

    /**
     * @brief 获取熔炉背包
     */
    [[nodiscard]] IInventory* getFurnaceInventory() const { return m_furnaceInventory; }

    /**
     * @brief 设置玩家引用（用于经验发放）
     * @param player 玩家指针
     */
    void setPlayer(Player* player) { m_player = player; }

    /**
     * @brief 获取累积的烧炼经验
     * @return 累积的经验值
     */
    [[nodiscard]] f32 getStoredExperience() const;

    /**
     * @brief 提取并清除累积的烧炼经验
     * @return 提取的经验值
     */
    f32 extractStoredExperience();

    /**
     * @brief 检查玩家是否仍可访问熔炉
     * @param player 玩家
     * @return 是否仍可访问
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    // ========== 快速移动 ==========

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    /**
     * @brief 初始化槽位布局
     * @param playerInventory 玩家背包
     */
    void initSlots(PlayerInventory* playerInventory);

    /**
     * @brief 从输出槽取出物品时发放经验
     * @param extractedCount 取出的物品数量
     */
    void grantExperienceForOutput(i32 extractedCount);

    IInventory* m_furnaceInventory;                      ///< 熔炉背包
    std::shared_ptr<IInventory> m_furnaceInventoryOwner; ///< 熔炉背包所有权（可选）
    AbstractFurnaceEntity* m_furnaceEntity;              ///< 熔炉实体（用于经验发放）
    Player* m_player = nullptr;                          ///< 玩家（用于经验发放）
};

} // namespace blockentity
} // namespace mc
