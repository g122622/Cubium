#pragma once

#include "../BlockEntity.hpp"
#include "../../block/Block.hpp"
#include "../../../util/Direction.hpp"
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
 * 参考: net.minecraft.tileentity.PistonTileEntity
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
     * @param pistonState 被移动的方块状态
     * @param facing 活塞朝向
     * @param extending 是否正在伸出
     * @param shouldRenderHead 是否渲染活塞头
     */
    PistonBlockEntity(
        const BlockPos& pos,
        std::unique_ptr<BlockState> pistonState,
        Direction facing,
        bool extending,
        bool shouldRenderHead);

    // ========== BlockEntity 接口实现 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    /**
     * @brief 活塞需要每tick更新
     */
    [[nodiscard]] bool needsTick() const override { return true; }

    /**
     * @brief 每tick更新移动进度
     * @param world 世界引用
     */
    void tick(IWorld& world) override;

    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 活塞特有方法 ==========

    /**
     * @brief 检查活塞是否正在伸出
     * @return true 如果正在伸出
     */
    [[nodiscard]] bool isExtending() const { return m_extending; }

    /**
     * @brief 获取活塞朝向
     * @return 朝向方向
     */
    [[nodiscard]] Direction getFacing() const { return m_facing; }

    /**
     * @brief 是否应该渲染活塞头
     * @return true 如果应该渲染活塞头
     */
    [[nodiscard]] bool shouldRenderPistonHead() const { return m_shouldRenderHead; }

    /**
     * @brief 获取移动进度（0.0 - 1.0）
     * @param partialTick 部分tick时间（用于插值）
     * @return 插值后的进度
     */
    [[nodiscard]] float getProgress(float partialTick) const;

    /**
     * @brief 获取上一tick的进度
     * @return 上一tick进度
     */
    [[nodiscard]] float getLastProgress() const { return m_lastProgress; }

    /**
     * @brief 获取被移动的方块状态
     * @return 方块状态指针，可能为空
     */
    [[nodiscard]] const BlockState* getPistonState() const { return m_pistonState.get(); }

    /**
     * @brief 获取移动方向
     *
     * 伸出时移动方向与朝向相同，收回时相反。
     *
     * @return 移动方向
     */
    [[nodiscard]] Direction getMotionDirection() const;

    /**
     * @brief 获取X偏移量（用于渲染）
     * @param partialTick 部分tick时间
     * @return X偏移量
     */
    [[nodiscard]] float getOffsetX(float partialTick) const;

    /**
     * @brief 获取Y偏移量（用于渲染）
     * @param partialTick 部分tick时间
     * @return Y偏移量
     */
    [[nodiscard]] float getOffsetY(float partialTick) const;

    /**
     * @brief 获取Z偏移量（用于渲染）
     * @param partialTick 部分tick时间
     * @return Z偏移量
     */
    [[nodiscard]] float getOffsetZ(float partialTick) const;

    /**
     * @brief 清除活塞方块实体
     *
     * 当活塞移动完成后调用，移除方块实体并放置最终方块。
     *
     * @param world 世界引用
     */
    void clearPistonBlockEntity(IWorld& world);

    /**
     * @brief 检查活塞移动是否完成
     * @return true 如果进度>=1.0
     */
    [[nodiscard]] bool isComplete() const { return m_progress >= 1.0f; }

    /**
     * @brief 获取最后一次tick的游戏时间
     * @return 游戏时间
     */
    [[nodiscard]] i64 getLastTicked() const { return m_lastTicked; }

private:
    /**
     * @brief 获取扩展进度
     *
     * 伸出时返回 progress - 1.0
     * 收回时返回 1.0 - progress
     *
     * @param progress 原始进度
     * @return 扩展进度
     */
    [[nodiscard]] float getExtendedProgress(float progress) const;

    /**
     * @brief 移动碰撞的实体
     * @param world 世界引用
     * @param progressDelta 进度增量
     */
    void moveCollidedEntities(IWorld& world, float progressDelta);

    /**
     * @brief 根据位置和进度调整AABB
     * @param aabb 原始AABB
     * @return 调整后的AABB
     */
    [[nodiscard]] AxisAlignedBB moveByPositionAndProgress(const AxisAlignedBB& aabb) const;

    /// 被移动的方块状态
    std::unique_ptr<BlockState> m_pistonState;

    /// 活塞朝向
    Direction m_facing = Direction::North;

    /// 是否正在伸出
    bool m_extending = true;

    /// 是否应该渲染活塞头（活塞本体位置）
    bool m_shouldRenderHead = false;

    /// 当前移动进度（0.0 - 1.0）
    float m_progress = 0.0f;

    /// 上一tick的进度
    float m_lastProgress = 0.0f;

    /// 最后一次tick的游戏时间
    i64 m_lastTicked = 0;

    /// 每tick移动速度
    static constexpr float PROGRESS_PER_TICK = 0.5f;

    /// 完成进度阈值
    static constexpr float COMPLETE_THRESHOLD = 1.0f;
};

} // namespace blockentity
} // namespace mc
