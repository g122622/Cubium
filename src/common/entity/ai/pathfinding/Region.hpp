#pragma once

#include "../../../core/Types.hpp"

// 前向声明
namespace mc {
class BlockState;
}

namespace mc::entity::ai::pathfinding {

/**
 * @brief 世界区域访问接口
 *
 * 提供寻路算法对世界区块的有限访问。
 * 这是一个抽象接口，允许不同实现（服务端世界、客户端世界等）。
 *
 * 参考 MC 1.16.5 IBlockReader
 */
class Region {
public:
    virtual ~Region() = default;

    /**
     * @brief 获取指定位置的方块状态ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 方块状态ID，如果位置无效返回0（空气）
     */
    [[nodiscard]] virtual u32 getBlockStateId(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取指定位置的方块状态
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 方块状态指针，如果位置无效或为空气返回nullptr
     */
    [[nodiscard]] virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否在加载范围内
     */
    [[nodiscard]] virtual bool isLoaded(i32 x, i32 z) const = 0;

    /**
     * @brief 获取最高方块Y坐标
     * @param x X坐标
     * @param z Z坐标
     * @return 最高方块Y坐标
     */
    [[nodiscard]] virtual i32 getHeight(i32 x, i32 z) const = 0;

    /**
     * @brief 检查位置是否可通行
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 是否可通行
     */
    [[nodiscard]] virtual bool isWalkable(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 检查位置是否是水
     */
    [[nodiscard]] virtual bool isWater(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 检查位置是否是岩浆
     */
    [[nodiscard]] virtual bool isLava(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取方块顶部高度（考虑台阶、楼梯等）
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 顶部高度偏移（通常为0或0.5）
     */
    [[nodiscard]] virtual f32 getBlockTopY(i32 /*x*/, i32 y, i32 /*z*/) const
    {
        // 默认实现
        return static_cast<f32>(y) + 1.0f;
    }
};

} // namespace mc::entity::ai::pathfinding
