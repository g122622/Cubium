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
