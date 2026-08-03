/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"

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
     * @param hand 使用的手
     * @param playerYaw 玩家yaw角度
     * @param playerPitch 玩家pitch角度（度数，正值俯视，负值仰视）
     */
    ItemUseContext(IWorld& world,
        Player* player,
        const ItemStack& stack,
        const Vector3& hitPos,
        const BlockPos& blockPos,
        Direction face,
        Hand hand,
        f32 playerYaw,
        f32 playerPitch);

    virtual ~ItemUseContext() = default;

    // ========== 访问器 ==========

    /**
     * @brief 获取世界（只读）
     */
    [[nodiscard]] const IWorld& world() const { return m_world; }

    /**
     * @brief 获取世界引用（可修改）
     */
    [[nodiscard]] IWorld& world() { return m_world; }

    /**
     * @brief 获取世界引用（别名）
     */
    [[nodiscard]] IWorld& getWorld() { return m_world; }

    /**
     * @brief 获取世界引用（只读别名）
     */
    [[nodiscard]] const IWorld& getWorld() const { return m_world; }

    /**
     * @brief 获取玩家（可为nullptr）
     */
    [[nodiscard]] Player* player() const { return m_player; }

    /**
     * @brief 获取玩家（别名）
     */
    [[nodiscard]] Player* getPlayer() const { return m_player; }

    /**
     * @brief 获取物品堆
     */
    [[nodiscard]] const ItemStack& itemStack() const { return *m_stack; }

    /**
     * @brief 获取物品堆（别名）
     */
    [[nodiscard]] const ItemStack& getItemStack() const { return *m_stack; }

    /**
     * @brief 获取物品堆（可修改）
     */
    [[nodiscard]] ItemStack& getItemStackMut() { return *m_stack; }

    /**
     * @brief 获取使用的手（主手或副手）
     * @return 手枚举值
     */
    [[nodiscard]] Hand getHand() const { return m_hand; }

    /**
     * @brief 获取玩家 yaw 角度
     * @return yaw 角度
     */
    [[nodiscard]] f32 getPlayerYaw() const { return m_playerYaw; }

    /**
     * @brief 获取玩家 pitch 角度
     * @return pitch 角度（度数，正值俯视，负值仰视）
     */
    [[nodiscard]] f32 getPlayerPitch() const { return m_playerPitch; }

    /**
     * @brief 获取击中点（世界坐标）
     */
    [[nodiscard]] const Vector3& hitPosition() const { return m_hitPos; }

    /**
     * @brief 获取击中点（别名）
     */
    [[nodiscard]] const Vector3& getHitPos() const { return m_hitPos; }

    /**
     * @brief 获取击中的方块位置
     */
    [[nodiscard]] const BlockPos& blockPos() const { return m_blockPos; }

    /**
     * @brief 获取击中的方块位置（别名）
     */
    [[nodiscard]] const BlockPos& getBlockPos() const { return m_blockPos; }

    /**
     * @brief 获取击中的面
     */
    [[nodiscard]] Direction face() const { return m_face; }

    /**
     * @brief 获取击中的面（别名）
     */
    [[nodiscard]] Direction getFace() const { return m_face; }

    /**
     * @brief 获取击中的面（别名，与face()相同）
     */
    [[nodiscard]] Direction getClickedFace() const { return m_face; }

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
    IWorld& m_world; // 使用非 const 引用以支持修改操作
    Player* m_player;
    ItemStack* m_stack; // 使用指针以支持子类的修改
    Vector3 m_hitPos;
    BlockPos m_blockPos;
    Direction m_face;
    Hand m_hand;
    f32 m_playerYaw = 0.0f;
    f32 m_playerPitch = 0.0f;

    // 击中点在方块内的相对坐标（0-1范围）
    f32 m_hitX = 0.0f;
    f32 m_hitY = 0.0f;
    f32 m_hitZ = 0.0f;
};

} // namespace mc
