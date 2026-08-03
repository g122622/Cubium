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

#include "TameableEntity.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include <optional>
#include <vector>

namespace mc {

// ==================== 静态成员初始化 ====================
entity::DataParameter<i8> TameableEntity::DATA_FLAGS_PARAM = entity::EntityDataManager::createKey<i8>();
entity::DataParameter<entity::OptionalUuidValue> TameableEntity::DATA_OWNERUUID_PARAM =
    entity::EntityDataManager::createKey<entity::OptionalUuidValue>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = AnimalEntity::classInfo()）
// ============================================================================
const entity::EntityClassInfo& TameableEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"TameableEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

TameableEntity::TameableEntity(EntityInstanceId id)
    : AnimalEntity(id)
{
    // 注册属性
    registerAttributes();

    // 显式调用 registerData() 注册同步数据参数
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类（Entity::Entity 内部调用
    // registerData() 时调用的是 Entity::registerData 而非 TameableEntity::registerData），
    // 必须在派生类构造函数中显式调用，参考 WolfEntity / CatEntity 模式。
    registerData();
}

void TameableEntity::setTamed(bool tamed)
{
    if (isTamed() != tamed) {
        // 写 DATA_FLAGS_PARAM 的 bit2（对齐 vanilla TamableAnimal.DATA_FLAGS_ID & 4 = tame）
        const i8 flags = m_dataManager.get<i8>(DATA_FLAGS_PARAM);
        m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(tamed ? (flags | 0x04) : (flags & ~0x04)));
        onTamed(tamed);
    }
}

void TameableEntity::setOwnerId(Uuid ownerId)
{
    m_ownerId = ownerId;
    // 同步到 DATA_OWNERUUID_PARAM（对齐 vanilla TamableAnimal.DATA_OWNERUUID_ID）
    m_dataManager.set(DATA_OWNERUUID_PARAM, entity::OptionalUuidValue{true, ownerId});
}

void TameableEntity::clearOwner()
{
    m_ownerId = std::nullopt;
    m_dataManager.set(DATA_OWNERUUID_PARAM, entity::OptionalUuidValue{false, {}});
}

void TameableEntity::setSitting(bool sitting)
{
    if (m_sitting != sitting) {
        m_sitting = sitting;
        // 同步 sitting 到 DATA_FLAGS_PARAM 的 bit0（对齐 vanilla TamableAnimal.DATA_FLAGS_ID & 1），
        // 经元数据广播到客户端。m_sitting 仍是业务权威源（NBT 存它），flags bit0 为同步镜像。
        const i8 flags = m_dataManager.get<i8>(DATA_FLAGS_PARAM);
        m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(sitting ? (flags | 0x01) : (flags & ~0x01)));
        // 坐下时停止移动
        if (sitting) {
            clearNavigation();
        }
    }
}

void TameableEntity::setAttackTarget(LivingEntity* target)
{
    MobEntity::setAttackTarget(target);
    if (target != nullptr) {
        setAngerTime(MAX_ANGER_TIME);
    }
}

void TameableEntity::setRevengeTarget(LivingEntity* target)
{
    if (target != nullptr) {
        m_revengeTargetId = target->id();
        m_revengeTimer = MAX_ANGER_TIME;
        setAngerTime(MAX_ANGER_TIME);
    } else {
        m_revengeTargetId = std::nullopt;
        m_revengeTimer = 0;
    }
}

LivingEntity* TameableEntity::getRevengeTarget() const
{
    if (!m_revengeTargetId.has_value()) {
        return nullptr;
    }
    // 从世界获取复仇目标
    IWorld* worldPtr = const_cast<IWorld*>(this->world());
    if (!worldPtr) {
        return nullptr;
    }
    Entity* entity = worldPtr->getEntity(m_revengeTargetId.value());
    if (!entity || !entity->isAlive()) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(entity);
}

void TameableEntity::setAngry(bool angry)
{
    if (angry) {
        setAngerTime(MAX_ANGER_TIME);
    } else {
        setAngerTime(0);
        setAttackTarget(nullptr);
        m_revengeTargetId = std::nullopt;
    }
}

void TameableEntity::tick()
{
    AnimalEntity::tick();
    updateAnger();
}

void TameableEntity::updateAnger()
{
    // 通过虚函数 getAngerTime/setAngerTime 访问愤怒时间，
    // 允许子类（如 WolfEntity）将愤怒状态存储到 DataParameter 并自动同步到客户端。
    if (getAngerTime() > 0) {
        setAngerTime(getAngerTime() - 1);
        if (getAngerTime() <= 0) {
            // 愤怒时间结束，清除攻击目标
            setAttackTarget(nullptr);
            m_revengeTargetId = std::nullopt;
        }
    }
}

void TameableEntity::registerGoals()
{
    // 基础目标由子类添加
    // 子类应该调用此方法然后添加自己的目标
    AnimalEntity::registerGoals();
}

void TameableEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 驯服动物的基础属性（子类可以覆盖）
    // 大多数驯服动物的属性由子类设置
}

