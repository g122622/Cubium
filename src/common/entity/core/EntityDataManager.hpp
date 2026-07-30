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

#include "DataParameter.hpp"
#include "EntityClassRegistry.hpp"
#include "EntityPose.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include <mutex>
#include <unordered_map>
#include <variant>
#include <vector>

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::f32;
using mc::i32;
using mc::i64;
using mc::i8;
using mc::u16;

using mc::Vector2f;
using mc::Vector3f;
using mc::Vector3i;

/**
 * @brief Pose 字段值包装
 *
 * 对齐 vanilla 1.21.11 Entity.DATA_POSE（Pose serializer，EntityDataSerializers id=20）。
 * wire 上以 VarInt(EntityPose 枚举值=vanilla Pose.id) 传输，与 Byte(serializerId=0，wire 单字节)
 * 不同，故需独立 variant 类型区分。
 */
struct PoseValue {
    EntityPose value{EntityPose::Standing};

    PoseValue() = default;
    explicit PoseValue(EntityPose v) noexcept
        : value(v)
    {}

    bool operator==(const PoseValue& other) const noexcept { return value == other.value; }
    bool operator!=(const PoseValue& other) const noexcept { return value != other.value; }
};

/**
 * @brief Optional<Component> 字段值包装
 *
 * 对齐 vanilla 1.21.11 Entity.DATA_CUSTOM_NAME（OptionalComponent serializer，
 * EntityDataSerializers id=6）。wire = 1 byte present + 若 present 则 NBT Component
 * （纯文本折叠为 StringTag 0x08 + U16 BE 长度 + UTF8，与 writeTextComponentNbt 同源）。
 */
struct OptionalComponentValue {
    bool present{false};
    std::string text; // present 时为组件纯文本（getUnformattedText）

    OptionalComponentValue() = default;
    OptionalComponentValue(bool p, std::string t) noexcept
        : present(p)
        , text(std::move(t))
    {}

