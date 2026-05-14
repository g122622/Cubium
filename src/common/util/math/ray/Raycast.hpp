#pragma once

#include "../../../core/BlockRaycastResult.hpp"
#include "../../../world/IWorld.hpp"
#include "Ray.hpp"

namespace mc {

/**
 * @brief 射线检测上下文
 *
 * 包含射线检测所需的所有参数。
 * 参考MC的RayTraceContext。
 */
struct RaycastContext {
    Ray ray;
    f32 maxDistance = 5.0f; ///< MC默认5格，创造模式更远

    RaycastContext() = default;

    /**
     * @brief 构造射线检测上下文
     * @param r 射线
     * @param dist 最大检测距离
     */
    explicit RaycastContext(const Ray& r, f32 dist = 5.0f)
        : ray(r)
        , maxDistance(dist)
    {}

    /**
     * @brief 获取射线终点
     */
    [[nodiscard]] Vector3 endPosition() const { return ray.at(maxDistance); }
};

/**
 * @brief 执行方块射线检测
 *
 * 使用DDA算法沿射线遍历方块，并基于方块 shape 计算命中结果。
 *
 * 算法说明（参考MC的IBlockReader.doRayTrace）：
 * 1. 计算射线在各轴上穿过一个方块所需的时间比率
 * 2. 每次选择最近的方块边界进入
 * 3. 对候选方块的 shape 做精确包围盒相交测试
 * 4. 超出距离限制或Y范围时停止
 * 5. 区块未加载（nullptr）视为空气，继续检测
 *
 * @param context 射线参数
 * @param world 方块读取器
 * @return 检测结果，若未击中返回miss
 */
[[nodiscard]] BlockRaycastResult raycastBlocks(const RaycastContext& context, const IWorld& world);

} // namespace mc
