#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 营火方块
 *
 * 营火是一种多功能方块：
 * - 光源：发出15级光照（点燃时）
 * - 烹饪：可以烹饪食物（最多4个）
 * - 烟雾：产生向上飘的烟雾粒子
 * - 伤害：站在上面会造成伤害
 *
 * 状态属性：
 * - LIT: 是否点燃
 * - SIGNAL_FIRE: 是否为信号火（添加烟雾高度）
 * - WATERLOGGED: 是否被水淹没
 * - AGE: 熄灭进度（雨天时增加）
 *
 * 参考: net.minecraft.block.CampfireBlock
 */
class CampfireBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param lightValue 光照等级（普通营火=15，灵魂营火=10）
     */
    explicit CampfireBlock(BlockProperties properties, u8 lightValue = 15);
    ~CampfireBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    // ========== 光照 ==========

    /**
     * @brief 获取动态光照等级
     *
     * 点燃时发出光照，熄灭时不发光。
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级 (0 或配置值)
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr) const override;

    // ========== 工具方法 ==========

    /**
     * @brief 是否点燃
     */
    [[nodiscard]] static bool isLit(const BlockState& state) {
        return state.get(BlockStateProperties::LIT());
    }

    /**
     * @brief 是否被水淹没
     */
    [[nodiscard]] static bool isWaterlogged(const BlockState& state) {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /**
     * @brief 是否为信号火
     */
    [[nodiscard]] static bool isSignalFire(const BlockState& state) {
        return state.get(BlockStateProperties::SIGNAL_FIRE());
    }

    /**
     * @brief 点燃营火
     */
    static void light(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 熄灭营火
     */
    static void extinguish(IWorld& world, const BlockPos& pos, BlockState& state);

protected:
    /// 营火形状
    CollisionShape m_shape;
    /// 光照等级（普通=15，灵魂=10）
    u8 m_lightValue;
};

/**
 * @brief 灵魂营火方块
 *
 * 灵魂营火是营火的变种：
 * - 光源：发出10级光照（比普通营火暗）
 * - 催化：可以重生物魂土上的生物
 * - 蓝色火焰：视觉效果不同于普通营火
 *
 * 参考: net.minecraft.block.SoulCampfireBlock
 */
class SoulCampfireBlock : public CampfireBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit SoulCampfireBlock(BlockProperties properties);
    ~SoulCampfireBlock() override = default;
};

} // namespace blocks
} // namespace mc
