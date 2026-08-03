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

#include "Activity.hpp"
#include <functional>
#include <string>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace schedule {

// 静态成员初始化
Activity Activity::CORE = _registerActivity("core");
Activity Activity::IDLE = _registerActivity("idle");
Activity Activity::WORK = _registerActivity("work");
Activity Activity::PLAY = _registerActivity("play");
Activity Activity::REST = _registerActivity("rest");
Activity Activity::MEET = _registerActivity("meet");
Activity Activity::PANIC = _registerActivity("panic");
Activity Activity::RAID = _registerActivity("raid");
Activity Activity::PRE_RAID = _registerActivity("pre_raid");
Activity Activity::HIDE = _registerActivity("hide");
Activity Activity::FIGHT = _registerActivity("fight");
Activity Activity::CELEBRATE = _registerActivity("celebrate");
Activity Activity::ADMIRE_ITEM = _registerActivity("admire_item");
Activity Activity::AVOID = _registerActivity("avoid");
Activity Activity::RIDE = _registerActivity("ride");

Activity::Activity(const std::string& key)
    : m_key(key)
    , m_hash(std::hash<std::string>{}(key))
{}

Activity Activity::_registerActivity(const std::string& key)
{
    return Activity(key);
}

} // namespace schedule
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
