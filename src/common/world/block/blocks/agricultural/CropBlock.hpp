#pragma once

#include "BushBlock.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class ServerWorld;

namespace blocks {

/**
 * @brief 农作物方块基类
 *
 * 可生长的农作物，如小麦、胡萝卜、马铃薯等。
 * 使用 AGE_0_7 属性表示生长阶段（0-7，共8个阶段）。
 *
 * 参考: net.minecraft.block.CropsBlock
 */
class CropBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CropBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~CropBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取年龄属性
     */
    [[nodiscard]] virtual const IntegerProperty& getAgeProperty() const;

    /**
     * @brief 获取最大年龄
     */
    [[nodiscard]] virtual int getMaxAge() const { return 7; }

    /**
     * @brief 获取当前年龄
     */
    [[nodiscard]] int getAge(const BlockState& state) const;

    /**
     * @brief 创建指定年龄的状态
     */
    [[nodiscard]] BlockState withAge(int age) const;

    /**
     * @brief 是否为最大年龄
     */
    [[nodiscard]] bool isMaxAge(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 生长逻辑 ==========

    /**
     * @brief 随机刻（用于生长）
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否需要随机 tick
     */
    [[nodiscard]] bool ticksRandomly() const override;

    /**
     * @brief 生长（使用骨粉）
     */
    void grow(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 获取骨粉增加的年龄
     */
    [[nodiscard]] virtual int getBonemealAgeIncrease() const;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 掉落物 ==========

    /**
     * @brief 获取作物物品（成熟时掉落）
     */
    [[nodiscard]] virtual u32 getCropItem() const = 0;

    /**
     * @brief 获取种子物品
     */
    [[nodiscard]] virtual u32 getSeedItem() const = 0;

protected:
    /**
     * @brief 检查下方是否可支撑
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState,
        IWorld& world,
        const BlockPos& groundPos) const override;

    /**
     * @brief 计算生长速度
     */
    [[nodiscard]] static float getGrowthChance(
        const Block& block,
        IBlockReader& world,
        const BlockPos& pos);

    /// 各年龄阶段的形状缓存
    std::array<CollisionShape, 8> m_shapesByAge;
};

} // namespace blocks
} // namespace mc
