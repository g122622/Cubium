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

#include "../../../../../core/Types.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"

#include <optional>
#include <unordered_set>

namespace mc {

class CopperGolemEntity;
class IWorld;
class IInventory;
class BlockState;

namespace blockentity {
class ChestEntity;
}

namespace test {
class TransportItemsBetweenContainersGoalTestAccessor; // 测试访问器
} // namespace test

namespace entity::ai::goal {

/**
 * @brief 铜傀儡物品运输目标
 *
 * 对应 MC 1.21.11 net.minecraft.world.entity.ai.behavior.TransportItemsBetweenContainers
 * （Brain Behavior），本项目适配为 GoalSelector 体系下的 Goal。
 *
 * 行为：
 * - 主手为空时（拾取模式）：寻找附近的铜箱子（BlockTags::COPPER_CHESTS），
 *   到达后打开容器、取出最多 16 个物品到主手。
 * - 主手有物品时（放置模式）：寻找附近的普通箱子或陷阱箱，
 *   到达后打开容器、将主手物品放入容器（先空槽后可堆叠槽）。
 *
 * 状态机：
 * - TRAVELLING：正在寻路前往目标方块
 * - QUEUING：目标被其他实体占用，排队等待（对应 MC shouldQueueForTarget）
 * - INTERACTING：到达目标，执行 60 tick 的开盖-转移-关盖交互序列
 *
 * 关键时序（对应 MC onReachedTargetInteraction）：
 * - ticks == 1：container.startOpen(coppergolem)、setOpenedChestPos(pos)、setState(动画状态)
 * - ticks == 9：播放对应音效（ITEM_GET / ITEM_NO_GET / ITEM_DROP / ITEM_NO_DROP）
 * - ticks == 60：执行物品转移、container.stopOpen(coppergolem)、clearOpenedChestPos
 *
 * 冷却：无目标或完成一次交互后进入 IDLE_COOLDOWN（140 tick）冷却。
 *
 * 记忆：visitedPositions（已访问位置，避免重复）与 unreachablePositions（不可达位置）。
 *
 * 参考: net.minecraft.world.entity.ai.behavior.TransportItemsBetweenContainers (MC 1.21.11)
 *       net.minecraft.world.entity.animal.golem.CopperGolemAi (MC 1.21.11)
 */
class TransportItemsBetweenContainersGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param golem 拥有此目标的铜傀儡
     * @param speedMultiplier 移动速度倍率（MC 中为 1.0F）
     */
    TransportItemsBetweenContainersGoal(CopperGolemEntity* golem, f64 speedMultiplier);

    ~TransportItemsBetweenContainersGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "TransportItemsBetweenContainersGoal"; }

private:
    /**
     * @brief 运输状态
     *
     * 对应 MC TransportItemsBetweenContainers 内部状态机。
     */
    enum class TransportState : u8 {
        Travelling = 0, ///< 正在寻路前往目标
        Queuing = 1,    ///< 目标被占用，排队等待
        Interacting = 2 ///< 到达目标，执行交互序列
    };

    /**
     * @brief 交互阶段（到达目标后的 60 tick 序列）
     *
     * 对应 MC onReachedTargetInteraction 的 ticks==1/9/60 三个关键点。
     * 物品转移在 ticks==60 执行。
     */
    enum class InteractionPhase : u8 {
        StartOpen = 0,       ///< ticks 1：打开容器、记录位置、设置动画状态
        PlaySound = 1,       ///< ticks 9：播放音效
        TransferAndClose = 2 ///< ticks 60：执行物品转移、关闭容器、清除位置
    };

    /**
     * @brief 判断铜傀儡是否处于拾取模式
     *
     * 对应 MC TransportItemsBetweenContainers.isPickingUpItems(mob):
     *   return mob.getMainHandItem().isEmpty();
     *
     * @return 主手为空返回 true（拾取模式），否则为放置模式
     */
    [[nodiscard]] bool _isPickingUpItems() const;

    /**
     * @brief 检查方块状态是否为有效的运输目标
     *
     * 拾取模式：方块必须在 BlockTags::COPPER_CHESTS 标签中（源容器）
     * 放置模式：方块必须是 CHEST 或 TRAPPED_CHEST（目标容器）
     *
     * @param state 方块状态
     * @return 是否为有效目标
     */
    [[nodiscard]] bool _isValidTargetBlock(const BlockState& state) const;

