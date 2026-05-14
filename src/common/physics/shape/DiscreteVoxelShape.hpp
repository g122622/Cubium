#pragma once

#include "../../core/Types.hpp"
#include "../../util/Direction.hpp"
#include "BooleanOp.hpp"
#include <bitset>
#include <functional>
#include <vector>

namespace mc {

// 前向声明
class VoxelShape;

/**
 * @brief 离散体素形状
 *
 * 使用位图存储体素占用状态的离散形状表示。
 * 每个体素用1位表示是否被占用。
 *
 * 参考MC DiscreteVoxelShape。
 *
 * 坐标系：
 * - x, y, z 方向的尺寸可以不同
 * - 索引计算：(x * ySize + y) * zSize + z
 */
class DiscreteVoxelShape {
public:
    /**
     * @brief 面消费者接口
     */
    using IntFaceConsumer = std::function<void(Direction, i32, i32, i32)>;

    /**
     * @brief 线消费者接口
     */
    using IntLineConsumer = std::function<void(i32, i32, i32, i32, i32, i32)>;

    DiscreteVoxelShape();
    DiscreteVoxelShape(i32 xSize, i32 ySize, i32 zSize);
    virtual ~DiscreteVoxelShape() = default;

    // 允许拷贝
    DiscreteVoxelShape(const DiscreteVoxelShape& other);
    DiscreteVoxelShape& operator=(const DiscreteVoxelShape& other);

    // 允许移动
    DiscreteVoxelShape(DiscreteVoxelShape&& other) noexcept;
    DiscreteVoxelShape& operator=(DiscreteVoxelShape&& other) noexcept;

    // === 尺寸查询 ===
    [[nodiscard]] i32 getXSize() const { return m_xSize; }
    [[nodiscard]] i32 getYSize() const { return m_ySize; }
    [[nodiscard]] i32 getZSize() const { return m_zSize; }
    [[nodiscard]] i32 getSize(Axis axis) const;

    // === 体素操作 ===

    /**
     * @brief 检查指定位置是否被占用（不检查边界）
     */
    [[nodiscard]] virtual bool isFull(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查指定位置是否被占用（带边界检查）
     * 超出边界返回false
     */
    [[nodiscard]] bool isFullWide(i32 x, i32 y, i32 z) const;

    /**
     * @brief 带轴循环的检查
     */
    [[nodiscard]] bool isFullWide(AxisCycle cycle, i32 x, i32 y, i32 z) const;
    [[nodiscard]] bool isFull(AxisCycle cycle, i32 x, i32 y, i32 z) const;

    /**
     * @brief 填充指定位置的体素
     */
    virtual void fill(i32 x, i32 y, i32 z);

    /**
     * @brief 清除指定位置的体素
     */
    virtual void clear(i32 x, i32 y, i32 z);

    /**
     * @brief 检查形状是否为空
     */
    [[nodiscard]] virtual bool isEmpty() const;

    // === 边界查询 ===

    /**
     * @brief 获取指定轴第一个被占用的体素索引
     */
    [[nodiscard]] virtual i32 firstFull(Axis axis) const;

    /**
     * @brief 获取指定轴最后一个被占用的体素索引+1
     */
    [[nodiscard]] virtual i32 lastFull(Axis axis) const;

    /**
     * @brief 获取指定轴在给定切片上第一个被占用的体素索引
     */
    [[nodiscard]] i32 firstFull(Axis axis, i32 slice1, i32 slice2) const;

    /**
     * @brief 获取指定轴在给定切片上最后一个被占用的体素索引+1
     */
    [[nodiscard]] i32 lastFull(Axis axis, i32 slice1, i32 slice2) const;

    // === Z轴线操作（用于盒子合并） ===

    /**
     * @brief 检查指定位置的Z轴线段是否完全填充
     * @param fromZ 起始Z坐标（包含）
     * @param toZ 结束Z坐标（不包含）
     * @param x X坐标
     * @param y Y坐标
     * @return 如果线段内所有体素都填充返回true
     */
    [[nodiscard]] bool isZAxisLineFull(i32 fromZ, i32 toZ, i32 x, i32 y) const;

    /**
     * @brief 设置指定位置的Z轴线段的填充状态
     * @param fromZ 起始Z坐标（包含）
     * @param toZ 结束Z坐标（不包含）
     * @param x X坐标
     * @param y Y坐标
     * @param filled 是否填充
     */
    void setZAxisLine(i32 fromZ, i32 toZ, i32 x, i32 y, bool filled);

    /**
     * @brief 检查XZ平面矩形区域是否完全填充
     * @param fromX 起始X坐标（包含）
     * @param toX 结束X坐标（不包含）
     * @param fromZ 起始Z坐标（包含）
     * @param toZ 结束Z坐标（不包含）
     * @param y Y坐标
     * @return 如果矩形区域内所有体素都填充返回true
     */
    [[nodiscard]] bool isXZRectangleFull(i32 fromX, i32 toX, i32 fromZ, i32 toZ, i32 y) const;

    // === 遍历 ===

    /**
     * @brief 遍历所有边
     * @param consumer 消费者函数 (x1, y1, z1, x2, y2, z2)
     * @param simplify 是否简化相邻边
     */
    void forAllEdges(const IntLineConsumer& consumer, bool simplify = true);

    /**
     * @brief 遍历所有盒子
     * @param consumer 消费者函数 (x1, y1, z1, x2, y2, z2)
     * @param simplify 是否合并相邻体素
     */
    void forAllBoxes(const IntLineConsumer& consumer, bool simplify = true);

    /**
     * @brief 遍历所有面
     * @param consumer 消费者函数 (direction, x, y, z)
     */
    void forAllFaces(const IntFaceConsumer& consumer);

    // === 填充 ===

    /**
     * @brief 填充整个形状
     */
    void fillAll();

    /**
     * @brief 填充指定范围
     */
    void fillRange(i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ);

    // === 工厂方法 ===

    /**
     * @brief 创建指定范围内的填充形状
     */
    [[nodiscard]] static DiscreteVoxelShape withFilledBounds(
        i32 xSize, i32 ySize, i32 zSize, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ);

protected:
    i32 m_xSize = 0;
    i32 m_ySize = 0;
    i32 m_zSize = 0;
    std::vector<bool> m_storage; // 体素占用状态

    // 边界缓存
    mutable i32 m_xMin = 0;
    mutable i32 m_yMin = 0;
    mutable i32 m_zMin = 0;
    mutable i32 m_xMax = 0;
    mutable i32 m_yMax = 0;
    mutable i32 m_zMax = 0;
    mutable bool m_boundsDirty = true;

    /**
     * @brief 计算线性索引
     */
    [[nodiscard]] i32 getIndex(i32 x, i32 y, i32 z) const;

    /**
     * @brief 更新边界缓存
     */
    void updateBounds(i32 x, i32 y, i32 z);

    /**
     * @brief 重新计算边界缓存
     */
    void recalculateBounds() const;

    /**
     * @brief 带边界更新的填充
     */
    void fillUpdateBounds(i32 x, i32 y, i32 z, bool updateBounds);

private:
    void forAllAxisEdges(const IntLineConsumer& consumer, AxisCycle cycle, bool simplify);
    void forAllAxisFaces(const IntFaceConsumer& consumer, AxisCycle cycle);
};

} // namespace mc
