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

#include "AbstractRaiderEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/illager/PatrollerEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/village/raid/Raid.hpp"
#include "common/world/village/raid/RaiderType.hpp"

namespace mc {

// ==================== 同步数据参数静态成员初始化 ====================
// 对应 vanilla 1.21.11 Raider.IS_CELEBRATING，id 由 registerData 沿继承链分配为 16。
entity::DataParameter<bool> AbstractRaiderEntity::IS_CELEBRATING_PARAM = entity::EntityDataManager::createKey<bool>();

// ============================================================================
// 继承链标识（parent = PatrollerEntity::classInfo()）。
// vanilla 1.21.11 Raider 在 Mob(id15) 之后注册 IS_CELEBRATING(Boolean,id16)，本类补齐。
// ============================================================================
const entity::EntityClassInfo& AbstractRaiderEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"AbstractRaiderEntity", &PatrollerEntity::classInfo()};
    return s_classInfo;
}

void AbstractRaiderEntity::registerData()
{
    // 先调用父类方法。PatrollerEntity 无 registerData override，显式指 MobEntity::registerData()
    // 避免名字查找落空，确保 Mob(id15) 及以下基类参数已注册。
    MobEntity::registerData();

    // 标记当前正在注册 AbstractRaiderEntity 类的字段，使 registerParam 沿继承链分配 id
    // （续接 Mob id15 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册 vanilla 1.21.11 Raider.IS_CELEBRATING(Boolean,id16)：
    // 庆祝状态同步镜像，业务权威源仍为 m_state。
    m_dataManager.registerParam(IS_CELEBRATING_PARAM, false);
}

AbstractRaiderEntity::AbstractRaiderEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : PatrollerEntity(id, registry)
{
    // 显式调用 registerData() 注册 IS_CELEBRATING（C++ 基类构造期虚函数不派发，
    // Entity::Entity() 内部调用的 registerData() 解析到 MobEntity 而非本类）。
    registerData();
}

void AbstractRaiderEntity::joinRaid(world::village::raid::Raid* raid, i32 wave)
{
    m_raid = raid;
    m_wave = wave;
    m_canJoinRaid = false;
}

void AbstractRaiderEntity::leaveRaid()
{
    m_raid = nullptr;
    m_wave = 0;
    m_canJoinRaid = true;
}

void AbstractRaiderEntity::startCelebrating()
{
    m_celebrationTime = CELEBRATION_DURATION;
    setState(RaiderState::Celebrating);
}

void AbstractRaiderEntity::tick()
{
    PatrollerEntity::tick();

    // 更新庆祝时间
    if (m_celebrationTime > 0) {
        m_celebrationTime--;
        if (m_celebrationTime <= 0) {
            setState(RaiderState::Neutral);
        }
    }

    // 检查袭击是否仍然有效
    if (m_raid != nullptr) {
        // 如果袭击已结束，离开袭击
        if (m_raid->status() != world::village::raid::RaidStatus::Ongoing) {
            leaveRaid();
        }
    }
}

void AbstractRaiderEntity::die(DamageSource& cause)
{
    // 通知袭击掠夺者死亡
    if (m_raid != nullptr && m_world != nullptr) {
        m_raid->onRaiderDeath(id(), *m_world);
        leaveRaid();
    }

    // 调用父类die方法
    PatrollerEntity::die(cause);
}

} // namespace mc