void TameableEntity::registerData()
{
    // 调用父类方法，确保基类数据参数已注册
    AnimalEntity::registerData();

    // 标记当前正在注册 TameableEntity 类的字段，使 registerParam 沿 TameableEntity 继承链
    // 分配 id（续接 AnimalEntity 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册同步数据参数（对齐 vanilla TamableAnimal.defineSynchedData()）：
    //   id17: DATA_FLAGS_PARAM（Byte，bit2=tame / bit0=sitting）
    //   id18: DATA_OWNERUUID_PARAM（OptionalLivingEntityRef，主人 UUID）
    // vanilla DATA_FLAGS_ID define 默认 (byte)0；DATA_OWNERUUID_ID 默认空引用。
    m_dataManager.registerParam(DATA_FLAGS_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(DATA_OWNERUUID_PARAM, entity::OptionalUuidValue{false, {}});
}

Player* TameableEntity::getOwner() const
{
    // 通过主人ID在世界中查找玩家实体
    if (!m_ownerId.has_value()) {
        return nullptr;
    }

    IWorld* worldPtr = const_cast<IWorld*>(this->world());
    if (!worldPtr) {
        return nullptr;
    }

    // 获取所有玩家，查找匹配的主人
    // 对齐 vanilla TamableAnimal.getOwner()：按主人 UUID（profile UUID）查找，而非非持久 playerId。
    // Player::uuidBytes() 返回回填后的 profile UUID（UUid），由 MinecraftServer::createPlayerForConnection 写入。
    std::vector<Entity*> players = worldPtr->getPlayers();
    for (Entity* entity : players) {
        if (entity == nullptr) {
            continue;
        }
        Player* player = dynamic_cast<Player*>(entity);
        if (player != nullptr && player->uuidBytes() == m_ownerId.value()) {
            return player;
        }
    }

    return nullptr;
}

scoreboard::Team* TameableEntity::getTeam()
{
    // 已驯服的动物继承主人的队伍
    if (isTamed() && m_ownerId.has_value()) {
        Player* owner = getOwner();
        if (owner != nullptr) {
            return owner->getTeam();
        }
    }
    // 未驯服的动物使用默认队伍逻辑
    return AnimalEntity::getTeam();
}

const scoreboard::Team* TameableEntity::getTeam() const
{
    // 已驯服的动物继承主人的队伍
    if (isTamed() && m_ownerId.has_value()) {
        Player* owner = getOwner();
        if (owner != nullptr) {
            return owner->getTeam();
        }
    }
    // 未驯服的动物使用默认队伍逻辑
    return AnimalEntity::getTeam();
}

bool TameableEntity::wantsToAttack(const LivingEntity& target, const LivingEntity* owner) const
{
    MC_UNUSED(owner);
    MC_UNUSED(target);
    // 默认实现：允许攻击所有目标
    // 子类可重写此方法来限制攻击目标，例如狼不会攻击苦力怕和恶魂
    return true;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void TameableEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    AnimalEntity::addAdditionalSaveData(tag);

    // Sitting (byte/bool) - 是否坐下
    tag.put(nbt_keys::SITTING, static_cast<i8>(m_sitting ? 1 : 0));

    // Owner (string) - 主人 UUID（对齐 vanilla TamableAnimal NBT key "Owner"）
    // 值用 32 字符纯十六进制 UUID 字符串（项目自洽存档，非 vanilla int[4]，不与 vanilla 存档互通）。
    if (m_ownerId.has_value()) {
        tag.put(nbt_keys::OWNER, util::uuidToString(m_ownerId.value()));
    }

    // AngerTime (i32) - 愤怒剩余时间
    // 通过虚函数 getAngerTime() 读取，允许子类（如 WolfEntity）将愤怒状态存储到 DataParameter。
    if (getAngerTime() > 0) {
        tag.put(nbt_keys::ANGER_TIME, getAngerTime());
    }
}

Result<void> TameableEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    MC_TRY(AnimalEntity::readAdditionalSaveData(tag));

    // Sitting (byte/bool)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::SITTING)) {
        m_sitting = *val;
    }

    // Owner (string) - 主人 UUID（vanilla key "Owner"）
    // 兼容旧存档 key "OwnerUUID"（u64 十进制串）——直接弃旧，因 u64 playerId 非持久无法迁移到 UUID。
    if (auto val = nbt_helper::tryGetString(tag, nbt_keys::OWNER)) {
        m_ownerId = util::uuidFromString(*val);
    }

    // AngerTime (i32)
    // 通过虚函数 setAngerTime() 写入，允许子类（如 WolfEntity）将愤怒状态恢复到 DataParameter。
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::ANGER_TIME)) {
        setAngerTime(*val);
    }

    // 从存档恢复驯服状态：如果有主人则自动设为已驯服
    if (m_ownerId.has_value()) {
        setTamed(true);
        // 主人恢复后同步 owner 到 DataParameter（addAdditionalSaveData 只持久化 m_ownerId，
        // 重载后 DataParameter 需重新填充以广播到客户端）
        m_dataManager.set(DATA_OWNERUUID_PARAM, entity::OptionalUuidValue{true, m_ownerId.value()});
    }

    // 同步 sitting 到 DATA_FLAGS bit0（存档只持久化 m_sitting，需写回 flags 以广播）
    {
        const i8 flags = m_dataManager.get<i8>(DATA_FLAGS_PARAM);
        m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(m_sitting ? (flags | 0x01) : (flags & ~0x01)));
    }

    return Result<void>::ok();
}

} // namespace mc
