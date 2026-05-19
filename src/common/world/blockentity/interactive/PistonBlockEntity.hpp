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

#include "../../../util/Direction.hpp"
#include "../../block/Block.hpp"
#include "../BlockEntity.hpp"
#include <memory>

namespace mc {

class BlockState;

namespace blockentity {

/**
 * @brief 活塞方块实体
 *
 * 用于处理活塞移动过程中的动画和实体推动。
 * 当活塞伸出或收回时，会创建此方块实体来处理：
 * - 移动动画进度（0.0 - 1.0）
 * - 推动实体
 * - 完成后替换为最终方块
 *
 * @note m_pistonState 为非拥有指针，指向方块注册表中的稳定状态对象，
 *       绝不能在此处释放。
 */
class PistonBlockEntity : public BlockEntity {
public:
    /**
     * @brief 默认构造函数
     * @param pos 位置
     */
    explicit PistonBlockEntity(const BlockPos& pos);

    /**
     * @brief 完整构造函数
     * @param pos 位置
     * @param pistonState 被移动的方块状态（非拥有）
     * @param facing 活塞朝向
     * @param extending 是否正在伸出
     * @param shouldRenderHead 是否渲染活塞头
     */
    PistonBlockEntity(
        const BlockPos& pos, const BlockState* pistonState, Direction facing, bool extending, bool shouldRenderHead);

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    /**
     * @brief 活塞需要每tick更新。
     */
    [[nodiscard]] bool needsTick() const override { return true; }

    /**
     * @brief 每tick更新移动进度并处理实体推动。
     * @param world 世界引用
     */
    void tick(IWorld& world) override;

    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    [[nodiscard]] bool isExtending() const { return m_extending; }
    [[nodiscard]] Direction getFacing() const { return m_facing; }
    [[nodiscard]] bool shouldRenderPistonHead() const { return m_shouldRenderHead; }

    /**
     * @brief 获取移动进度（0.0 - 1.0）。
     * @param partialTick 插值系数，超过 1.0 会被钳制
     * @return 插值后的进度
     */
    [[nodiscard]] float getProgress(float partialTick) const;

    [[nodiscard]] float getLastProgress() const { return m_lastProgress; }
    [[nodiscard]] const BlockState* getPistonState() const { return m_pistonState; }
    [[nodiscard]] Direction getMotionDirection() const;
    [[nodiscard]] float getOffsetX(float partialTick) const;
    [[nodiscard]] float getOffsetY(float partialTick) const;
    [[nodiscard]] float getOffsetZ(float partialTick) const;

    /**
     * @brief 清除活塞方块实体并落地最终状态。
     * @param world 世界引用
     */
    void clearPistonBlockEntity(IWorld& world);

    [[nodiscard]] bool isComplete() const { return m_progress >= 1.0f; }
    [[nodiscard]] i64 getLastTicked() const { return m_lastTicked; }

    /**
     * @brief 计算扩展进度
     * @param progress 原始进度
     * @return 伸出时为 progress - 1.0，收回时为 1.0 - progress
     */
    [[nodiscard]] float getExtendedProgress(float progress) const;

private:
    /**
     * @brief 推动本 tick 与活塞运动体积相交的实体。
     * @param world 世界引用
     * @param progressDelta 下一帧进度
     */
    void moveCollidedEntities(IWorld& world, float progressDelta);

    /**
     * @brief 收回时修复卡入活塞基座的实体。
     *
     * MC 1.16.5: 当活塞收回时，如果实体被卡在活塞基座位置，
     * 需要将实体推出到活塞基座之外。
     *
     * @param entity 实体指针
     * @param direction 活塞运动方向（收回方向）
     * @param moveDistance 移动距离
     */
    void fixEntityWithinPistonBase(Entity& entity, Direction direction, f32 moveDistance);

    /**
     * @brief 蜂蜜块拖拽实体逻辑。
     *
     * MC 1.16.5: 当活塞推动蜂蜜块时，站在蜂蜜块上的实体应该被拖拽。
     * 参考: PistonTileEntity.func_227024_g_
     *
     * @param world 世界引用
     * @param progressDelta 下一帧进度
     */
    void dragEntitiesOnHoneyBlock(IWorld& world, float progressDelta);

    /**
     * @brief 检查活塞移动的方块是否是蜂蜜块。
     * @return 如果是蜂蜜块返回 true
     */
    [[nodiscard]] bool isHoneyBlock() const;

    /**
     * @brief 按当前方块位置与进度偏移 AABB。
     * @param aabb 方块局部 AABB
     * @return 偏移后的世界坐标 AABB
     */
    [[nodiscard]] AxisAlignedBB moveByPositionAndProgress(const AxisAlignedBB& aabb) const;

    /// 被移动的方块状态（非拥有）
    const BlockState* m_pistonState = nullptr;

    Direction m_facing = Direction::North;
    bool m_extending = true;
    bool m_shouldRenderHead = false;

    float m_progress = 0.0f;
    float m_lastProgress = 0.0f;

    i64 m_lastTicked = 0;

    static constexpr float PROGRESS_PER_TICK = 0.5f;
    static constexpr float COMPLETE_THRESHOLD = 1.0f;
};

} // namespace blockentity
} // namespace mc