    /**
     * @brief 搜索运输目标
     *
     * 对应 MC TransportItemsBetweenContainers.getTransportTarget：
     * 在水平半径 32、垂直半径 8 的范围内搜索第一个满足 _isValidTargetBlock
     * 且未被 visited/unreachable 标记、可达的 ChestBlockEntity。
     *
     * @return 是否找到有效目标
     */
    [[nodiscard]] bool _searchForTarget();

    /**
     * @brief 获取目标方块对应的 ChestEntity
     *
     * 若目标位置是双箱的一部分，返回双箱中 m_destinationBlock 对应的 ChestEntity
     * （不返回合并的 DoubleSidedInventory，因为 startOpen/stopOpen 需要在
     * 两个 ChestEntity 上分别调用以同步动画）。
     *
     * @return ChestEntity 指针，无效返回 nullptr
     */
    [[nodiscard]] class blockentity::ChestEntity* _getTargetChestEntity() const;

    /**
     * @brief 检查铜傀儡是否已到达目标方块附近
     *
     * 对应 MC TransportItemsBetweenContainers.onTravelToTarget 中的
     * "Interacting 进入判定"：isWithinTargetDistance(getInteractionRange(mob), target, level, mob, getCenterPos(mob))。
     *
     * getInteractionRange(mob) 在路径完成时返回 1.0、未完成时返回 0.5。
     * 判定方式为 AABB 相交测试（考虑目标方块碰撞箱 + 距离膨胀），与 MC 1.21.11 一致。
     *
     * @return 是否已到达
     */
    [[nodiscard]] bool _hasReachedTarget() const;

    /**
     * @brief 检查铜傀儡是否已进入"可开始排队"距离
     *
     * 对应 MC TransportItemsBetweenContainers.onTravelToTarget 中的
     * "Queuing 进入判定"：isWithinTargetDistance(3.0, target, level, mob, getCenterPos(mob))。
     * 当实体进入此距离且目标被其他 ContainerUser 占用时会进入 Queuing 状态。
     *
     * @return 是否在排队距离内
     */
    [[nodiscard]] bool _isWithinQueuingDistance() const;

    /**
     * @brief 检查铜傀儡是否仍处于"可继续交互"距离
     *
     * 对应 MC TransportItemsBetweenContainers.onReachedTarget 中的
     * "Interacting 保持判定"：isWithinTargetDistance(2.0, target, level, mob, getCenterPos(mob))。
     * 若不再满足则中断交互、回到 Travelling。
     *
     * @return 是否仍在交互距离内
     */
    [[nodiscard]] bool _isWithinContinueInteractingDistance() const;

    /**
     * @brief MC isWithinTargetDistance 的复刻
     *
     * 算法：
     * 1. 取铜傀儡当前 boundingBox 的 X/Y/Z 尺寸
     * 2. 以 center 为中心、铜傀儡尺寸为各轴尺寸构造 mobSideAABB
     *    （对应 MC AABB.ofSize(center, xsize, ysize, zsize)）
     * 3. 取目标方块碰撞箱的包围盒（方块本地坐标 [0,1] 范围）
     * 4. X/Z 轴膨胀 distance、Y 轴膨胀 0.5
     * 5. 平移到目标方块的世界坐标
     * 6. 与 mobSideAABB 做严格开区间相交测试
     *
     * @param distance 水平方向膨胀量（X/Z 轴）
     * @param center 构造铜傀儡侧 AABB 的中心点（通常为铜傀儡中心位置）
     * @return 是否在目标距离内
     */
    [[nodiscard]] bool _isWithinTargetDistance(f64 distance, const Vector3& center) const;

    /**
     * @brief 获取铜傀儡的"中心位置"
     *
     * 对应 MC TransportItemsBetweenContainers.getCenterPos(mob)：
     *   setMiddleYPosition(mob, mob.position())
     * 即铜傀儡脚底位置 + Y 方向上移包围盒高度的一半。
     *
     * @return 中心位置（X/Z 为脚底位置，Y 为包围盒中心高度）
     */
    [[nodiscard]] Vector3 _getCenterPos() const;

    /**
     * @brief 获取当前交互距离阈值
     *
     * 对应 MC TransportItemsBetweenContainers.getInteractionRange(mob)：
     *   return hasFinishedPath(mob) ? 1.0 : 0.5;
     * 其中 hasFinishedPath 等价于 navigator.getPath() != null && path.isDone()。
     *
     * 路径完成时返回 1.0（CLOSE_ENOUGH_TO_START_INTERACTING_WITH_TARGET_PATH_END_DISTANCE），
     * 路径未完成时返回 0.5（CLOSE_ENOUGH_TO_START_INTERACTING_WITH_TARGET_DISTANCE）。
     *
     * @return 交互距离阈值
     */
    [[nodiscard]] f64 _getInteractionRange() const;

