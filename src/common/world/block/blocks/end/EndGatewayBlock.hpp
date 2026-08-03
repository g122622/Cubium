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

#include "../../Block.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include <memory>

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 末地折跃门方块
 *
 * 在末地城和主岛之间传送的方块。
 *
 * 特点：
 * - 无碰撞箱（实体可以直接穿过）
 * - 关联 EndGatewayEntity 方块实体处理传送逻辑
 * - 实体进入时触发传送
 * - 有传送冷却（100 tick）
 */
class EndGatewayBlock : public Block {
public:
    explicit EndGatewayBlock(const BlockProperties& properties);
    ~EndGatewayBlock() override = default;

    // ========== 方块实体 ==========

    /**
     * @brief 检查是否有方块实体
     * @return 末地折跃门有关联的方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建方块实体
     * @param pos 方块位置
     * @return 新创建的 EndGatewayEntity
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    /**
     * @brief 获取方块实体类型
     * @return BlockEntityType::EndGateway
     */
    [[nodiscard]] BlockEntityType getBlockEntityType() const { return BlockEntityType::EndGateway; }

    // ========== 实体交互 ==========

    /**
     * @brief 实体碰撞时触发传送
     *
     * 当实体进入折跃门方块时，触发传送逻辑。
     * 实际传送由 EndGatewayEntity::tick() 处理。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param entity 碰撞的实体
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
