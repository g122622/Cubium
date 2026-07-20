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
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"

#include <optional>

namespace mc {

class Entity; // 前向声明
namespace server {
class ServerWorld; // 前向声明
} // namespace server

namespace gameevent {

/**
 * @brief 位置源接口
 *
 * 表示游戏事件监听器的位置来源。位置可以是固定的方块位置
 * （BlockPositionSource，如幽匿感测体）或跟随实体移动的位置
 * （EntityPositionSource，如监守者）。
 *
 */
class PositionSource {
public:
    virtual ~PositionSource() = default;

    /**
     * @brief 获取监听器的当前位置
     * @param world 世界引用（用于解析实体位置）
     * @return 监听器当前位置，如果无法确定则返回空
     */
    [[nodiscard]] virtual std::optional<Vector3d> getPosition(const server::ServerWorld& world) const = 0;

    /**
     * @brief 获取位置源类型标识
     * @return 位置源类型名称
     */
    [[nodiscard]] virtual const char* type() const noexcept = 0;
};

/**
 * @brief 方块位置源
 *
 * 固定的方块位置，用于附着在方块上的监听器（如幽匿感测体、幽匿尖啸体）。
 * 位置始终为方块中心坐标 (x+0.5, y+0.5, z+0.5)。
 *
 */
class BlockPositionSource final : public PositionSource {
public:
    explicit BlockPositionSource(const BlockPos& pos)
        : m_pos(pos)
    {}

    [[nodiscard]] std::optional<Vector3d> getPosition(const server::ServerWorld& /*world*/) const override
    {
        return Vector3d(m_pos.x + 0.5, m_pos.y + 0.5, m_pos.z + 0.5);
    }

    [[nodiscard]] const char* type() const noexcept override { return "block"; }

    [[nodiscard]] const BlockPos& pos() const noexcept { return m_pos; }

private:
    BlockPos m_pos;
};

/**
 * @brief 实体位置源
 *
 * 跟随实体移动的位置，用于实体上的监听器（如监守者、悦灵）。
 * 位置从实体的当前位置获取，支持Y轴偏移。
 *
 */
class EntityPositionSource final : public PositionSource {
public:
    /**
     * @brief 构造实体位置源
     * @param entityId 实体ID
     * @param yOffset Y轴偏移（如眼睛高度）
     */
    EntityPositionSource(EntityInstanceId entityId, f32 yOffset = 0.0f)
        : m_entityId(entityId)
        , m_yOffset(yOffset)
    {}

    [[nodiscard]] std::optional<Vector3d> getPosition(const server::ServerWorld& world) const override;

    [[nodiscard]] const char* type() const noexcept override { return "entity"; }

    [[nodiscard]] EntityInstanceId entityId() const noexcept { return m_entityId; }
    [[nodiscard]] f32 yOffset() const noexcept { return m_yOffset; }

private:
    EntityInstanceId m_entityId;
    f32 m_yOffset;
};

} // namespace gameevent

} // namespace mc
