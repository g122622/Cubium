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

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace schedule {

// 静态成员初始化
Activity Activity::CORE = registerActivity("core");
Activity Activity::IDLE = registerActivity("idle");
Activity Activity::WORK = registerActivity("work");
Activity Activity::PLAY = registerActivity("play");
Activity Activity::REST = registerActivity("rest");
Activity Activity::MEET = registerActivity("meet");
Activity Activity::PANIC = registerActivity("panic");
Activity Activity::RAID = registerActivity("raid");
Activity Activity::PRE_RAID = registerActivity("pre_raid");
Activity Activity::HIDE = registerActivity("hide");
Activity Activity::FIGHT = registerActivity("fight");
Activity Activity::CELEBRATE = registerActivity("celebrate");
Activity Activity::ADMIRE_ITEM = registerActivity("admire_item");
Activity Activity::AVOID = registerActivity("avoid");
Activity Activity::RIDE = registerActivity("ride");

Activity::Activity(const std::string& key)
    : m_key(key)
    , m_hash(std::hash<std::string>{}(key))
{}

Activity Activity::registerActivity(const std::string& key)
{
    return Activity(key);
}

} // namespace schedule
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