    bool operator==(const OptionalComponentValue& other) const noexcept
    {
        return present == other.present && text == other.text;
    }
    bool operator!=(const OptionalComponentValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief 数据参数值包装
 *
 * 用于存储任意类型的数据参数值
 */
class DataValue {
public:
    // 支持的数据类型
    // 注：ItemStackView（index 9）承载掉落物等实体的物品本体，经 EntityMetadataSerializer
    // 的 serializerId 7（ITEM_STACK）双向 codec 同步。variant 备选项顺序即 index，
    // EntityMetadataSerializer::getSerializerId/serializeEntry/deserialize 按 index 分支。
    // PoseValue（index 10）→ Pose serializer(id=20)；OptionalComponentValue（index 11）→
    // OptionalComponent serializer(id=6)。新增类型须同步更新 EntityMetadataSerializer 三处分支。
    using ValueType = std::variant<i8,
        i32,
        i64,
        f32,
        std::string,
        bool,
        Vector3i,
        Vector2f,
        Vector3f,
        network::ir::play::ItemStackView,
        PoseValue,
        OptionalComponentValue>;

    DataValue() = default;

    template <typename T>
    explicit DataValue(T value)
        : m_value(value)
    {}

    template <typename T>
    [[nodiscard]] T get() const
    {
        return std::get<T>(m_value);
    }

    template <typename T>
    void set(T value)
    {
        m_value = value;
    }

    [[nodiscard]] const ValueType& value() const noexcept { return m_value; }
    [[nodiscard]] size_t index() const noexcept { return m_value.index(); }

    bool operator==(const DataValue& other) const noexcept { return m_value == other.m_value; }

    bool operator!=(const DataValue& other) const noexcept { return m_value != other.m_value; }

private:
    ValueType m_value;
};

/**
 * @brief 数据条目
 *
 * 存储单个数据参数的值和脏标记
 */
struct DataEntry {
    DataValue value;
    bool dirty = false;
};

/**
 * @brief 实体数据管理器
 *
 * 管理实体的同步数据参数。用于：
 * - 客户端-服务端数据同步
 * - 实体状态管理（生命值、燃烧状态等）
 *
 * 使用方式：
 * @code
 * class MyEntity : public Entity {
 *     static DataParameter<i32> HEALTH;
 *
 *     MyEntity() {
 *         m_dataManager.registerParam(HEALTH, 20);
 *     }
 *
 *     void setHealth(i32 health) {
 *         m_dataManager.set(HEALTH, health);
 *     }
 *
 *     i32 getHealth() const {
 *         return m_dataManager.get<i32>(HEALTH);
 *     }
 * };
 * @endcode
 */
class EntityDataManager {
public:
    /**
     * @brief 创建数据参数键
     * @return 持哨兵 id 的数据参数键
     *
     * 不分配真实 id——返回哨兵 kUnassignedId(0xFFFF)。真实 id 由 registerParam 在
     * 运行时按当前正在注册的实体类继承链分配（对齐 vanilla ClassTreeIdRegistry）。
     * 这规避了跨翻译单元静态初始化顺序未定义的风险：父类 classInfo 是否已就绪在
     * 静态期无法保证，但 registerData 运行时父类 registerData 必先执行，父类 id 已分配。
     *
     * 所有 `DataParameter<X> Foo::PARAM = EntityDataManager::createKey<X>();` 调用点
     * 签名不变。
     */
    template <typename T>
    static DataParameter<T> createKey()
    {
        return DataParameter<T>(kUnassignedId);
    }

    /**
     * @brief RAII 守卫：在 registerData 中标记当前正在注册的实体类
     *
     * 构造时压入类标识栈（t_currentClass 指向栈顶），析构时弹栈。registerData 链中
     * 基类 registerData 先执行（其 guard 压入基类 classInfo，分配基类字段，析构弹出），
     * 子类 registerData 再压入子类 classInfo 分配子类字段。allocateIdForCurrentClass
     * 始终按栈顶类沿父链分配，与 vanilla ClassTreeIdRegistry.define(clazz) 一致。
     */
    class ClassRegisterGuard {
    public:
        ClassRegisterGuard(EntityDataManager& dm, const EntityClassInfo& cls)
            : m_dm(dm)
        {
            m_dm.beginClassRegistration(cls);
        }
        ~ClassRegisterGuard() { m_dm.endClassRegistration(); }

        ClassRegisterGuard(const ClassRegisterGuard&) = delete;
        ClassRegisterGuard& operator=(const ClassRegisterGuard&) = delete;

    private:
        EntityDataManager& m_dm;
    };

    /// 压入类标识栈（由 ClassRegisterGuard 调用）
    void beginClassRegistration(const EntityClassInfo& cls)
    {
        t_classStack.push_back(&cls);
        t_currentClass = &cls;
    }

    /// 弹出类标识栈（由 ClassRegisterGuard 调用）
    void endClassRegistration()
    {
        if (!t_classStack.empty()) {
            t_classStack.pop_back();
        }
        t_currentClass = t_classStack.empty() ? nullptr : t_classStack.back();
    }

    /**
     * @brief 为当前正在注册的实体类分配下一个 synched-data id
     *
     * 对齐 vanilla ClassTreeIdRegistry.define(clazz)：沿 t_currentClass 父类链查找
     * 已分配的最高 id（含父类），+1 续接，写入当前类的 lastAssignedId。
     *
     * - Entity(无父)8 字段 → id 0..7，Entity.lastAssignedId=7
     * - VehicleEntity(parent=Entity) → 首字段 id 8
     * - AbstractBoat(parent=VehicleEntity) → 首字段 id 11
     *
     * 若无当前类（t_currentClass==nullptr，如测试直接 registerParam），退化为全局自增
     * t_fallbackId，保持向后兼容。
     */
    u16 allocateIdForCurrentClass()
    {
        if (t_currentClass != nullptr) {
            i32 highest = -1;
            for (const EntityClassInfo* c = t_currentClass; c != nullptr; c = c->parent) {
                const i32 v = c->lastAssignedId.load(std::memory_order_acquire);
                if (v > highest) {
                    highest = v;
                }
            }
            const i32 newId = highest + 1;
            t_currentClass->lastAssignedId.store(newId, std::memory_order_release);
            return static_cast<u16>(newId);
        }
        // 无类上下文（测试/遗留路径）：全局自增兜底
        return static_cast<u16>(t_fallbackId++);
    }

    EntityDataManager() = default;

    /**
     * @brief 注册数据参数
     * @param param 参数键（持哨兵 id，首次注册时填入真实 id）
     * @param defaultValue 默认值
     *
     * 必须在使用前注册所有参数。id 由 allocateIdForCurrentClass 按当前实体类继承链
     * 分配（对齐 vanilla ClassTreeIdRegistry），并经 param.assignId 写回静态成员。
     *
     * 幂等：DataParameter 静态成员被所有同类实体实例共享。首次构造时 param 持哨兵
     * kUnassignedId，分配真实 id 并写回；后续同类实例构造时 param 已持真实 id，
     * 直接复用，不重复分配（否则会沿父链读到已分配的最高 id 再 +1，错乱编号）。
     */
    template <typename T>
    void registerParam(DataParameter<T>& param, T defaultValue)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (param.id() == kUnassignedId) {
            // 静态 DataParameter 被所有同类实体实例共享，首次分配须跨实例互斥，
            // 否则多线程并发首次构造同类实体时会重复分配 id。双重检查锁：全局
            // s_idAllocationMutex 串行化"检查哨兵→分配→写回"序列。
            std::lock_guard<std::mutex> allocLock(s_idAllocationMutex);
            if (param.id() == kUnassignedId) {
                const u16 id = allocateIdForCurrentClass();
                param.assignId(id);
            }
        }
        m_entries[param.id()] = DataEntry{DataValue(std::move(defaultValue)), false};
    }

    /**
     * @brief 设置参数值
     * @param param 参数键
     * @param value 新值
     *
     * 如果值发生变化，标记为脏数据
     */
    template <typename T>
    void set(DataParameter<T> param, T value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(param.id());
        if (it == m_entries.end()) {
            // 参数未注册，创建新条目
            m_entries[param.id()] = DataEntry{DataValue(value), true};
            return;
        }

        DataEntry& entry = it->second;
        if (entry.value.get<T>() != value) {
            entry.value.set(value);
            entry.dirty = true;
        }
    }

    /**
     * @brief 按原始参数ID设置值
     * @param id 参数ID
     * @param value 新值
     * @return 如果值发生变化则返回 true
     */
    bool setRaw(u16 id, const DataValue& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(id);
        if (it == m_entries.end()) {
            m_entries[id] = DataEntry{value, true};
            return true;
        }

        DataEntry& entry = it->second;
        if (entry.value != value) {
            entry.value = value;
            entry.dirty = true;
            return true;
        }

        return false;
    }

    /**
     * @brief 获取参数值
     * @param param 参数键
     * @return 参数值
     */
    template <typename T>
    [[nodiscard]] T get(DataParameter<T> param) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(param.id());
        if (it == m_entries.end()) {
            return T{};
        }
        return it->second.value.template get<T>();
    }

