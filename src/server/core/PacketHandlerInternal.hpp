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
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {
class Entity;
class IWorld;
} // namespace mc

namespace mc::server::core {

class ConnectionManager;

namespace detail {

/// moved wrongly 判定阈值（平方距离），对应 MC Java 的 0.0625（0.25² 格）
constexpr f64 kMovedWronglyThresholdSq = 0.0625;

/// moved too quickly 判定阈值（平方距离），对应 MC Java 的 100.0
constexpr f64 kMaxVehicleSpeedSq = 100.0;

/**
 * @brief 发送载具位置校正包到客户端
 *
 * 对应 MC Java ServerGamePacketListenerImpl 中
 * `this.send(ClientboundMoveVehiclePacket.fromEntity(entity))` 校正逻辑。
 * 使用载具当前的朝向与指定的校正位置构造 VehicleMovePacket 并发送给指定玩家。
 *
 * @param connectionManager 连接管理器
 * @param playerId 目标玩家 ID
 * @param vehicle 载具实体（取其 yaw/pitch）
 * @param correctionPos 校正位置（通常是服务端已知的载具旧位置）
 */
void sendVehicleMoveCorrection(
    ConnectionManager& connectionManager, PlayerId playerId, const mc::Entity& vehicle, const Vector3& correctionPos);

/**
 * @brief 检测载具移动到目标位置后是否与"新"实体发生碰撞
 *
 * 对应 MC Java ServerGamePacketListenerImpl.isEntityCollidingWithAnythingNew：
 * 取移动前的 AABB，分别沿 X/Y/Z 轴偏移到目标位置，检查是否与任何
 * `canBeCollidedWith()` 的实体（排除自身）碰撞。
 *
 * Cubium 的 EntityManager::getEntitiesInAABB 不会过滤 canBeCollidedWith，
 * 因此需在调用方手动过滤，对齐 MC Java 的 isPickable 语义。
 *
 * 进一步对齐 MC Java 的 getEntityCollisions 谓词
 * `entity::canCollideWith`：当候选实体与载具同处一条骑乘链时
 * （isPassengerOfSameVehicle），不视为"新"碰撞 —— 这避免载具把
 * 自己的乘客误判为碰撞物而触发回退。
 *
 * @param world 世界
 * @param vehicle 载具实体（用于排除自身与获取碰撞边界）
 * @param oldAABB 移动前的 AABB
 * @param targetPos 客户端请求的目标位置
 * @return true 若目标位置会与任何可碰撞实体相交
 */
bool isEntityCollidingWithAnythingNew(
    const mc::IWorld& world, const mc::Entity& vehicle, const AxisAlignedBB& oldAABB, const Vector3& targetPos);

} // namespace detail

} // namespace mc::server::core
