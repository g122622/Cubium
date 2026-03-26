#pragma once

#include "ItemUseContext.hpp"
#include "../world/block/Block.hpp"

namespace mc {

/**
 * @brief 方块放置上下文
 *
 * 继承自 ItemUseContext，提供方块放置专用的上下文信息。
 * 包含放置位置计算、可替换方块检测等功能。
 *
 * 参考: net.minecraft.item.BlockItemUseContext
 */
class BlockItemUseContext : public ItemUseContext {
public:
    /**
     * @brief 构造方块放置上下文
     * @param world 世界引用（IWorld接口）
     * @param player 玩家指针（可为nullptr）
     * @param stack 物品堆
     * @param hitPos 击中点（世界坐标）
     * @param blockPos 击中的方块位置
     * @param face 击中的面
     * @param playerYaw 玩家yaw角度（用于计算水平朝向）
     */
    BlockItemUseContext(const IWorld& world,
                        Player* player,
                        const ItemStack& stack,
                        const Vector3& hitPos,
                        const BlockPos& blockPos,
                        Direction face,
                        f32 playerYaw = 0.0f);

    ~BlockItemUseContext() override = default;

    // ========== 放置位置 ==========

    /**
     * @brief 获取实际放置位置
     *
     * 如果点击的方块可替换，返回点击的方块位置；
     * 否则返回相邻位置（击中面的另一侧）。
     *
     * @return 放置位置
     */
    [[nodiscard]] BlockPos placementPos() const { return m_placementPos; }

    /**
     * @brief 获取相邻位置（击中面的另一侧）
     * @return 相邻方块位置
     */
    [[nodiscard]] BlockPos adjacentPos() const { return m_adjacentPos; }

    // ========== 状态检查 ==========

    /**
     * @brief 检查是否可以放置方块
     *
     * 检查放置位置是否可以放置方块：
     * 1. 放置位置在世界边界内
     * 2. 放置位置为空气或可替换方块
     *
     * @return 是否可以放置
     */
    [[nodiscard]] bool canPlace() const;

    /**
     * @brief 是否替换点击的方块
     *
     * 如果点击的方块可替换（如水、草等），则为 true，
     * 此时 placementPos() 等于 blockPos()。
     *
     * @return 是否替换点击的方块
     */
    [[nodiscard]] bool replacingClickedBlock() const { return m_replacingClickedBlock; }

    // ========== 方向相关 ==========

    /**
     * @brief 获取玩家水平朝向
     *
     * 根据玩家的 yaw 角度计算面向的方向。
     * 返回 NORTH, SOUTH, EAST 或 WEST。
     *
     * @return 水平方向
     */
    [[nodiscard]] Direction horizontalDirection() const { return m_horizontalDirection; }

    /**
     * @brief 获取放置时的面向方向
     *
     * 对于需要朝向的方块（如楼梯、门），返回方块应该面向的方向。
     * 通常是玩家的朝向的反方向。
     *
     * @return 方块应该面向的方向
     */
    [[nodiscard]] Direction placementDirection() const;

    /**
     * @brief 获取放置位置当前方块状态
     * @return 方块状态，如果位置无效返回 nullptr
     */
    [[nodiscard]] const BlockState* getBlockStateAtPlacementPos() const;

private:
    /**
     * @brief 初始化放置上下文
     */
    void initialize();

    /**
     * @brief 检查方块是否可替换
     * @param pos 方块位置
     * @return 是否可替换
     */
    [[nodiscard]] bool canReplace(const BlockPos& pos) const;

    BlockPos m_adjacentPos;          // 相邻位置（击中面的另一侧）
    BlockPos m_placementPos;         // 实际放置位置
    bool m_replacingClickedBlock;    // 是否替换点击的方块
    Direction m_horizontalDirection; // 玩家水平朝向
};

} // namespace mc
