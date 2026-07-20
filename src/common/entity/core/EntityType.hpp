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

#include "EntityClassification.hpp"
#include "EntitySize.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

// 前向声明
class Entity;
class EntityTypeTag;
class IWorld;

namespace entity {

// 引入 mc 命名空间的类型
using mc::f32;
using mc::i32;
using mc::u16;
using mc::u32;

/**
 * @brief 实体类型ID
 *
 * 用于唯一标识实体类型的数字ID。
 * 正整数ID由注册表自动分配，负数ID保留给特殊用途。
 */
using EntityTypeId = u16;

/**
 * @brief 实体类型标志
 *
 * 定义实体类型的各种属性标志
 */
enum class EntityFlags : u32 {
    None = 0,
    ImmuneToFire = 1 << 0,     // 免疫火焰
    ImmuneToLava = 1 << 1,     // 免疫岩浆
    CanSummon = 1 << 2,        // 可以被召唤
    Serializable = 1 << 3,     // 可以序列化到NBT
    ImmuneToDrowning = 1 << 4, // 免疫溺水
    ImmuneToFall = 1 << 5,     // 免疫摔落伤害
};

inline EntityFlags operator|(EntityFlags a, EntityFlags b)
{
    return static_cast<EntityFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline EntityFlags operator&(EntityFlags a, EntityFlags b)
{
    return static_cast<EntityFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool hasEntityFlag(EntityFlags flags, EntityFlags flag)
{
    return (static_cast<u32>(flags) & static_cast<u32>(flag)) != 0;
}

/**
 * @brief 实体工厂函数类型
 *
 * 用于创建实体实例的函数指针。
 * 返回 std::unique_ptr<Entity> 以支持多态。
 */
using EntityFactory = std::function<std::unique_ptr<Entity>(IWorld*)>;

/**
 * @brief 实体类型基类
 *
 * 存储实体类型的各种属性，包括：
 * - 分类（怪物、动物等）
 * - 尺寸（宽度、高度）
 * - 追踪距离
 * - 更新频率
 * - 各种标志
 */
class EntityType {
public:
    /**
     * @brief 实体类型构建器
     *
     * 用于流式构建实体类型配置
     */
    class Builder {
    public:
        explicit Builder(EntityFactory factory, EntityClassification classification)
            : m_factory(std::move(factory))
            , m_classification(classification)
            , m_size(EntitySize::flexible(0.6f, 1.8f))
            , m_trackingRange(5)
            , m_updateInterval(3)
            , m_flags(EntityFlags::Serializable)
        {}

        /**
         * @brief 设置实体尺寸
         * @param width 宽度
         * @param height 高度
         * @return Builder引用
         */
        Builder& size(f32 width, f32 height)
        {
            m_size = EntitySize::flexible(width, height);
            return *this;
        }

        /**
         * @brief 设置固定尺寸
         * @param width 宽度
         * @param height 高度
         * @return Builder引用
         */
        Builder& fixedSize(f32 width, f32 height)
        {
            m_size = EntitySize::fixed(width, height);
            return *this;
        }

        /**
         * @brief 设置追踪距离
         * @param range 区块距离
         * @return Builder引用
         */
        Builder& trackingRange(i32 range)
        {
            m_trackingRange = range;
            return *this;
        }

        /**
         * @brief 设置更新间隔
         * @param interval tick间隔
         * @return Builder引用
         */
        Builder& updateInterval(i32 interval)
        {
            m_updateInterval = interval;
            return *this;
        }

        /**
         * @brief 设置火焰免疫
         * @return Builder引用
         */
        Builder& immuneToFire()
        {
            m_flags = m_flags | EntityFlags::ImmuneToFire;
            return *this;
        }

        /**
         * @brief 设置岩浆免疫
         * @return Builder引用
         */
        Builder& immuneToLava()
        {
            m_flags = m_flags | EntityFlags::ImmuneToLava;
            return *this;
        }

        /**
         * @brief 禁用序列化
         * @return Builder引用
         */
        Builder& disableSerialization()
        {
            m_flags =
                static_cast<EntityFlags>(static_cast<u32>(m_flags) & ~static_cast<u32>(EntityFlags::Serializable));
            return *this;
        }

        /**
         * @brief 设置可召唤
         * @return Builder引用
         */
        Builder& canSummon(bool value = true)
        {
            if (value) {
                m_flags = m_flags | EntityFlags::CanSummon;
            } else {
                m_flags =
                    static_cast<EntityFlags>(static_cast<u32>(m_flags) & ~static_cast<u32>(EntityFlags::CanSummon));
            }
            return *this;
        }

        /**
         * @brief 构建实体类型
         * @return 配置好的EntityType
         */
        EntityType build() const
        {
            return EntityType(m_factory, m_classification, m_size, m_trackingRange, m_updateInterval, m_flags);
        }

    private:
        EntityFactory m_factory;
        EntityClassification m_classification;
        EntitySize m_size;
        i32 m_trackingRange;
        i32 m_updateInterval;
        EntityFlags m_flags;
    };

    // 默认构造函数
    EntityType() = default;

    /**
     * @brief 析构函数
     *
     * 必须在 cpp 文件中定义，因为 unique_ptr<Entity> 需要完整类型
     */
    ~EntityType();

    /**
     * @brief 移动构造函数
     */
    EntityType(EntityType&&) noexcept = default;

    /**
     * @brief 移动赋值运算符
     */
    EntityType& operator=(EntityType&&) noexcept = default;

    // 禁止拷贝
    EntityType(const EntityType&) = delete;
    EntityType& operator=(const EntityType&) = delete;

    /**
     * @brief 构造实体类型
     */
    EntityType(EntityFactory factory,
        EntityClassification classification,
        EntitySize size,
        i32 trackingRange,
        i32 updateInterval,
        EntityFlags flags)
        : m_factory(std::move(factory))
        , m_classification(classification)
        , m_size(size)
        , m_trackingRange(trackingRange)
        , m_updateInterval(updateInterval)
        , m_flags(flags)
    {}

    /**
     * @brief 创建实体实例
     * @param world 世界实例
     * @return 实体实例，如果工厂无效则返回nullptr
     */
    std::unique_ptr<Entity> create(IWorld* world) const;

    /**
     * @brief 获取实体分类
     */
    [[nodiscard]] EntityClassification classification() const { return m_classification; }

    /**
     * @brief 获取实体尺寸
     */
    [[nodiscard]] EntitySize size() const { return m_size; }

    /**
     * @brief 获取追踪距离（区块）
     */
    [[nodiscard]] i32 trackingRange() const { return m_trackingRange; }

    /**
     * @brief 获取更新间隔（tick）
     */
    [[nodiscard]] i32 updateInterval() const { return m_updateInterval; }

    /**
     * @brief 获取实体标志
     */
    [[nodiscard]] EntityFlags flags() const { return m_flags; }

    /**
     * @brief 检查是否有特定标志
     */
    [[nodiscard]] bool hasFlag(EntityFlags flag) const { return entity::hasEntityFlag(m_flags, flag); }

    /**
     * @brief 是否免疫火焰
     */
    [[nodiscard]] bool immuneToFire() const { return hasFlag(EntityFlags::ImmuneToFire); }

    /**
     * @brief 是否免疫岩浆
     */
    [[nodiscard]] bool immuneToLava() const { return hasFlag(EntityFlags::ImmuneToLava); }

    /**
     * @brief 是否可以序列化
     */
    [[nodiscard]] bool serializable() const { return hasFlag(EntityFlags::Serializable); }

    /**
     * @brief 是否可以被召唤
     */
    [[nodiscard]] bool canSummon() const { return hasFlag(EntityFlags::CanSummon); }

    /**
     * @brief 获取ID
     */
    [[nodiscard]] EntityTypeId id() const { return m_id; }

    /**
     * @brief 获取名称
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return m_factory != nullptr; }

    /**
     * @brief 检查此实体类型是否属于指定标签
     *
     * 通过实体类型名称匹配标签中的成员。
     *
     * @param tag 实体类型标签
     * @return 如果此实体类型在标签中返回 true
     */
    [[nodiscard]] bool isIn(const EntityTypeTag& tag) const;

    /**
     * @brief 比较操作符
     */
    bool operator==(const EntityType& other) const { return m_id == other.m_id; }
    bool operator!=(const EntityType& other) const { return m_id != other.m_id; }

private:
    friend class EntityRegistry;

    /**
     * @brief 绑定实体类型的数字ID与资源位置名
     *
     * 仅由 EntityRegistry::registerType 在注册时调用，用于把注册顺序分配的数字 ID
     * 与权威的资源位置字符串注入到 EntityType。封装写权，避免外部 const_cast。
     *
     * @param id 注册表分配的数字 ID
     * @param name 资源位置（如 minecraft:pig）
     */
    void bindIdentity(EntityTypeId id, std::string name)
    {
        m_id = id;
        m_name = std::move(name);
    }

    EntityFactory m_factory;
    EntityClassification m_classification = EntityClassification::Misc;
    EntitySize m_size = EntitySize::flexible(0.6f, 1.8f);
    i32 m_trackingRange = 5;
    i32 m_updateInterval = 3;
    EntityFlags m_flags = EntityFlags::Serializable;
    EntityTypeId m_id = 0;
    std::string m_name;
};

} // namespace entity

// 注意：不使用 using EntityType = entity::EntityType;
// 因为 Constants.hpp 中已有 entity::EntityTypeId 枚举
// 使用时请明确使用 mc::entity::EntityType

} // namespace mc
