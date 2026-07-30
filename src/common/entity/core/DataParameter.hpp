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
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <functional>
#include <variant>
#include <vector>

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::f32;
using mc::i32;
using mc::i64;
using mc::i8;
using mc::u16;
using mc::u8;

using mc::Vector2f;
using mc::Vector3d;
using mc::Vector3f;
using mc::Vector3i;

/**
 * @brief 数据序列化器类型枚举
 *
 * 定义数据参数的类型，用于网络同步。
 * 每种类型对应不同的序列化方式。
 */
enum class DataSerializerType : u8 {
    Byte = 0,              // i8
    Int = 1,               // i32
    Long = 2,              // i64
    Float = 3,             // f32
    String = 4,            // String
    Component = 5,         // 文本组件（暂用String）
    ItemStack = 6,         // 物品堆
    Boolean = 7,           // bool
    Rotation = 8,          // Vector3f (旋转)
    BlockPos = 9,          // Vector3i (方块位置)
    Direction = 10,        // BlockFace
    OptionalInt = 11,      // std::optional<i32>
    ParticleData = 12,     // 粒子数据
    VillagerData = 13,     // 村民数据
    OptionalBlockPos = 14, // std::optional<Vector3i>
    CompoundTag = 15,      // NBT数据
    Vector3f = 16,         // Vector3f
    Quaternion = 17,       // 四元数
    UUID = 18,             // UUID
    OptionalVector3f = 19, // std::optional<Vector3f>
};

/**
 * @brief 数据参数键
 *
 * 类型安全的数据键，用于 EntityDataManager。
 * 每个键有唯一的ID和数据类型。
 *
 * 使用方式：
 * @code
 * // 定义静态数据参数
 * static DataParameter<i32> HEALTH = EntityDataManager::createKey<i32>();
 *
 * // 在实体中使用
 * m_dataManager.set(HEALTH, 20);
 * i32 health = m_dataManager.get(HEALTH);
 * @endcode
 */
template <typename T>
class DataParameter {
public:
    /**
     * @brief 构造数据参数
     * @param id 参数ID
     *
     * createKey 返回哨兵 kUnassignedId（0xFFFF），真正 id 由 EntityDataManager::registerParam
     * 在运行时按继承链分配后经 assignId 写入。
     */
    explicit constexpr DataParameter(u16 id)
        : m_id(id)
    {}

    /**
     * @brief 获取参数ID
     */
    [[nodiscard]] constexpr u16 id() const { return m_id; }

    /**
     * @brief 运行时分配真实 id（由 EntityDataManager::registerParam 调用）
     *
     * 对齐 vanilla ClassTreeIdRegistry：id 在 registerData 时沿继承链分配，
     * 而非静态 createKey 时。静态成员在 createKey 后持哨兵 0xFFFF，registerParam
     * 填入真实 id，之后 set/get 用 id() 取得正确值。
     */
    void assignId(u16 id) noexcept { m_id = id; }

    /**
     * @brief 获取参数类型
     */
    [[nodiscard]] DataSerializerType type() const noexcept;

    /**
     * @brief 比较操作符
     */
    [[nodiscard]] bool operator==(const DataParameter& other) const noexcept { return m_id == other.m_id; }
    [[nodiscard]] bool operator!=(const DataParameter& other) const noexcept { return m_id != other.m_id; }

private:
    u16 m_id;
};

// 类型特化：获取序列化类型
template <>
inline DataSerializerType DataParameter<i8>::type() const noexcept
{
    return DataSerializerType::Byte;
}
template <>
inline DataSerializerType DataParameter<i32>::type() const noexcept
{
    return DataSerializerType::Int;
}
template <>
inline DataSerializerType DataParameter<i64>::type() const noexcept
{
    return DataSerializerType::Long;
}
template <>
inline DataSerializerType DataParameter<f32>::type() const noexcept
{
    return DataSerializerType::Float;
}
template <>
inline DataSerializerType DataParameter<std::string>::type() const noexcept
{
    return DataSerializerType::String;
}
template <>
inline DataSerializerType DataParameter<bool>::type() const noexcept
{
    return DataSerializerType::Boolean;
}
template <>
inline DataSerializerType DataParameter<Vector3i>::type() const noexcept
{
    return DataSerializerType::BlockPos;
}
template <>
inline DataSerializerType DataParameter<Vector2f>::type() const noexcept
{
    return DataSerializerType::Rotation;
}
template <>
inline DataSerializerType DataParameter<Vector3f>::type() const noexcept
{
    return DataSerializerType::Vector3f;
}
template <>
inline DataSerializerType DataParameter<Vector3d>::type() const noexcept
{
    return DataSerializerType::Vector3f;
}

} // namespace mc::entity