    /**
     * @brief 检查目标容器是否被其他 ContainerUser 占用
     *
     * 对应 MC TransportItemsBetweenContainers.isAnotherMobInteractingWithTarget：
     *   getConnectedTargets(target, level).anyMatch(shouldQueueForTarget)
     * 其中 shouldQueueForTarget 检查目标（或其双箱连通位置）的 ChestBlockEntity
     * 的 openersCounter.getEntitiesWithContainerOpen() 是否非空。
     *
     * 本项目实现：遍历目标位置（及双箱另一半位置）附近的 ContainerUser 实体，
     * 排除自身后检查是否有任何 ContainerUser.hasContainerOpen(targetPos) 为 true。
     *
     * @return 是否有其他实体正在与目标容器交互
     */
    [[nodiscard]] bool _isAnotherMobInteractingWithTarget() const;

    /**
     * @brief 开始寻路前往目标方块
     */
    void _startTravelling();

    /**
     * @brief 进入排队等待状态
     *
     * 对应 MC TransportItemsBetweenContainers.startQueuing：
     *   stopInPlace(mob); setTransportingState(QUEUING);
     * 停止寻路并停留在原地，等待目标容器空闲。
     */
    void _startQueuing();

    /**
     * @brief 恢复寻路（从排队状态返回旅行状态）
     *
     * 对应 MC TransportItemsBetweenContainers.resumeTravelling：
     *   setTransportingState(TRAVELLING); walkTowardsTarget(mob);
     */
    void _resumeTravelling();

    /**
     * @brief 进入交互状态
     *
     * 重置 m_interactionTicks 为 0，进入 INTERACTING 状态。
     */
    void _startInteracting();

    /**
     * @brief 执行交互序列（每 tick 调用）
     *
     * 对应 MC onReachedTargetInteraction：
     * - m_interactionTicks == 1：startOpen + setOpenedChestPos + setState
     * - m_interactionTicks == 9：playSound
     * - m_interactionTicks == 60：物品转移 + stopOpen + clearOpenedChestPos
     */
    void _tickInteracting();

    /**
     * @brief 从容器中拾取物品到铜傀儡主手
     *
     * 对应 MC TransportItemsBetweenContainers.pickupItemFromContainer：
     *   遍历容器找到第一个非空槽，取 min(count, 16) 个。
     *   调用 removeItem(slot, count) 从容器移除。
     *   setItemSlot(MAINHAND, picked)。
     *   setGuaranteedDrop(MAINHAND)。
     *
     * @param container 容器库存
     */
    void _pickupItemFromContainer(IInventory& container);

    /**
     * @brief 将铜傀儡主手物品放入容器
     *
     * 对应 MC TransportItemsBetweenContainers.addItemsToContainer：
     *   遍历容器，先找空槽直接整堆放入；再找可堆叠槽增量堆叠。
     *   剩余物品留在主手。
     *
     * @param container 容器库存
     */
    void _addItemsToContainer(IInventory& container);

    /**
     * @brief 根据交互结果设置 CopperGolemState 动画状态
     *
     * 对应 MC onReachedTargetInteraction 的四个 ContainerInteractionState：
     * - 拾取成功 → GettingItem
     * - 拾取失败（容器空）→ GettingNoItem
     * - 放置成功 → DroppingItem
     * - 放置失败（容器满）→ DroppingNoItem
     *
     * @param success 交互是否成功（取出/放入了物品）
     */
    void _setAnimationState(bool success);

    /**
     * @brief 播放交互音效
     *
     * 对应 MC onReachedTargetInteraction 的 ticks==9 音效播放：
     * - 拾取模式 → COPPER_GOLEM_ITEM_GET（成功）或 COPPER_GOLEM_ITEM_NO_GET（失败）
     * - 放置模式 → COPPER_GOLEM_ITEM_DROP（成功）或 COPPER_GOLEM_ITEM_NO_DROP（失败）
     *
     * @param success 交互是否成功
     */
    void _playInteractionSound(bool success);

    /**
     * @brief 进入冷却
     *
     * 对应 MC IDLE_COOLDOWN = 140。
     * 无目标或完成一次交互后设置 m_cooldown = IDLE_COOLDOWN。
     */
    void _enterCooldown();

