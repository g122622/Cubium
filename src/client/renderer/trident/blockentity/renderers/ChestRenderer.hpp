#pragma once

#include "client/renderer/trident/blockentity/IBlockEntityRenderer.hpp"
#include "client/renderer/trident/blockentity/BlockEntityRenderer.hpp"

namespace mc {

namespace blockentity {
class ChestEntity;
}

namespace client::renderer::trident::blockentity {

/**
 * @brief 箱子方块实体渲染器
 *
 * 渲染箱子和盖子的开合动画。
 * 参考 MC 1.16.5 ChestTileEntityRenderer
 *
 * 箱子动画特点：
 * - 盖子角度从0.0到1.0
 * - 使用非线性缓动：angle = 1.0 - (1.0 - angle)^3
 * - 需要考虑双箱情况（两个箱子合并）
 */
class ChestRenderer : public BlockEntityRenderer<mc::blockentity::ChestEntity> {
public:
    ChestRenderer();
    ~ChestRenderer() override = default;

    // 禁止拷贝
    ChestRenderer(const ChestRenderer&) = delete;
    ChestRenderer& operator=(const ChestRenderer&) = delete;

    // 允许移动
    ChestRenderer(ChestRenderer&&) noexcept = default;
    ChestRenderer& operator=(ChestRenderer&&) noexcept = default;

    /**
     * @brief 渲染箱子方块实体
     *
     * @param entity 箱子方块实体
     * @param partialTick 部分tick（0.0-1.0）
     * @param light 组合光照值
     */
    void render(const mc::blockentity::ChestEntity& entity, f32 partialTick, u32 light) override;

    /**
     * @brief 箱子不是全局渲染器
     */
    [[nodiscard]] bool isGlobalRenderer() const override { return false; }

    /**
     * @brief 箱子最大渲染距离
     */
    [[nodiscard]] f64 getMaxRenderDistanceSquared() const override { return 64.0; }

private:
    BlockEntityRendererHelper m_helper;  ///< 渲染辅助工具

    /**
     * @brief 计算插值后的盖子角度
     *
     * @param entity 箱子实体
     * @param partialTick 部分tick
     * @return 插值后的角度（0.0-1.0）
     */
    [[nodiscard]] f32 getInterpolatedLidAngle(
        const mc::blockentity::ChestEntity& entity,
        f32 partialTick) const;

    /**
     * @brief 应用非线性缓动
     *
     * MC风格的三次缓动，使盖子动画更自然。
     *
     * @param angle 原始角度（0.0-1.0）
     * @return 缓动后的角度
     */
    [[nodiscard]] f32 applyLidEasing(f32 angle) const;

    /**
     * @brief 渲染箱体
     *
     * @param pos 方块位置
     * @param light 组合光照
     */
    void renderChestBody(const BlockPos& pos, u32 light);

    /**
     * @brief 渲染盖子
     *
     * @param pos 方块位置
     * @param lidAngle 盖子角度（已缓动）
     * @param light 组合光照
     */
    void renderChestLid(const BlockPos& pos, f32 lidAngle, u32 light);

    /**
     * @brief 渲染锁扣
     *
     * @param pos 方块位置
     * @param lidAngle 盖子角度（已缓动）
     * @param light 组合光照
     */
    void renderChestLatch(const BlockPos& pos, f32 lidAngle, u32 light);
};

} // namespace mc::client::renderer::trident::blockentity
} // namespace mc