    /**
     * @brief 按原始参数ID获取值
     * @param id 参数ID
     * @return 数据值指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const DataValue* getRaw(u16 id) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(id);
        if (it == m_entries.end()) {
            return nullptr;
        }
        return &it->second.value;
    }

    /**
     * @brief 检查参数是否存在
     * @param id 参数ID
     */
    [[nodiscard]] bool hasParam(u16 id) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_entries.find(id) != m_entries.end();
    }

    /**
     * @brief 获取所有脏数据条目
     * @return 脏数据ID列表
     */
    [[nodiscard]] std::vector<u16> getDirtyParams() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<u16> dirty;
        for (const auto& [id, entry] : m_entries) {
            if (entry.dirty) {
                dirty.push_back(id);
            }
        }
        return dirty;
    }

    /**
     * @brief 清除脏标记
     * @param id 参数ID，如果为 0xFFFF 则清除所有
     */
    void clearDirty(u16 id = 0xFFFF)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (id == 0xFFFF) {
            for (auto& [entryId, entry] : m_entries) {
                entry.dirty = false;
            }
        } else {
            auto it = m_entries.find(id);
            if (it != m_entries.end()) {
                it->second.dirty = false;
            }
        }
    }

    /**
     * @brief 检查是否有脏数据
     */
    [[nodiscard]] bool hasDirtyData() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& [id, entry] : m_entries) {
            if (entry.dirty) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 获取所有数据条目（用于序列化）
     */
    [[nodiscard]] const std::unordered_map<u16, DataEntry>& getAllEntries() const { return m_entries; }

    /**
     * @brief 从其他管理器复制数据
     * @param other 源管理器
     */
    void copyFrom(const EntityDataManager& other)
    {
        // 使用 scoped_lock 避免死锁
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_entries = other.m_entries;
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<u16, DataEntry> m_entries;

    // 哨兵 id：createKey 返回此值，registerParam 填入真实 id
    static constexpr u16 kUnassignedId = 0xFFFF;

    // 静态 DataParameter 首次分配 id 的跨实例互斥量（双重检查锁外层锁）。
    // 注意：这是进程级共享的，因为 DataParameter 是实体类的静态成员，
    // 跨所有 EntityDataManager 实例共享同一个 param.id() 写入。
    static inline std::mutex s_idAllocationMutex;

    // 类标识栈（thread_local）：registerData 链中 ClassRegisterGuard 压入/弹出当前类
    static inline thread_local std::vector<const EntityClassInfo*> t_classStack;
    static inline thread_local const EntityClassInfo* t_currentClass = nullptr;
    // 无类上下文时的全局自增兜底（测试/遗留路径）
    static inline thread_local u32 t_fallbackId = 0;
};

} // namespace mc::entity
