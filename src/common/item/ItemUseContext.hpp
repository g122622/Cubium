#pragma once

#include "../core/Types.hpp"
#include "../util/math/Vector3.hpp"
#include "../util/Direction.hpp"
#include "../world/block/BlockPos.hpp"
#include "../world/block/Block.hpp"
#include "../util/math/ray/Raycast.hpp"

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 物品使用上下文
 *
 * 提供物品使用时的上下文信息，包括世界、玩家、物品堆和射线检测结果。
 * 这是所有物品使用上下文的基类。
 *
 * 参考: net.minecraft.item.ItemUseContext
 */
class ItemUseContext {
public:
    /**
     * @brief 构造物品使用上下文
     * @param world 世界引用（IWorld接口）
     * @param player 玩家指针（可为nullptr）
     * @param stack 物品堆
     * @param hitPos 击中点（世界坐标）
     * @param blockPos 击中的方块位置
     * @param face 击中的面
     */
    ItemUseContext(const IWorld& world,
                   Player* player,
                   const ItemStack& stack,
                   const Vector3& hitPos,
                   const BlockPos& blockPos,
                   Direction face);

    virtual ~ItemUseContext() = default;

    // ========== 访问器 ==========

    /**
     * @brief 获取世界（只读）
     */
    [[nodiscard]] const IWorld& world() const { return m_world; }

    /**
     * @brief 获取玩家（可为nullptr）
     */
    [[nodiscard]] Player* player() const { return m_player; }

    /**
     * @brief 获取物品堆
     */
    [[nodiscard]] const ItemStack& itemStack() const { return m_stack; }

    /**
     * @brief 获取击中点（世界坐标）
     */
    [[nodiscard]] const Vector3& hitPosition() const { return m_hitPos; }

    /**
     * @brief 获取击中的方块位置
     */
    [[nodiscard]] const BlockPos& blockPos() const { return m_blockPos; }

    /**
     * @brief 获取击中的面
     */
    [[nodiscard]] Direction face() const { return m_face; }

    /**
     * @brief 获取击中点在方块内的相对坐标（0-1范围）
     * @return 相对坐标向量
     */
    [[nodiscard]] Vector3 hitPositionInBlock() const;

    /**
     * @brief 获取击中点在指定轴上的相对坐标
     * @param axis 坐标轴
     * @return 相对坐标（0-1范围）
     */
    [[nodiscard]] f32 getHitU(Axis axis) const;

    /**
     * @brief 获取击中点在X轴的相对坐标
     */
    [[nodiscard]] f32 getHitX() const { return m_hitX; }

    /**
     * @brief 获取击中点在Y轴的相对坐标
     */
    [[nodiscard]] f32 getHitY() const { return m_hitY; }

    /**
     * @brief 获取击中点在Z轴的相对坐标
     */
    [[nodiscard]] f32 getHitZ() const { return m_hitZ; }

    /**
     * @brief 检查上下文是否有效
     */
    [[nodiscard]] bool isValid() const { return m_face != Direction::None; }

protected:
    const IWorld& m_world;
    Player* m_player;
    const ItemStack& m_stack;
    Vector3 m_hitPos;
    BlockPos m_blockPos;
    Direction m_face;

    // 击中点在方块内的相对坐标（0-1范围）
    f32 m_hitX = 0.0f;
    f32 m_hitY = 0.0f;
    f32 m_hitZ = 0.0f;
};

} // namespace mc
