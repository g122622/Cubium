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

#include <cstddef>
#include <functional>
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

    [[nodiscard]] const std::string& getKey() const noexcept { return m_key; }
    [[nodiscard]] size_t getHash() const noexcept { return m_hash; }

    bool operator==(const Activity& other) const noexcept { return m_key == other.m_key; }
    bool operator!=(const Activity& other) const noexcept { return m_key != other.m_key; }
    bool operator<(const Activity& other) const noexcept { return m_key < other.m_key; }

private:
    std::string m_key;
    size_t m_hash;

    static Activity _registerActivity(const std::string& key);
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
    size_t operator()(const mc::entity::ai::brain::schedule::Activity& activity) const noexcept
    {
        return activity.getHash();
    }
};
} // namespace std
