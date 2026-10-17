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

#include "AgeableWaterCreatureEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"

namespace mc {

namespace {
// NBT 键：agrowing_age 对齐 AgeableEntity 使用的实体 NBT 键。
constexpr const char* kNbtKeyAge = "Age";
} // namespace

// ==================== 同步数据参数静态成员初始化 ====================
// id 由 registerData 沿继承链分配为 16（WaterMobEntity 无自有字段）。
entity::DataParameter<bool> AgeableWaterCreatureEntity::DATA_BABY_PARAM = entity::EntityDataManager::createKey<bool>();

// 继承链标识（parent = WaterMobEntity::classInfo()）。
// vanilla 1.21.11 AgeableWaterCreature 经 AgeableMob，id16=DATA_BABY(Boolean)。
const entity::EntityClassInfo& AgeableWaterCreatureEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"AgeableWaterCreatureEntity", &WaterMobEntity::classInfo()};
    return s_classInfo;
}

AgeableWaterCreatureEntity::AgeableWaterCreatureEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : WaterMobEntity(id, registry)
{
    // 显式调用 registerData() 注册 DATA_BABY（C++ 基类构造期虚函数不派发，
    // Entity::Entity() 内部调用的 registerData() 解析到 MosEntity 而非本类）。
    registerData();
}

void AgeableWaterCreatureEntity::registerData()
{
    // 先串联父类链（WaterMobEntity→CreatureEntity→…→MobEntity），确保 Mob(id15) 已注册。
    WaterMobEntity::registerData();

    // 标记当前正在注册本类的字段，使 registerParam 沿继承链分配 id
    // （WaterMobEntity 无字段，故 DATA_BABY 落 id16，对齐 vanilla AgeableMob.DATA_BABY）。RAII 配对压弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册 vanilla 1.21.11 AgeableMob.DATA_BABY(Boolean,id16)。缺此字段会使子类
    // （Dolphin.GOT_FISH/MOISTNESS、GlowSquid.DATA_DARK_TICKS）id 前移，撞客户端字段类型校验崩溃。
    m_dataManager.registerParam(DATA_BABY_PARAM, false);
}

void AgeableWaterCreatureEntity::setGrowingAge(i32 age)
{
    const bool wasChild = isChild();
    m_growingAge = age;
    if (wasChild != isChild()) {
        refreshDimensions();
        // 同步幼体状态到数据管理器（vanilla DATA_BABY）
        m_dataManager.set(DATA_BABY_PARAM, isChild());
    }
}

void AgeableWaterCreatureEntity::setChild(bool child)
{
    setGrowingAge(child ? BABY_AGE : MAX_AGE);
}

void AgeableWaterCreatureEntity::ageUp(i32 seconds)
{
    i32 ticks = seconds * 20; // 秒转换为 tick
    const bool wasChild = isChild();
    m_growingAge += ticks;

    if (m_growingAge >= MAX_AGE) {
        m_growingAge = MAX_AGE;
        onGrowUp();
    }

    if (wasChild != isChild()) {
        refreshDimensions();
        m_dataManager.set(DATA_BABY_PARAM, isChild());
    }
}

void AgeableWaterCreatureEntity::addGrowingAge(i32 amount)
{
    const bool wasChild = isChild();
    m_growingAge += amount;

    if (m_growingAge >= MAX_AGE && amount > 0) {
        m_growingAge = MAX_AGE;
        onGrowUp();
    }

    if (wasChild != isChild()) {
        refreshDimensions();
        m_dataManager.set(DATA_BABY_PARAM, isChild());
    }
}

void AgeableWaterCreatureEntity::tick()
{
    WaterMobEntity::tick();
    updateAge();
}

void AgeableWaterCreatureEntity::updateAge()
{
    const bool wasChild = isChild();

    if (isChild()) {
        // 幼体成长：每 tick +1（乘成长速度倍率，本类暂不支持强制成长，恒 1.0）
        m_growingAge += 1;

        if (m_growingAge >= MAX_AGE) {
            m_growingAge = MAX_AGE;
            onGrowUp();
        }
    } else {
        // 成体繁殖冷却（负数年龄转正后递减）
        if (m_growingAge > 0) {
            --m_growingAge;
        }
    }

    if (wasChild != isChild()) {
        refreshDimensions();
        m_dataManager.set(DATA_BABY_PARAM, isChild());
    }
}

void AgeableWaterCreatureEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    WaterMobEntity::addAdditionalSaveData(tag);
    // Age (i32)：仅幼体（负值）时保存，对齐 AgeableEntity 的写法。
    if (m_growingAge != 0) {
        tag.put(kNbtKeyAge, m_growingAge);
    }
}

Result<void> AgeableWaterCreatureEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace entity::serialization;
    MC_TRY(WaterMobEntity::readAdditionalSaveData(tag));
    if (auto val = nbt_helper::tryGetInt(tag, kNbtKeyAge)) {
        setGrowingAge(*val);
    }
    return Result<void>::ok();
}

} // namespace mc