    /**
     * @brief 重置运输状态（保留冷却和访问记录）
     *
     * 清除 m_destinationBlock、m_state、m_interactionTicks，取消寻路，
     * 清除铜傀儡的 openedChestPos 并重置行为状态为 Idle。
     */
    void _resetTransportState();

    CopperGolemEntity* m_golem;
    f64 m_speedMultiplier;

    /// 当前运输状态
    TransportState m_state = TransportState::Travelling;

    /// 目标方块位置
    std::optional<BlockPos> m_destinationBlock;

    /// 交互 tick 计数器（0 到 TARGET_INTERACTION_TIME）
    i32 m_interactionTicks = 0;

    /// 交互是否成功（用于决定音效和动画状态）
    bool m_interactionSuccess = false;

    /// 冷却计数器（>0 时 shouldExecute 返回 false）
    i32 m_cooldown = 0;

    /// 已访问的方块位置（避免重复访问同一箱子）
    std::unordered_set<i64> m_visitedPositions;

    /// 不可达的方块位置（寻路失败时记录，避免重复尝试）
    std::unordered_set<i64> m_unreachablePositions;

    // ========== 常量（对应 MC TransportItemsBetweenContainers） ==========

    /// 交互持续时间（tick），对应 MC TARGET_INTERACTION_TIME = 60
    static constexpr i32 TARGET_INTERACTION_TIME = 60;

    /// 单次运输物品最大堆叠数，对应 MC TRANSPORTED_ITEM_MAX_STACK_SIZE = 16
    static constexpr i32 TRANSPORTED_ITEM_MAX_STACK_SIZE = 16;

    /// 空闲冷却（tick），对应 MC IDLE_COOLDOWN = 140
    static constexpr i32 IDLE_COOLDOWN = 140;

    /// 水平搜索半径，对应 MC TRANSPORT_ITEM_HORIZONTAL_SEARCH_RADIUS = 32
    static constexpr i32 HORIZONTAL_SEARCH_RADIUS = 32;

    /// 垂直搜索半径，对应 MC TRANSPORT_ITEM_VERTICAL_SEARCH_RADIUS = 8
    static constexpr i32 VERTICAL_SEARCH_RADIUS = 8;

    /// 已访问位置上限，对应 MC MAX_VISITED_POSITIONS = 10
    static constexpr i32 MAX_VISITED_POSITIONS = 10;

    /// 不可达位置上限，对应 MC MAX_UNREACHABLE_POSITIONS = 50
    static constexpr i32 MAX_UNREACHABLE_POSITIONS = 50;

    /// 路径未完成时的交互距离阈值，对应 MC CLOSE_ENOUGH_TO_START_INTERACTING_WITH_TARGET_DISTANCE = 0.5
    static constexpr f64 CLOSE_ENOUGH_TO_START_INTERACTING_DISTANCE = 0.5;

    /// 路径完成时的交互距离阈值，对应 MC CLOSE_ENOUGH_TO_START_INTERACTING_WITH_TARGET_PATH_END_DISTANCE = 1.0
    static constexpr f64 CLOSE_ENOUGH_TO_START_INTERACTING_WITH_TARGET_PATH_END_DISTANCE = 1.0;

    /// 进入排队距离阈值，对应 MC CLOSE_ENOUGH_TO_START_QUEUING_DISTANCE = 3.0
    static constexpr f64 CLOSE_ENOUGH_TO_START_QUEUING_DISTANCE = 3.0;

    /// 继续交互距离阈值，对应 MC CLOSE_ENOUGH_TO_CONTINUE_INTERACTING_WITH_TARGET = 2.0
    static constexpr f64 CLOSE_ENOUGH_TO_CONTINUE_INTERACTING_WITH_TARGET = 2.0;

    /// isWithinTargetDistance 中 Y 轴固定膨胀量（MC 硬编码常量）
    static constexpr f64 TARGET_DISTANCE_Y_INFLATE = 0.5;

    /// tick==1：开始交互
    static constexpr i32 TICK_TO_START_INTERACTION = 1;
    /// tick==9：播放音效
    static constexpr i32 TICK_TO_PLAY_SOUND = 9;
    /// tick==60：结束交互（转移物品 + 关闭容器）
    static constexpr i32 TICK_TO_END_INTERACTION = 60;

    friend class test::TransportItemsBetweenContainersGoalTestAccessor;
};

} // namespace entity::ai::goal
} // namespace mc
