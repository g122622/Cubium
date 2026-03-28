#pragma once

#include "../agricultural/BushBlock.hpp"
#include "DoublePlantBlock.hpp"
#include "../../../../util/property/Properties.hpp"
#include <array>

namespace mc {
namespace blocks {

/**
 * @brief 花朵方块基类
 *
 * 小型花朵（蒲公英、玫瑰等）。
 * 可放置在草方块、泥土、耕地等上。
 * 可以放在花盆中。
 *
 * 参考: net.minecraft.block.FlowerBlock
 */
class FlowerBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param suspiciousStewEffect 可疑炖汤效果（可选）
     * @param effectDuration 效果持续时间（秒）
     */
    FlowerBlock(
        const BlockProperties& properties,
        u32 suspiciousStewEffect = 0,
        i32 effectDuration = 0);

    /**
     * @brief 析构函数
     */
    ~FlowerBlock() override = default;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 效果 ==========

    /**
     * @brief 获取可疑炖汤效果ID
     */
    [[nodiscard]] u32 getSuspiciousStewEffect() const { return m_suspiciousStewEffect; }

    /**
     * @brief 获取效果持续时间（秒）
     */
    [[nodiscard]] i32 getEffectDuration() const { return m_effectDuration; }

protected:
    /**
     * @brief 检查下方是否可支撑
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState,
        IWorld& world,
        const BlockPos& groundPos) const override;

    /// 可疑炖汤效果ID
    u32 m_suspiciousStewEffect;
    /// 效果持续时间（秒）
    i32 m_effectDuration;
};

/**
 * @brief 丁香方块
 *
 * 两格高的花朵，可制作染料。
 *
 * 参考: net.minecraft.block.LilacBlock
 */
class LilacBlock : public DoublePlantBlock {
public:
    explicit LilacBlock(const BlockProperties& properties);
};

/**
 * @brief 玫瑰丛方块
 *
 * 两格高的花朵，可制作红色染料。
 *
 * 参考: net.minecraft.block.RoseBushBlock
 */
class RoseBushBlock : public DoublePlantBlock {
public:
    explicit RoseBushBlock(const BlockProperties& properties);
};

/**
 * @brief 牡丹方块
 *
 * 两格高的花朵，可制作粉色染料。
 *
 * 参考: net.minecraft.block.PeonyBlock
 */
class PeonyBlock : public DoublePlantBlock {
public:
    explicit PeonyBlock(const BlockProperties& properties);
};

/**
 * @brief 向日葵方块
 *
 * 两格高的花朵，朝向太阳。
 *
 * 参考: net.minecraft.block.SunflowerBlock
 */
class SunflowerBlock : public DoublePlantBlock {
public:
    explicit SunflowerBlock(const BlockProperties& properties);
};

} // namespace blocks
} // namespace mc
