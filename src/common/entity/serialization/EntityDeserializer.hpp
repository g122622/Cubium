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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <vector>

namespace mc {
class Entity;
class IWorld;

namespace ecs {
class EntityRegistry;
} // namespace ecs

namespace entity::serialization {

/**
 * @brief 实体反序列化器
 *
 * 从 NBT 数据创建实体实例。
 *
 * 流程：
 * 1. 读取 "id" 标签获取实体类型字符串
 * 2. 通过 EntityRegistry 查找 EntityType
 * 3. 调用 EntityType::create() 创建实例
 * 4. 调用 Entity::readFromNBT() 填充数据
 * 5. Passengers 标签暂存到实体的 m_pendingPassengersNbt（不在反序列化阶段 spawn 乘客）
 *
 * 乘客挂载流程：
 * - 主实体被调用方 spawnEntity 注入世界、拿到真实 id 之后，
 *   调用 attachPassengers(vehicle, world) 递归 spawn 乘客并 startRiding。
 * - 这样保证乘客的 m_vehicle 指向主实体的真实 id，避免
 *   "vehicle id 从 0 改写为真实 id 后乘客 m_vehicle 失效"的缺陷。
 */
class EntityDeserializer {
public:
    /**
     * @brief 从 NBT 反序列化实体
     *
     * 本方法仅反序列化主实体本身（含 readFromNBT），不处理 Passengers。
     * 若 NBT 中含 Passengers 标签，会将其暂存到实体的 m_pendingPassengersNbt 字段，
     * 由调用方在 spawn 主实体后调用 attachPassengers 处理。
     *
     * @param tag NBT 复合标签
     * @param registry ECS 实体注册表。EntityType::create 在此 registry 内 create ECS 实体
     *   并 attach 高频组件（entt 实体不可跨 registry 迁移，故构造时 registry 必须就位）。
     *   调用方（chunk 加载/存档读入）由所在 ServerWorld 透传 `world.entityManager().registry()`。
     * @return 实体实例或错误
     */
    static Result<std::unique_ptr<Entity>> deserialize(const nbt::tags::compound_tag& tag, ecs::EntityRegistry& registry);

    /**
     * @brief 从二进制数据反序列化实体
     *
     * 本方法仅反序列化主实体本身，Passengers 处理同 deserialize(tag, registry)。
     *
     * @param data 压缩的 NBT 二进制数据
     * @param registry ECS 实体注册表，透传给 deserialize
     * @return 实体实例或错误
     */
    static Result<std::unique_ptr<Entity>> deserializeFromBinary(const std::vector<u8>& data, ecs::EntityRegistry& registry);

    /**
     * @brief 挂载主实体的待处理乘客
     *
     * 在主实体被 spawnEntity 注入世界、拿到真实 id 之后调用。本方法会：
     * 1. 取走主实体的 m_pendingPassengersNbt 列表
     * 2. 对每个乘客 NBT 递归调用 deserialize 构造乘客实体
     * 3. 调用 world.spawnEntity 把乘客注入世界（乘客拿到真实 id）
     * 4. 调用 passenger.startRiding(vehicle) 建立骑乘关系
     *    （此时 vehicle.id() 已是真实 id，乘客的 m_vehicle 会被正确设置）
     * 5. 递归处理乘客自身的 m_pendingPassengersNbt（多层骑乘）
     *
     * 若主实体没有待处理乘客（hasPendingPassengersNbt() 为 false），本方法为空操作。
     *
     * @param vehicle 已 spawn 的主实体（必须已注入 world，id 非 0）
     * @param world 主实体所在的世界
     * @return 成功或错误（任一乘客挂载失败则返回错误，已挂载的乘客不会被回滚）
     */
    static Result<void> attachPassengers(Entity& vehicle, IWorld& world);

    /**
     * @brief 将实体序列化为二进制数据
     * @param entity 实体引用
     * @return 压缩的 NBT 二进制数据或错误
     */
    static Result<std::vector<u8>> serializeToBinary(const Entity& entity);
};

} // namespace entity::serialization
} // namespace mc
