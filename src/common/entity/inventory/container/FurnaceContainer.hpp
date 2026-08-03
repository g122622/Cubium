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

#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
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

    /// 窗口属性索引（对齐 vanilla AbstractFurnaceMenu，经 WindowPropertyPacket 同步）
    static constexpr i16 DATA_LIT_TIME = 0;           ///< 剩余燃烧时间（驱动火焰指示器）
    static constexpr i16 DATA_LIT_DURATION = 1;       ///< 总燃烧时间
    static constexpr i16 DATA_COOKING_PROGRESS = 2;   ///< 当前熔炼进度（驱动箭头）
    static constexpr i16 DATA_COOKING_TOTAL_TIME = 3; ///< 总熔炼时间

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
     * @brief 获取熔炉方块实体
     *
     * 返回关联的熔炉方块实体指针，用于读取燃烧/熔炼进度数据。
     * 在客户端侧可能为nullptr（当容器未关联到实际方块实体时）。
     */
    [[nodiscard]] AbstractFurnaceEntity* getFurnaceEntity() const { return m_furnaceEntity; }

    /**
     * @brief 燃烧进度（0.0~1.0），火焰指示器用
     *
     * 优先读 tracked int（客户端经 WindowPropertyPacket 同步），无 tracked int 时回退实体。
     */
    [[nodiscard]] f32 getLitProgress() const;
    /**
     * @brief 熔炼进度（0.0~1.0），箭头指示器用
     *
     * 优先读 tracked int（客户端经 WindowPropertyPacket 同步），无 tracked int 时回退实体。
     */
    [[nodiscard]] f32 getBurnProgress() const;

    /**
     * @brief 从熔炉实体刷新燃烧/熔炼进度到 tracked int 独立存储
     *
     * 服务端每 tick 在 broadcastChanges 前调用，把实体的 getBurnTime/getCookTime 等写入
     * tracked int 绑定的独立存储成员，detectAndSendChanges 检测变化经 WindowPropertyPacket 下推。
     * 客户端侧实体为 nullptr，调用为空操作。
     */
    void syncProgressFromEntity();

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
    void _initSlots(PlayerInventory* playerInventory);

    /**
     * @brief 绑定熔炉燃烧/熔炼进度到 tracked int
     *
     * tracked int 绑定到菜单内独立存储成员（m_dataXxx），不直接绑实体成员。
     * 服务端每 tick 由 syncProgressFromEntity 从实体刷新这些成员，
     * detectAndSendChanges 检测变化经 WindowPropertyPacket 下推；
     * 客户端经 setTrackedInt 写入这些成员（实体为 nullptr 时仍可持久化）。
     */
    void _initTrackedInts();

    /**
     * @brief 从输出槽取出物品时发放经验
     * @param extractedCount 取出的物品数量
     */
    void _grantExperienceForOutput(i32 extractedCount);

    IInventory* m_furnaceInventory;                      ///< 熔炉背包
    std::shared_ptr<IInventory> m_furnaceInventoryOwner; ///< 熔炉背包所有权（可选）
    AbstractFurnaceEntity* m_furnaceEntity;              ///< 熔炉实体（用于经验发放）
    Player* m_player = nullptr;                          ///< 玩家（用于经验发放）

    // 燃烧/熔炼进度的独立存储（tracked int 绑定到此，不直接绑实体）。
    // 服务端 tick 时由 syncProgressFromEntity 从实体刷新；客户端经 setTrackedInt 写入。
    i32 m_dataLitTime = 0;
    i32 m_dataLitDuration = 0;
    i32 m_dataCookingProgress = 0;
    i32 m_dataCookingTotalTime = 0;
};

} // namespace blockentity
} // namespace mc
