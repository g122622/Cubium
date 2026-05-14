#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace schedule {

/**
 * @brief 活动类型
 *
 * 定义实体的不同AI活动状态
 * 参考 MC 1.16.5 Activity
 */
class Activity {
public:
    // 预定义活动类型
    static Activity CORE;
    static Activity IDLE;
    static Activity WORK;
    static Activity PLAY;
    static Activity REST;
    static Activity MEET;
    static Activity PANIC;
    static Activity RAID;
    static Activity PRE_RAID;
    static Activity HIDE;
    static Activity FIGHT;
    static Activity CELEBRATE;
    static Activity ADMIRE_ITEM;
    static Activity AVOID;
    static Activity RIDE;

    explicit Activity(const std::string& key);

    [[nodiscard]] const std::string& getKey() const { return m_key; }
    [[nodiscard]] size_t getHash() const { return m_hash; }

    bool operator==(const Activity& other) const { return m_key == other.m_key; }

    bool operator!=(const Activity& other) const { return m_key != other.m_key; }

    bool operator<(const Activity& other) const { return m_key < other.m_key; }

private:
    std::string m_key;
    size_t m_hash;

    static Activity registerActivity(const std::string& key);
};

} // namespace schedule
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc

// 哈希函数特化
namespace std {
template <>
struct hash<mc::entity::ai::brain::schedule::Activity> {
    size_t operator()(const mc::entity::ai::brain::schedule::Activity& activity) const { return activity.getHash(); }
};
} // namespace std
