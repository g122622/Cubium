#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/Direction.hpp"
#include "../BlockPos.hpp"

namespace mc {

// 前向声明
class IWorld;
class ItemStack;
class BlockState;

namespace blocks {

/**
 * @brief 发射位置接口
 *
 * 提供发射物品所需的位置和世界信息。
 * 参考 MC 1.16.5 IBlockSource
 */
class IBlockSource {
public:
    virtual ~IBlockSource() = default;

    /**
     * @brief 获取X坐标
     * @return X坐标（方块中心为 +0.5）
     */
    [[nodiscard]] virtual double getX() const = 0;

    /**
     * @brief 获取Y坐标
     * @return Y坐标（方块中心为 +0.5）
     */
    [[nodiscard]] virtual double getY() const = 0;

    /**
     * @brief 获取Z坐标
     * @return Z坐标（方块中心为 +0.5）
     */
    [[nodiscard]] virtual double getZ() const = 0;

    /**
     * @brief 获取方块位置
     * @return 方块坐标
     */
    [[nodiscard]] virtual BlockPos getBlockPos() const = 0;

    /**
     * @brief 获取方块状态
     * @return 方块状态
     */
    [[nodiscard]] virtual const BlockState& getBlockState() const = 0;

    /**
     * @brief 获取世界引用
     * @return 世界引用
     */
    [[nodiscard]] virtual IWorld& getWorld() = 0;
};

/**
 * @brief 发射位置实现
 *
 * 简单的位置实现类。
 * 参考 MC 1.16.5 Position
 */
class DispensePosition : public IBlockSource {
public:
    /**
     * @brief 构造函数
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param offsetX X偏移（默认0.5，方块中心）
     * @param offsetY Y偏移（默认0.5，方块中心）
     * @param offsetZ Z偏移（默认0.5，方块中心）
     */
    DispensePosition(IWorld& world, const BlockPos& pos, const BlockState& state,
                     double offsetX = 0.5, double offsetY = 0.5, double offsetZ = 0.5);

    [[nodiscard]] double getX() const override { return m_pos.x + m_offsetX; }
    [[nodiscard]] double getY() const override { return m_pos.y + m_offsetY; }
    [[nodiscard]] double getZ() const override { return m_pos.z + m_offsetZ; }
    [[nodiscard]] BlockPos getBlockPos() const override { return m_pos; }
    [[nodiscard]] const BlockState& getBlockState() const override { return m_state; }
    [[nodiscard]] IWorld& getWorld() override { return m_world; }

private:
    IWorld& m_world;
    BlockPos m_pos;
    const BlockState& m_state;
    double m_offsetX;
    double m_offsetY;
    double m_offsetZ;
};

} // namespace blocks
} // namespace mc
