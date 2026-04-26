#pragma once

#include "../../core/Types.hpp"
#include <functional>
#include <variant>
#include <vector>

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::u8;
using mc::u16;
using mc::i8;
using mc::i32;
using mc::i64;
using mc::f32;
using mc::String;
using mc::Vector3i;
using mc::Vector2f;
using mc::Vector3f;
using mc::Vector3d;

/**
 * @brief 数据序列化器类型枚举
 *
 * 定义数据参数的类型，用于网络同步。
 * 每种类型对应不同的序列化方式。
 *
 * 参考 MC 1.16.5 DataSerializers
 */
enum class DataSerializerType : u8 {
    Byte = 0,       // i8
    Int = 1,        // i32
    Long = 2,       // i64
    Float = 3,      // f32
    String = 4,     // String
    Component = 5,  // 文本组件（暂用String）
    ItemStack = 6,  // 物品堆
    Boolean = 7,    // bool
    Rotation = 8,   // Vector3f (旋转)
    BlockPos = 9,   // Vector3i (方块位置)
    Direction = 10, // BlockFace
    OptionalInt = 11,   // std::optional<i32>
    ParticleData = 12,  // 粒子数据
    VillagerData = 13,  // 村民数据
    OptionalBlockPos = 14,  // std::optional<Vector3i>
    CompoundTag = 15,  // NBT数据
    Vector3f = 16,    // Vector3f
    Quaternion = 17,  // 四元数
    UUID = 18,        // UUID
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
 *
 * 参考 MC 1.16.5 DataParameter
 */
template<typename T>
class DataParameter {
public:
    /**
     * @brief 构造数据参数
     * @param id 参数ID
     */
    explicit constexpr DataParameter(u16 id)
        : m_id(id)
    {}

    /**
     * @brief 获取参数ID
     */
    [[nodiscard]] constexpr u16 id() const { return m_id; }

    /**
     * @brief 获取参数类型
     */
    [[nodiscard]] DataSerializerType type() const;

    /**
     * @brief 比较操作符
     */
    bool operator==(const DataParameter& other) const { return m_id == other.m_id; }
    bool operator!=(const DataParameter& other) const { return m_id != other.m_id; }

private:
    u16 m_id;
};

// 类型特化：获取序列化类型
template<> inline DataSerializerType DataParameter<i8>::type() const { return DataSerializerType::Byte; }
template<> inline DataSerializerType DataParameter<i32>::type() const { return DataSerializerType::Int; }
template<> inline DataSerializerType DataParameter<i64>::type() const { return DataSerializerType::Long; }
template<> inline DataSerializerType DataParameter<f32>::type() const { return DataSerializerType::Float; }
template<> inline DataSerializerType DataParameter<String>::type() const { return DataSerializerType::String; }
template<> inline DataSerializerType DataParameter<bool>::type() const { return DataSerializerType::Boolean; }
template<> inline DataSerializerType DataParameter<Vector3i>::type() const { return DataSerializerType::BlockPos; }
template<> inline DataSerializerType DataParameter<Vector2f>::type() const { return DataSerializerType::Rotation; }
template<> inline DataSerializerType DataParameter<Vector3f>::type() const { return DataSerializerType::Vector3f; }
template<> inline DataSerializerType DataParameter<Vector3d>::type() const { return DataSerializerType::Vector3f; }

} // namespace mc::entity
