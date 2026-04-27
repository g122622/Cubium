#pragma once

#include "client/renderer/trident/blockentity/IBlockEntityRenderer.hpp"
#include "client/renderer/trident/blockentity/BlockEntityRenderer.hpp"

namespace mc {

class MatrixStack;

namespace blockentity {
class PistonBlockEntity;
}

namespace client::renderer::trident::blockentity {

/**
 * @brief 活塞方块实体渲染器
 *
 * 渲染活塞移动过程中的动画效果。
 * 参考 MC 1.16.5 PistonTileEntityRenderer
 *
 * 活塞动画特点：
 * - 动画持续2个游戏tick（约0.1秒）
 * - 每tick进度增加0.5
 * - 使用partialTick进行帧间插值
 * - 需要渲染被移动的方块和活塞臂
 */
class PistonRenderer : public BlockEntityRenderer<mc::blockentity::PistonBlockEntity> {
public:
    PistonRenderer();
    ~PistonRenderer() override = default;

    // 禁止拷贝
    PistonRenderer(const PistonRenderer&) = delete;
    PistonRenderer& operator=(const PistonRenderer&) = delete;

    // 允许移动
    PistonRenderer(PistonRenderer&&) noexcept = default;
    PistonRenderer& operator=(PistonRenderer&&) noexcept = default;

    /**
     * @brief 渲染活塞方块实体
     *
     * @param entity 活塞方块实体
     * @param partialTick 部分tick（0.0-1.0）
     * @param light 组合光照值
     */
    void render(const mc::blockentity::PistonBlockEntity& entity, f32 partialTick, u32 light) override;

    /**
     * @brief 活塞是全局渲染器
     *
     * 活塞可能推动方块到相邻区块，需要跨区块可见。
     */
    [[nodiscard]] bool isGlobalRenderer() const override { return true; }

    /**
     * @brief 活塞最大渲染距离
     *
     * 活塞通常在视距内，返回默认值。
     */
    [[nodiscard]] f64 getMaxRenderDistanceSquared() const override { return 64.0; }

private:
    BlockEntityRendererHelper m_helper;  ///< 渲染辅助工具

    /**
     * @brief 渲染活塞臂
     *
     * @param entity 活塞方块实体
     * @param progress 插值后的进度
     * @param light 组合光照
     */
    void renderPistonHead(
        const mc::blockentity::PistonBlockEntity& entity,
        f32 progress,
        u32 light);

    /**
     * @brief 渲染被移动的方块
     *
     * @param entity 活塞方块实体
     * @param progress 插值后的进度
     * @param light 组合光照
     */
    void renderMovingBlock(
        const mc::blockentity::PistonBlockEntity& entity,
        f32 progress,
        u32 light);
};

} // namespace mc::client::renderer::trident::blockentity
} // namespace mc
