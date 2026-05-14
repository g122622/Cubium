#pragma once

#include "util/AxisAlignedBB.hpp"
#include "util/Direction.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"

namespace mc {
namespace blockentity {

/**
 * @brief 漏斗接口
 *
 * 定义漏斗的通用接口，用于统一处理漏斗方块和漏斗矿车。
 * 提供位置获取方法，用于物品传输时查找相邻容器。
 *
 * 参考: net.minecraft.tileentity.IHopper
 */
class IHopper {
public:
    virtual ~IHopper() = default;

    /**
     * @brief 获取漏斗所在的世界
     * @return 世界指针（可能为nullptr）
     */
    [[nodiscard]] virtual IWorld* getWorld() = 0;
    [[nodiscard]] virtual const IWorld* getWorld() const = 0;

    /**
     * @brief 获取漏斗的X坐标（世界坐标）
     * @return X坐标
     */
    [[nodiscard]] virtual double getXPos() const = 0;

    /**
     * @brief 获取漏斗的Y坐标（世界坐标）
     * @return Y坐标
     */
    [[nodiscard]] virtual double getYPos() const = 0;

    /**
     * @brief 获取漏斗的Z坐标（世界坐标）
     * @return Z坐标
     */
    [[nodiscard]] virtual double getZPos() const = 0;

    /**
     * @brief 获取漏斗位置（方块坐标）
     * @return 方块位置
     */
    [[nodiscard]] virtual BlockPos getHopperPos() const = 0;

    /**
     * @brief 获取漏斗输出方向
     * @return 输出方向（默认向下）
     */
    [[nodiscard]] virtual Direction getOutputDirection() const { return Direction::Down; }

    // ========== 静态工具方法 ==========

    /**
     * @brief 获取漏斗上方的收集区域
     * @param hopper 漏斗
     * @return 收集区域的AABB
     *
     * 收集区域包括:
     * - 漏斗内部碗状区域 (2, 11, 2) -> (14, 16, 14)
     * - 上方一格方块区域 (0, 16, 0) -> (16, 32, 16)
     */
    [[nodiscard]] static AxisAlignedBB getCollectionArea(const IHopper& hopper);

    /**
     * @brief 获取漏斗的输出位置
     * @param hopper 漏斗
     * @return 输出位置
     */
    [[nodiscard]] static BlockPos getOutputPosition(const IHopper& hopper)
    {
        return hopper.getHopperPos().offset(hopper.getOutputDirection());
    }
};

} // namespace blockentity
} // namespace mc
