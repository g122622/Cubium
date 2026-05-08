#pragma once

#include "DataParameter.hpp"
#include "../../core/Types.hpp"
#include "../../core/Result.hpp"
#include <unordered_map>
#include <variant>
#include <vector>
#include <mutex>
#include <any>

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::u16;
using mc::i8;
using mc::i32;
using mc::i64;
using mc::f32;
using mc::std::string;
using mc::Vector3i;
using mc::Vector2f;
using mc::Vector3f;

/**
 * @brief 数据参数值包装
 *
 * 用于存储任意类型的数据参数值
 */
class DataValue {
public:
    // 支持的数据类型
    using ValueType = std::variant<
        i8,
        i32,
        i64,
        f32,
        std::string,
        bool,
        Vector3i,
        Vector2f,
        Vector3f
    >;

    DataValue() = default;

    template<typename T>
    explicit DataValue(T value) : m_value(value) {}

    template<typename T>
    [[nodiscard]] T get() const {
        return std::get<T>(m_value);
    }

    template<typename T>
    void set(T value) {
        m_value = value;
    }

    [[nodiscard]] const ValueType& value() const { return m_value; }
    [[nodiscard]] size_t index() const { return m_value.index(); }

    bool operator==(const DataValue& other) const {
        return m_value == other.m_value;
    }

    bool operator!=(const DataValue& other) const {
        return m_value != other.m_value;
    }

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
 *
 * 参考 MC 1.16.5 EntityDataManager (DataTracker)
 */
class EntityDataManager {
public:
    /**
     * @brief 创建数据参数键
     * @return 新的数据参数键
     *
     * 线程安全，ID自动递增
     */
    template<typename T>
    static DataParameter<T> createKey() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return DataParameter<T>(s_nextId++);
    }

    EntityDataManager() = default;

    /**
     * @brief 注册数据参数
     * @param param 参数键
     * @param defaultValue 默认值
     *
     * 必须在使用前注册所有参数
     */
    template<typename T>
    void registerParam(DataParameter<T> param, T defaultValue) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries[param.id()] = DataEntry{DataValue(defaultValue), false};
    }

    /**
     * @brief 设置参数值
     * @param param 参数键
     * @param value 新值
     *
     * 如果值发生变化，标记为脏数据
     */
    template<typename T>
    void set(DataParameter<T> param, T value) {
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
    bool setRaw(u16 id, const DataValue& value) {
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
    template<typename T>
    [[nodiscard]] T get(DataParameter<T> param) const {
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
    [[nodiscard]] const DataValue* getRaw(u16 id) const {
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
    [[nodiscard]] bool hasParam(u16 id) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_entries.find(id) != m_entries.end();
    }

    /**
     * @brief 获取所有脏数据条目
     * @return 脏数据ID列表
     */
    [[nodiscard]] std::vector<u16> getDirtyParams() const {
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
     * @param id 参数ID，如果为空则清除所有
     */
    void clearDirty(u16 id = 0xFFFF) {
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
    [[nodiscard]] bool hasDirtyData() const {
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
    [[nodiscard]] const std::unordered_map<u16, DataEntry>& getAllEntries() const {
        return m_entries;
    }

    /**
     * @brief 从其他管理器复制数据
     * @param other 源管理器
     */
    void copyFrom(const EntityDataManager& other) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::lock_guard<std::mutex> otherLock(other.m_mutex);
        m_entries = other.m_entries;
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<u16, DataEntry> m_entries;

    static inline u16 s_nextId = 0;
    static inline std::mutex s_mutex;
};

} // namespace mc::entity
