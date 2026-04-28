#pragma once

#include "../Block.hpp"
#include "../../fluid/Fluid.hpp"
#include "../../fluid/FlowingFluid.hpp"
#include "../../IWorld.hpp"
#include "../BlockPos.hpp"
#include "../../../util/property/Properties.hpp"
#include <memory>

namespace mc {
namespace block {

/**
 * @brief 液体方块
 *
 * 与流体关联的方块，用于在世界中表示流体。
 *
 * 参考: net.minecraft.block.LiquidBlock
 *
 * 方块LEVEL (0-15) 与流体LEVEL (1-8) 的映射：
 * - 方块level=0 -> 流体level=8（源头）
 * - 方块level=1-7 -> 流体level=1-7
 * - 方块level=8-15 -> 流体level=8 + falling=true（下落）
 */
class LiquidBlock : public Block {
public:
    /**
     * @brief 构造液体方块
     *
     * @param fluid 关联的流动流体
     * @param properties 方块属性
     */
    LiquidBlock(fluid::FlowingFluid& fluid, BlockProperties properties);

    // ========== Block接口重写 ==========

    /**
     * @brief 获取流体状态
     *
     * 根据方块LEVEL属性计算对应的流体状态。
     *
     * @param state 方块状态
     * @return 流体状态指针
     */
    [[nodiscard]] const mc::fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 是否为空气
     *
     * 液体方块不是空气。
     */
    [[nodiscard]] bool isAir(const BlockState& state) const override {
        (void)state;
        return false;
    }

    /**
     * @brief 获取碰撞形状
     *
     * 液体方块没有碰撞形状。
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 检查指定面是否为实体面
     *
     * 液体方块没有实体面。
     */
    [[nodiscard]] bool isSolidSide(const BlockState& state, IWorld& world,
                                   const BlockPos& pos, Direction side) const override {
        (void)state;
        (void)world;
        (void)pos;
        (void)side;
        return false;
    }

    /**
     * @brief 检查是否传播天空光向下
     *
     * 液体总是传播天空光（衰减1级）。
     */
    [[nodiscard]] bool propagatesSkylightDown(const BlockState& state,
                                               IWorld* world = nullptr,
                                               const BlockPos* pos = nullptr) const override {
        (void)state;
        (void)world;
        (void)pos;
        return false;
    }

    /**
     * @brief 方块被放置时的处理
     *
     * 流体方块放置时需要调度流体tick。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居方块更新
     *
     * 当邻居方块改变时，重新调度流体tick。
     */
    void neighborChanged(IWorld& world, const BlockPos& pos,
                         Block& neighborBlock, const BlockPos& neighborPos,
                         bool isMoving) override;

    /**
     * @brief 执行方块tick
     *
     * 委托给流体的tick方法。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

    /**
     * @brief 是否需要随机tick
     */
    [[nodiscard]] bool ticksRandomly() const override;

    /**
     * @brief 随机tick
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state,
                    math::IRandom& random) override;

    /**
     * @brief 获取关联的流体
     */
    [[nodiscard]] fluid::FlowingFluid& getFluid() const { return m_fluid; }

    // ========== 方块与流体等级转换 ==========

    /**
     * @brief 方块等级转流体等级
     *
     * 方块level (0-15) -> 流体level (1-8)
     * - level=0 -> 流体level=8（源头）
     * - level=1-7 -> 流体level=1-7
     * - level=8-15 -> 流体level=8, falling=true
     *
     * @param blockLevel 方块等级 (0-15)
     * @return 流体等级 (1-8)
     */
    [[nodiscard]] static i32 blockLevelToFluidLevel(i32 blockLevel);

    /**
     * @brief 流体等级转方块等级
     *
     * @param fluidLevel 流体等级 (1-8)
     * @param falling 是否下落
     * @return 方块等级 (0-15)
     */
    [[nodiscard]] static i32 fluidLevelToBlockLevel(i32 fluidLevel, bool falling);

    /**
     * @brief 检查方块等级是否表示源头
     */
    [[nodiscard]] static bool isSourceLevel(i32 blockLevel) {
        return blockLevel == 0;
    }

    /**
     * @brief 检查方块等级是否表示下落
     */
    [[nodiscard]] static bool isFallingLevel(i32 blockLevel) {
        return blockLevel >= 8;
    }

    /**
     * @brief 获取放置时的方块状态
     *
     * 流体方块通常不能直接放置，但水源可以通过桶放置。
     * 默认返回默认状态（源头）。
     *
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override {
        (void)context;
        return defaultState();
    }

    /**
     * @brief 方块更新后处理
     *
     * 当邻居方块更新时检查是否需要触发流体混合反应。
     *
     * @param state 当前方块状态
     * @param facing 更新的方向
     * @param facingState 邻居状态
     * @param world 世界
     * @param currentPos 当前方块位置
     * @param facingPos 邻居位置
     * @return 更新后的状态
     */
    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 岩浆水反应 ==========

    /**
     * @brief 检查并触发岩浆水反应
     *
     * 当岩浆接触到水时：
     * - 源头岩浆 + 水 -> 黑曜石
     * - 流动岩浆 + 水 -> 圆石
     * - 岩浆 + 蓝冰 + 灵魂土 -> 玄武岩
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 如果反应发生返回 false（阻止后续 tick）
     */
    bool reactWithNeighbors(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 触发流体混合效果
     *
     * 播放烟雾粒子效果和嘶嘶声。
     *
     * @param world 世界
     * @param pos 位置
     */
    void triggerMixEffects(IWorld& world, const BlockPos& pos);

    // ========== 桶装流体 ==========

    /**
     * @brief 用桶舀起流体
     *
     * 如果是源头方块，移除流体并返回对应的流体。
     *
     * @param world 世界
     * @param pos 位置
     * @param state 方块状态
     * @return 如果成功舀起返回流体指针，否则返回 nullptr
     */
    [[nodiscard]] fluid::Fluid* pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state);

private:
    fluid::FlowingFluid& m_fluid;

    // 缓存的流体状态（方块level -> 流体状态）
    // 存储FluidState对象而非指针，避免悬垂引用
    mutable std::vector<fluid::FluidState> m_fluidStateCache;

    /**
     * @brief 构建流体状态缓存
     */
    void buildFluidStateCache();
};

} // namespace block
} // namespace mc
