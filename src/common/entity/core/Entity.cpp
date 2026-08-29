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

#include "Entity.hpp"
#include "../../physics/PhysicsConstants.hpp"
#include "../../physics/PhysicsEngine.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../sound/SoundEvents.hpp"
#include "../../util/UuidUtils.hpp"
#include "../../util/assert/AssertMacros.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../util/math/ray/Raycast.hpp"
#include "../../util/text/StringTextComponent.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/WorldConstants.hpp"
#include "../../world/block/Block.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../../world/block/BlockSoundType.hpp"
#include "../../world/block/BlockTags.hpp"
#include "../../world/entity/JavaEntityTypeIdMap.hpp"
#include "../../world/fluid/Fluid.hpp"
#include "../damage/DamageSource.hpp"
#include "../entities/effect/EffectEntities.hpp"
#include "../entities/player/Player.hpp"
#include "../entities/projectile/ProjectileDeflection.hpp"
#include "../entities/projectile/ProjectileEntity.hpp"
#include "../serialization/EntityNbtKeys.hpp"
#include "../serialization/NbtHelper.hpp"
#include "../serialization/components/ComponentSerializerRegistry.hpp"
#include "../tag/EntityTypeTags.hpp"
#include "EntityRegistry.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/MoverType.hpp"
#include "common/entity/ecs/components/EntityFlagsComponent.hpp"
#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/EntityStateComponent.hpp"
#include "common/entity/ecs/components/FireComponent.hpp"
#include "common/entity/ecs/components/FreezeComponent.hpp"
#include "common/entity/ecs/components/PhysicsStateComponent.hpp"
#include "common/entity/ecs/components/PortalComponent.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/mod/bedrock/addon/binding/ScriptHandleRegistry.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/ray/Ray.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// 静态数据参数定义（通过 createKey 自动分配唯一 ID，避免跨类 ID 冲突）
// ============================================================================

entity::DataParameter<i8> Entity::DATA_FLAGS_PARAM = entity::EntityDataManager::createKey<i8>();
entity::DataParameter<i32> Entity::DATA_AIR_PARAM = entity::EntityDataManager::createKey<i32>();
entity::DataParameter<bool> Entity::DATA_CUSTOM_NAME_VISIBLE_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<entity::OptionalComponentValue> Entity::DATA_CUSTOM_NAME_PARAM =
    entity::EntityDataManager::createKey<entity::OptionalComponentValue>();
entity::DataParameter<bool> Entity::DATA_SILENT_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<bool> Entity::DATA_NO_GRAVITY_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<entity::PoseValue> Entity::DATA_POSE_PARAM =
    entity::EntityDataManager::createKey<entity::PoseValue>();
entity::DataParameter<i32> Entity::DATA_TICKS_FROZEN_PARAM = entity::EntityDataManager::createKey<i32>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry）
// ============================================================================
// Entity 是继承链根，parent 为 nullptr。子类 classInfo 的 parent 指向其直接父类
// 的 classInfo()，运行时解引用构建继承链。
const entity::EntityClassInfo& Entity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"Entity", nullptr};
    return s_classInfo;
}

// ============================================================================
// Entity 实现
// ============================================================================

Entity::Entity(EntityInstanceId id, IWorld* world, ecs::EntityRegistry& registry)
    : m_memTrack(this)
    , m_id(id)
    , m_random(
          static_cast<u64>(id) ^ static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()))
    , m_world(world)
{
    // 在传入 registry 内 create 出 ECS 实体，并 attach 4 个高频组件，缓存裸指针到
    // m_builtIn。此后 position()/velocity() 等 getter 直接解引用 m_builtIn，零双写。
    // 对齐基岩版 Actor(ILevel&, EntityContext&) 构造签名透传 + _addActorBuiltInComponents
    // attach 基础组件的设计（基岩版 attach 在 initializeComponents 阶段，首批4组件简单，
    // 此处构造时一次 attach）。
    const ecs::EntityId ecsEntity = registry.create();
    m_builtIn.stateVector = &registry.raw().emplace<ecs::StateVectorComponent>(ecsEntity);
    m_builtIn.velocity = &registry.raw().emplace<ecs::VelocityComponent>(ecsEntity);
    m_builtIn.aabbShape = &registry.raw().emplace<ecs::AABBShapeComponent>(ecsEntity);
    m_builtIn.rotation = &registry.raw().emplace<ecs::EntityRotationComponent>(ecsEntity);
    // 第二批：attach Portal/Fire/PhysicsState/Freeze 四组件。Portal/Fire/Freeze 不进
    // m_builtIn 缓存（低频，走 tryGetComponent）；PhysicsState 高频（move/checkOnGround/
    // updateFallDistance 及各 tick 40+ 处直接访问），破例进 m_builtIn 缓存裸指针。
    // HurtStateComponent 由 LivingEntity 构造时 attach。
    registry.raw().emplace<ecs::PortalComponent>(ecsEntity);
    registry.raw().emplace<ecs::FireComponent>(ecsEntity);
    m_builtIn.physicsState = &registry.raw().emplace<ecs::PhysicsStateComponent>(ecsEntity);
    registry.raw().emplace<ecs::FreezeComponent>(ecsEntity);
    // 第四批：attach EntityFlags/EntityState 两组件，承载 flags/air/customName/
    // customNameVisible/silent/noGravity/pose 七个 C 类同步字段（真相源），
    // 对应 DataParameter 退为同步镜像。低频走 tryGetComponent 查询。
    registry.raw().emplace<ecs::EntityFlagsComponent>(ecsEntity);
    registry.raw().emplace<ecs::EntityStateComponent>(ecsEntity);
    // 反向桥接组件：ECS→OOP。FireTickSystem/PortalTickSystem 等 System 经
    // view<..., EntityOwnerComponent> 遍历实体时，由此反查 OOP 句柄调虚函数
    // （isInWater/hurt/canTeleport 等）。非拥有裸指针——Entity 所有权归
    // EntityManager::m_entities 或测试局部变量，本组件仅反查。Entity 析构时
    // destroy entt 实体（含本组件），故指针不会悬垂。详见 EntityOwnerComponent.hpp。
    registry.raw().emplace<ecs::EntityOwnerComponent>(ecsEntity, this);
    m_entityContext = std::make_unique<ecs::EntityContext>(registry, ecsEntity);

    // 生成随机 UUID（使用实体的持久化随机数生成器）。
    // 必须用 util::generateRandomUuid + util::uuidToString 生成固定 32 字符的十六进制
    // 字符串——不能用 `ss << std::hex << u64 << u64`，那种写法不补零，会得到 <32 字符
    // 的串，导致 util::uuidFromString（要求恰好 32 字符）解析失败返回全零 UUID。
    const Uuid uuidBytes = util::generateRandomUuid(m_random);
    m_uuid = util::uuidToString(uuidBytes);

    // 注册数据参数
    registerData();
}

Entity::~Entity()
{
    // 销毁 ECS 实体及其全部组件（含 EntityOwnerComponent，消除悬垂反查指针）。
    // m_entityContext 此刻仍存活（成员在 ~Entity body 之后析构），可安全取 registry 与 entity id。
    // valid() 校验防御：极少数路径（如 move-into-graveyard 后实体被显式 destroy）下
    // entt 实体可能已失效，此时跳过避免重复 destroy 断言。
    if (m_entityContext != nullptr && m_entityContext->valid()) {
        m_entityContext->registry().destroy(m_entityContext->entity());
    }

    // 路径A兜底：实体析构（remove()/discard → graveyard 延迟析构，或其它直接析构路径）。
    // invalidateAll 把所有指向本实体的 JS 句柄 ObjectData::ptr 置 nullptr，防 owned=false 裸 Entity*
    // 句柄悬垂 UAF（见 ScriptHandleRegistry.hpp 问题背景）。与 EntityManager::removeEntity 的
    // invalidateAll（路径B立即 free）形成双保险：路径A经 graveyard 延迟，可能先于析构就被 removeEntity
    // 处理过（此时 invalidate 已幂等 no-op），也可能不经 removeEntity 直接析构（此处兜底）。
    // 重复调用安全：已 invalidate 的 id 在注册表中已 erase，再次调为 no-op。
    mc::mod::bedrock::addon::ScriptHandleRegistry::instance().invalidateAll(m_id);
}

void Entity::registerData()
{
    // 标记当前正在注册 Entity 类的字段，使 registerParam 沿 Entity 继承链分配 id。
    // RAII：构造压栈，析构弹栈。基类 registerData 先执行，子类 registerData 再压入子类 classInfo。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册基础数据参数。id 由继承链分配器按此调用顺序连续分配 id 0..7，
    // 故调用顺序必须严格等于 vanilla 1.21.11 Entity 各 EntityDataAccessor 的
    // defineId declare 顺序（ClassTreeIdRegistry.define 在 defineId 静态初始化时
    // 按 declare 顺序分配 id，而非 define() 默认值设置顺序）。
    //   id0 FLAGS(Byte, Entity.java:242) / id1 AIR(Int, :250) /
    //   id2 CUSTOM_NAME(Optional<Component>, :251) / id3 CUSTOM_NAME_VISIBLE(Boolean, :254) /
    //   id4 SILENT(Boolean, :255) / id5 NO_GRAVITY(Boolean, :256) /
    //   id6 POSE(Pose, :257) / id7 TICKS_FROZEN(Int, :258)
    // 注意：CUSTOM_NAME(id2,Optional) 必须在 CUSTOM_NAME_VISIBLE(id3,Boolean) 之前注册，
    // 顺序反则 wire index 2 发 Boolean 而真客户端按 Optional 校验，set_entity_data 崩。
    m_dataManager.registerParam(DATA_FLAGS_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(DATA_AIR_PARAM, maxAir());
    m_dataManager.registerParam(DATA_CUSTOM_NAME_PARAM, entity::OptionalComponentValue{false, std::string{}});
    m_dataManager.registerParam(DATA_CUSTOM_NAME_VISIBLE_PARAM, false);
    m_dataManager.registerParam(DATA_SILENT_PARAM, false);
    m_dataManager.registerParam(DATA_NO_GRAVITY_PARAM, false);
    m_dataManager.registerParam(DATA_POSE_PARAM, entity::PoseValue{EntityPose::Standing});
    m_dataManager.registerParam(DATA_TICKS_FROZEN_PARAM, static_cast<i32>(0));
}

entity::EntitySize Entity::getDimensions(EntityPose pose) const
{
    (void)pose;
    return entity::EntitySize(width(), height(), eyeHeight(), false);
}

void Entity::refreshDimensions()
{
    m_dimensions = getDimensions(pose());
    m_dimensionsInitialized = true;
    reapplyPosition();
}

void Entity::reapplyPosition()
{
    if (!m_dimensionsInitialized) {
        m_dimensions = getDimensions(pose());
        m_dimensionsInitialized = true;
    }

    m_builtIn.aabbShape->m_aabb = m_dimensions.makeBoundingBox(
        m_builtIn.stateVector->m_pos.x, m_builtIn.stateVector->m_pos.y, m_builtIn.stateVector->m_pos.z);

    // 经 EntityManager 反向指针通知空间索引：实体位置变更可能跨 section，需迁移。
    // addEntity 前调 setPosition 时 m_entityManager 仍为 nullptr，跳过——addEntity 时
    // 按当前位置一次性登记索引，正确。addEntity 后调 setPosition 则触发迁移，正确。
    if (m_entityManager != nullptr) {
        m_entityManager->_onEntityPositionChanged(*this);
    }
}

void Entity::setPose(EntityPose poseIn)
{
    if (pose() == poseIn) {
        return;
    }

    auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    c->m_pose = poseIn;
    m_dataManager.set(DATA_POSE_PARAM, entity::PoseValue{poseIn});
    refreshDimensions();
}

void Entity::setFlags(EntityFlags flags)
{
    auto* c = m_entityContext->tryGetComponent<ecs::EntityFlagsComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    c->m_flags = flags;
    m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(static_cast<u8>(flags)));
}

void Entity::addFlag(EntityFlags flag)
{
    auto* c = m_entityContext->tryGetComponent<ecs::EntityFlagsComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    c->m_flags = c->m_flags | flag;
    m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(static_cast<u8>(c->m_flags)));
}

void Entity::removeFlag(EntityFlags flag)
{
    auto* c = m_entityContext->tryGetComponent<ecs::EntityFlagsComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    c->m_flags = static_cast<EntityFlags>(static_cast<u8>(c->m_flags) & ~static_cast<u8>(flag));
    m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(static_cast<u8>(c->m_flags)));
}

bool Entity::isGlowing() const
{
    // 客户端检查数据参数中的 Glowing 标志位
    // 服务端检查 m_glowing 字段
    if (m_world != nullptr && m_world->isClientSide()) {
        return hasFlag(EntityFlags::Glowing);
    }
    return m_glowing;
}

void Entity::setGlowing(bool glowing)
{
    // 在服务端设置字段并同步标志位
    m_glowing = glowing;
    if (m_world != nullptr && !m_world->isClientSide()) {
        if (glowing) {
            addFlag(EntityFlags::Glowing);
        } else {
            removeFlag(EntityFlags::Glowing);
        }
    }
}

bool Entity::isVisuallySwimming() const
{
    // 基类实现：仅依赖 Swimming 姿态判定
    // LivingEntity 重写时会额外考虑 FallFlying 姿态
    return pose() == EntityPose::Swimming;
}

void Entity::setAir(i32 air)
{
    auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    c->m_air = air;
    m_dataManager.set(DATA_AIR_PARAM, c->m_air);
}

void Entity::setCustomName(const std::string& name)
{
    // 委托 setCustomNameComponent（virtual），确保所有命名路径统一经由虚入口，
    // 派生类（如 Vindicator 的 Johnny 触发）的 override 对文本与组件两种调用均生效。
    if (name.empty()) {
        setCustomNameComponent(nullptr);
    } else {
        setCustomNameComponent(std::make_unique<text::StringTextComponent>(name));
    }
}

void Entity::setCustomNameComponent(std::unique_ptr<text::ITextComponent> name)
{
    auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    c->m_customName = std::move(name);
    // 数据管理器存 OptionalComponentValue（present + 纯文本），用于网络同步。
    if (c->m_customName) {
        m_dataManager.set(
            DATA_CUSTOM_NAME_PARAM, entity::OptionalComponentValue{true, c->m_customName->getUnformattedText()});
    } else {
        m_dataManager.set(DATA_CUSTOM_NAME_PARAM, entity::OptionalComponentValue{false, std::string{}});
    }
}

std::unique_ptr<text::ITextComponent> Entity::getDisplayName() const
{
    const auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    if (c->m_customName) {
        return c->m_customName->deepCopy();
    }
    // 返回默认名称
    return std::make_unique<text::StringTextComponent>("entity");
}

void Entity::setCustomNameVisible(bool visible)
{
    auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    c->m_customNameVisible = visible;
    m_dataManager.set(DATA_CUSTOM_NAME_VISIBLE_PARAM, c->m_customNameVisible);
}

void Entity::setSilent(bool silent)
{
    auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    c->m_silent = silent;
    m_dataManager.set(DATA_SILENT_PARAM, c->m_silent);
}

void Entity::setNoGravity(bool noGravity)
{
    auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
    MC_ASSERT_RELEASE(c != nullptr);
    c->m_noGravity = noGravity;
    m_dataManager.set(DATA_NO_GRAVITY_PARAM, c->m_noGravity);
}

// ============================================================================
// 实体标签实现
// ============================================================================

namespace {
// 标签数量上限
constexpr size_t MAX_TAGS = 1024;
} // namespace

bool Entity::addTag(const std::string& tag)
{
    // 每个实体最多1024个标签
    if (m_tags.size() >= MAX_TAGS) {
        return false;
    }
    auto result = m_tags.insert(tag);
    return result.second; // 如果插入成功返回 true
}

bool Entity::removeTag(const std::string& tag)
{
    return m_tags.erase(tag) > 0;
}

bool Entity::hasTag(const std::string& tag) const
{
    return m_tags.count(tag) > 0;
}

std::string Entity::getTypeId() const
{
    // 返回实体类型标识符；未设置时返回空串，由调用方判断，不编造占位值
    return m_typeId;
}

const entity::EntityType* Entity::entityType() const
{
    // 懒查询：缓存未命中且 m_typeId 非空时，按名查注册表填充。
    // 返回指针指向 EntityRegistry::m_types 内对象，与 VanillaEntityTypeKeys::* 同源，
    // 地址稳定（deque 不失效），可安全指针比较。
    if (m_entityType == nullptr && !m_typeId.empty()) {
        m_entityType = entity::EntityRegistry::instance().getType(m_typeId);
    }
    return m_entityType;
}

u32 Entity::getJavaEntityTypeId() const
{
    // 默认实现：按 entityType()->name()（如 "minecraft:item"）查 JavaEntityTypeIdMap，
    // 返回 vanilla 1.21.11 entity_type 注册表 id。船类 override 按木种拼变体名。
    const auto* type = entityType();
    const std::string_view name = type != nullptr ? std::string_view(type->name()) : std::string_view{};
    return JavaEntityTypeIdMap::instance().toJavaRegistryId(name);
}

std::string Entity::getLootTableId() const
{
    // 从实体类型ID推导默认战利品表路径
    // minecraft:pig -> minecraft:entities/pig
    // minecraft:zombie -> minecraft:entities/zombie
    const std::string& typeId = m_typeId;
    if (typeId.empty()) {
        return {};
    }

    // 查找命名空间和路径的分隔符
    auto colonPos = typeId.find(':');
    if (colonPos == std::string::npos) {
        // 没有命名空间前缀，使用默认 minecraft 命名空间
        return "minecraft:entities/" + typeId;
    }

    // 在路径部分前插入 "entities/"
    std::string ns = typeId.substr(0, colonPos);
    std::string path = typeId.substr(colonPos + 1);
    return ns + ":entities/" + path;
}

std::optional<ResourceLocation> Entity::makeSoundEventId(std::string_view suffix) const
{
    const std::string typeId = getTypeId();
    const size_t separatorPos = typeId.find(':');
    if (separatorPos == std::string::npos || separatorPos + 1 >= typeId.size()) {
        return std::nullopt;
    }

    const std::string typePath = typeId.substr(separatorPos + 1);
    if (typePath.empty() || typePath == "unknown") {
        return std::nullopt;
    }

    std::string soundId = "minecraft:entity.";
    soundId += typePath;
    soundId += '.';
    soundId += std::string(suffix);
    return ResourceLocation(soundId);
}

void Entity::playSound(const ResourceLocation& soundEventId, f32 volume, f32 pitch) const
{
    if (m_world == nullptr || isSilent()) {
        return;
    }

    m_world->playSound(soundEventId, getSoundCategory(), m_builtIn.stateVector->m_pos, volume, pitch);
}

ResourceLocation Entity::getSplashSound() const
{
    return SoundEvents::ENTITY_GENERIC_SPLASH;
}

ResourceLocation Entity::getHighspeedSplashSound() const
{
    // 默认返回与普通溅水相同的声音，子类可覆盖
    return SoundEvents::ENTITY_GENERIC_SPLASH;
}

void Entity::doWaterSplashEffect()
{
    // 确定控制者（骑乘时使用乘客的速度）
    // 获取速度向量
    Vector3 vel = velocity();
    f32 vx = vel.x;
    f32 vy = vel.y;
    f32 vz = vel.z;

    // 计算速度因子 f1
    // 默认 f = 0.2F（实体自己）
    f32 f1 = std::sqrt(vx * vx * 0.2f + vy * vy + vz * vz * 0.2f) * 0.2f;

    // f1 限制在 [0, 1] 范围
    if (f1 > 1.0f) {
        f1 = 1.0f;
    }

    // 获取随机数生成器
    math::Random& rng = m_world->getRandom();

    // 根据速度选择声音并播放
    // 音调: 1.0F + (this.rand.nextFloat() - this.rand.nextFloat()) * 0.4F
    f32 pitch = 1.0f + (rng.nextFloat() - rng.nextFloat()) * 0.4f;

    if (static_cast<f64>(f1) < 0.25) {
        playSound(getSplashSound(), f1, pitch);
    } else {
        playSound(getHighspeedSplashSound(), f1, pitch);
    }

    // 生成粒子
    // 粒子数量: 1 + width * 20
    i32 particleCount = static_cast<i32>(1.0f + width() * 20.0f);

    // Y 坐标: floor(posY) + 1.0 (水面上方一格)
    f32 particleY = std::floor(m_builtIn.stateVector->m_pos.y) + 1.0f;

    // 引入粒子类型
    using particle::ParticleTypeId;

    // 生成气泡粒子 (BUBBLE)
    for (i32 i = 0; i < particleCount; ++i) {
        // 位置: 在实体包围盒内随机
        f32 offsetX = (rng.nextDouble() * 2.0 - 1.0) * static_cast<f64>(width());
        f32 offsetZ = (rng.nextDouble() * 2.0 - 1.0) * static_cast<f64>(width());

        f32 particleX = m_builtIn.stateVector->m_pos.x + static_cast<f32>(offsetX);
        f32 particleZ = m_builtIn.stateVector->m_pos.z + static_cast<f32>(offsetZ);

        // 速度: 使用实体速度，Y 方向减去随机值
        f32 bubbleVy = vy - static_cast<f32>(rng.nextDouble() * 0.2);

        m_world->addParticle(
            ParticleTypeId::Bubble, Vector3(particleX, particleY, particleZ), Vector3(vx, bubbleVy, vz));
    }

    // 生成水溅粒子 (SPLASH)
    for (i32 j = 0; j < particleCount; ++j) {
        // 位置: 在实体包围盒内随机
        f32 offsetX = (rng.nextDouble() * 2.0 - 1.0) * static_cast<f64>(width());
        f32 offsetZ = (rng.nextDouble() * 2.0 - 1.0) * static_cast<f64>(width());

        f32 particleX = m_builtIn.stateVector->m_pos.x + static_cast<f32>(offsetX);
        f32 particleZ = m_builtIn.stateVector->m_pos.z + static_cast<f32>(offsetZ);

        // 速度: 使用实体速度
        m_world->addParticle(ParticleTypeId::Splash, Vector3(particleX, particleY, particleZ), Vector3(vx, vy, vz));
    }
}

void Entity::playStepSound(const BlockPos& pos, const BlockState* blockState)
{
    if (blockState == nullptr || m_world == nullptr) {
        return;
    }

    // 获取主脚步声方块位置（可能因 INSIDE/COMBINATION 标签而上移）
    BlockPos primaryPos = getPrimaryStepSoundBlockPos(pos);

    if (primaryPos != pos) {
        // 上方方块属于 INSIDE_STEP_SOUND_BLOCKS 或 COMBINATION_STEP_SOUND_BLOCKS
        const BlockState* primaryState = m_world->getBlockState(primaryPos);
        if (primaryState != nullptr) {
            // 随机音调偏移（供步声和紫水晶铃声共用）
            u32 seed = static_cast<u32>(m_id) ^ static_cast<u32>(m_ticksExisted);
            f32 randomValue = static_cast<f32>((seed * 1103515245 + 12345) % 32768) / 32768.0f;

            if (BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(*primaryState)) {
                // 组合步声：上方正常步声 + 下方沉闷步声
                playCombinationStepSounds(*primaryState, *blockState);
            } else {
                // 内部步声：只播放上方方块的步声
                const BlockSoundType& soundType = primaryState->getSoundType();
                f32 volume = soundType.getVolume() * 0.15f;
                f32 pitch = soundType.getPitch() * (0.8f + randomValue * 0.4f);
                playSound(soundType.getStepSound(), volume, pitch);
            }
            // 紫水晶共振铃声（始终检查脚下方块）
            if (shouldPlayAmethystStepSound(*blockState)) {
                playAmethystStepSound();
            }
            return;
        }
    }

    // 普通步声：播放脚下方块的步声
    const BlockSoundType& soundType = blockState->getSoundType();
    u32 seed = static_cast<u32>(m_id) ^ static_cast<u32>(m_ticksExisted);
    f32 randomValue = static_cast<f32>((seed * 1103515245 + 12345) % 32768) / 32768.0f;
    f32 volume = soundType.getVolume() * 0.15f;
    f32 pitch = soundType.getPitch() * (0.8f + randomValue * 0.4f);

    playSound(soundType.getStepSound(), volume, pitch);

    // 紫水晶共振铃声
    if (shouldPlayAmethystStepSound(*blockState)) {
        playAmethystStepSound();
    }
}

BlockPos Entity::getPrimaryStepSoundBlockPos(const BlockPos& pos) const
{
    if (m_world == nullptr) {
        return pos;
    }

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = m_world->getBlockState(abovePos);

    if (aboveState != nullptr && !aboveState->isAir()) {
        if (BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(*aboveState) ||
            BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(*aboveState)) {
            return abovePos;
        }
    }

    return pos;
}

void Entity::playCombinationStepSounds(const BlockState& aboveState, const BlockState& belowState)
{
    // 播放上方方块的正常步声
    const BlockSoundType& aboveSoundType = aboveState.getSoundType();
    u32 seed = static_cast<u32>(m_id) ^ static_cast<u32>(m_ticksExisted);
    f32 randomValue = static_cast<f32>((seed * 1103515245 + 12345) % 32768) / 32768.0f;
    f32 volume = aboveSoundType.getVolume() * 0.15f;
    f32 pitch = aboveSoundType.getPitch() * (0.8f + randomValue * 0.4f);

    playSound(aboveSoundType.getStepSound(), volume, pitch);

    // 播放下方方块的沉闷步声
    playMuffledStepSound(belowState);
}

void Entity::playMuffledStepSound(const BlockState& blockState)
{
    const BlockSoundType& soundType = blockState.getSoundType();
    f32 volume = soundType.getVolume() * 0.05f;
    f32 pitch = soundType.getPitch() * 0.8f;

    playSound(soundType.getStepSound(), volume, pitch);
}

bool Entity::shouldPlayAmethystStepSound(const BlockState& blockState) const
{
    return BlockTags::CRYSTAL_SOUND_BLOCKS().contains(blockState) &&
        static_cast<i32>(m_ticksExisted) >= m_lastCrystalSoundPlayTick + 20;
}

void Entity::playAmethystStepSound()
{
    // 惰性衰减：用 0.997^(elapsed_ticks) 补偿自上次播放以来的强度衰减
    i32 elapsedTicks = static_cast<i32>(m_ticksExisted) - m_lastCrystalSoundPlayTick;
    m_crystalSoundIntensity = m_crystalSoundIntensity * std::pow(0.997f, static_cast<f32>(elapsedTicks));

    // 累加并钳制到 [0, 1]
    m_crystalSoundIntensity = std::min(1.0f, m_crystalSoundIntensity + 0.07f);

    // 计算音调和音量
    f32 pitch = 0.5f + m_crystalSoundIntensity * m_random.nextFloat() * 1.2f;
    f32 volume = 0.1f + m_crystalSoundIntensity * 1.2f;

    playSound(SoundEvents::BLOCK_AMETHYST_BLOCK_CHIME, volume, pitch);
    m_lastCrystalSoundPlayTick = static_cast<i32>(m_ticksExisted);
}

void Entity::setPosition(f32 x, f32 y, f32 z)
{
    m_builtIn.stateVector->m_posPrev = m_builtIn.stateVector->m_pos;
    m_builtIn.stateVector->m_pos = Vector3(x, y, z);
    reapplyPosition();
}

void Entity::snapshotInterpolationState()
{
    m_builtIn.stateVector->m_posPrev = m_builtIn.stateVector->m_pos;
    m_builtIn.rotation->m_rotPrev.x = m_builtIn.rotation->m_rot.x;
    m_builtIn.rotation->m_rotPrev.y = m_builtIn.rotation->m_rot.y;
}

void Entity::setRotation(f32 yaw, f32 pitch)
{
    m_builtIn.rotation->m_rotPrev.x = m_builtIn.rotation->m_rot.x;
    m_builtIn.rotation->m_rotPrev.y = m_builtIn.rotation->m_rot.y;
    m_builtIn.rotation->m_rot.x = yaw;
    m_builtIn.rotation->m_rot.y = pitch;
}

void Entity::setVelocity(f32 x, f32 y, f32 z)
{
    m_builtIn.velocity->m_velocity = Vector3(x, y, z);
}

void Entity::moveRelative(f32 factor, f32 strafe, f32 vertical, f32 forward)
{
    // 计算输入向量长度平方
    f32 lengthSq = strafe * strafe + vertical * vertical + forward * forward;
    if (lengthSq < 1.0E-7f) {
        // 输入向量太短，不移动
        return;
    }

    // 如果长度大于 1，归一化
    f32 length = std::sqrt(lengthSq);
    f32 normalizedStrafe, normalizedVertical, normalizedForward;
    if (lengthSq > 1.0f) {
        normalizedStrafe = strafe / length;
        normalizedVertical = vertical / length;
        normalizedForward = forward / length;
    } else {
        normalizedStrafe = strafe;
        normalizedVertical = vertical;
        normalizedForward = forward;
    }

    // 应用移动因子
    normalizedStrafe *= factor;
    normalizedVertical *= factor;
    normalizedForward *= factor;

    // 根据偏航角旋转移动向量
    // MC 公式: absoluteX = vec.x * cos - vec.z * sin
    //          absoluteZ = vec.z * cos + vec.x * sin
    // 注意: strafe 对应 x 方向，forward 对应 z 方向
    f32 yawRad = m_builtIn.rotation->m_rot.x * math::DEG_TO_RAD;
    f32 sinYaw = std::sin(yawRad);
    f32 cosYaw = std::cos(yawRad);

    f32 moveX = normalizedStrafe * cosYaw - normalizedForward * sinYaw;
    f32 moveZ = normalizedForward * cosYaw + normalizedStrafe * sinYaw;

    // 添加到当前速度
    m_builtIn.velocity->m_velocity.x += moveX;
    m_builtIn.velocity->m_velocity.y += normalizedVertical;
    m_builtIn.velocity->m_velocity.z += moveZ;
}

void Entity::tick()
{
    m_ticksExisted++;

    // 基础 tick
    baseTick();

    // 传送门逻辑（tickPortal + onPortalTriggered）已迁入 PortalTickSystem，
    // 在 SystemPhase::PostEntityTick 阶段执行（本 tick 之后）。见 PortalTickSystem.hpp。
}

void Entity::baseTick()
{
    // 更新前一帧位置
    m_builtIn.stateVector->m_posPrev = m_builtIn.stateVector->m_pos;
    m_builtIn.rotation->m_rotPrev.x = m_builtIn.rotation->m_rot.x;
    m_builtIn.rotation->m_rotPrev.y = m_builtIn.rotation->m_rot.y;

    // 检查车辆是否被移除
    // 如果正在骑乘且车辆已被移除，则下车
    if (isRiding()) {
        if (m_world) {
            Entity* vehicle = m_world->getEntity(m_vehicle);
            if (vehicle == nullptr || vehicle->isRemoved()) {
                stopRiding();
            }
        } else {
            stopRiding();
        }
    }

    // 更新传送冷却——已迁入 PortalTickSystem（PostEntityTick 阶段）。

    // 骑乘冷却（m_rideCooldown）已整体移除（对齐 vanilla，详见 addPassenger 注释），不再递减。

    // 更新环境状态（包括水中/岩浆/眼睛流体等状态）
    // MC Java: Entity.baseTick() 中在火焰处理之前调用 updateInWaterStateAndDoFluidPushing() + updateFluidOnEyes()
    // 此处将 updateEnvironmentState() 移至火焰处理之前，与 MC 原版时序一致
    // 火焰链（fire 递减/伤害/水中熄灭/雨中扑灭）已迁入 FireTickSystem（PostEntityTick 阶段），
    // 在本 baseTick 之后执行，可读到此处刚产出的环境状态。
    updateEnvironmentState();

    // 在岩浆中减少坠落距离
    if (isInLava()) {
        m_builtIn.physicsState->m_fallDistance *= 0.5f;
    }

    // 冰冻状态处理
    // 在火焰处理之后重置 isInPowderSnow
    // 实际的冰冻 tick 递增由 PowderSnowBlock::onEntityCollision() 处理
    // 实际的冰冻 tick 递减和伤害在 LivingEntity 中处理
    if (auto* freeze = m_entityContext->tryGetComponent<ecs::FreezeComponent>()) {
        freeze->m_isInPowderSnow = false;
    }

    // 空气值处理完全由 LivingEntity::updateAirSupply() 负责
    // 非 LivingEntity 实体（如 ItemEntity）不使用 Entity 层的空气处理
    // ItemEntity 的水中物理行为在自身的 _applyWaterPhysics() 中独立处理

    // 重新探测地面状态，避免实体在脚下方块被移除后仍然沿用旧的 onGround 缓存。
    checkOnGround();

    // 清除运动速度乘数
    // 每帧开始时清除，由 onEntityCollision 在需要时重新设置
    clearMotionMultiplier();

    // 载具乘客位置同步（对齐 Java Entity.rideTick 中的 positionRider 调用）。
    // Java 由 Level.tickPassenger 对每个乘客调 passenger.rideTick()，其中
    // getVehicle().positionRider(this) 把乘客位置吸附到载具。Cubium 无世界级
    // tickPassenger 阶段，故在 baseTick 末尾由载具主动同步自身所有乘客位置。
    // 时序：baseTick 是实体 tick 第一步，载具本 tick 后续移动（aiStep/travel）尚未
    // 应用，乘客位置滞后一 tick 收敛；GameTest 轮询判定可接受。
    // 仅当本实体是载具（有乘客）时才同步，避免无谓遍历。
    if (!m_passengers.empty()) {
        updatePassengers();
    }
}

bool Entity::onPortalTriggered()
{
    // 基类实现：默认不做任何事
    // 子类（如 ServerPlayer）可重写此方法以实现实际的维度切换逻辑

    // 重置传送门状态
    setInPortal(false);
    resetPortalTime();
    triggerPortalCooldown();

    return false;
}

bool Entity::isOnLadder() const
{
    // 检查实体碰撞箱内的方块是否为可攀爬方块

    if (m_world == nullptr) {
        return false;
    }

    // 检查实体碰撞箱内的方块
    const AxisAlignedBB box = boundingBox();

    i32 minX = static_cast<i32>(std::floor(box.minX));
    i32 maxX = static_cast<i32>(std::floor(box.maxX));
    i32 minY = static_cast<i32>(std::floor(box.minY));
    i32 maxY = static_cast<i32>(std::floor(box.maxY));
    i32 minZ = static_cast<i32>(std::floor(box.minZ));
    i32 maxZ = static_cast<i32>(std::floor(box.maxZ));

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                const BlockState* blockState = m_world->getBlockState(x, y, z);
                if (blockState != nullptr) {
                    const Block& block = blockState->getBlock();
                    BlockPos pos(x, y, z);
                    if (block.isLadder(*blockState, m_world, &pos, this)) {
                        // 记录攀爬位置
                        const_cast<Entity*>(this)->m_lastClimbPos = pos;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void Entity::updateEnvironmentState()
{
    // 需要遍历碰撞箱内的所有方块，计算流体浸入高度

    // 眼睛位置（用于判断眼睛是否在水下）
    const f32 eyeY = m_builtIn.stateVector->m_pos.y + eyeHeight();
    const i32 eyeBlockY = static_cast<i32>(std::floor(eyeY));

    // 重置流体状态
    m_inWater = false;
    m_inLava = false;
    m_waterHeight = 0.0f;
    m_lavaHeight = 0.0f;
    m_eyesInWater = false;
    m_eyesInLava = false;

    if (m_world == nullptr && m_physicsEngine == nullptr) {
        return;
    }

    // 获取碰撞箱并收缩一点以避免边界问题
    AxisAlignedBB box = boundingBox().shrink(0.001);

    // 计算碰撞箱覆盖的方块范围
    const i32 minX = static_cast<i32>(std::floor(box.minX));
    const i32 maxX = static_cast<i32>(std::floor(box.maxX));
    const i32 minY = static_cast<i32>(std::floor(box.minY));
    const i32 maxY = static_cast<i32>(std::floor(box.maxY));
    const i32 minZ = static_cast<i32>(std::floor(box.minZ));
    const i32 maxZ = static_cast<i32>(std::floor(box.maxZ));

    // 遍历碰撞箱内的所有方块
    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                // 获取流体状态
                const fluid::FluidState* fluidState = nullptr;
                if (m_world) {
                    fluidState = m_world->getFluidState(x, y, z);
                } else if (m_physicsEngine) {
                    const ICollisionWorld* collisionWorld = m_physicsEngine->getWorld();
                    if (collisionWorld) {
                        const BlockState* blockState = collisionWorld->getBlockState(x, y, z);
                        fluidState = blockState != nullptr ? blockState->getFluidState() : nullptr;
                    }
                }

                if (fluidState == nullptr || fluidState->isEmpty()) {
                    continue;
                }

                // 计算流体高度
                // MC: (float)y + fluidState.getActualHeight()
                f32 fluidTopY = static_cast<f32>(y) + fluidState->getHeight();

                // 检查流体是否在碰撞箱内
                if (fluidTopY > box.minY) {
                    // 计算浸入高度
                    f32 submergedHeight = fluidTopY - box.minY;

                    // 判断流体类型
                    const ResourceLocation& fluidId = fluidState->getFluid().fluidLocation();
                    bool isWater = fluidId.namespace_() == "minecraft" &&
                        (fluidId.path() == "water" || fluidId.path() == "flowing_water");
                    bool isLava = fluidId.namespace_() == "minecraft" &&
                        (fluidId.path() == "lava" || fluidId.path() == "flowing_lava");

                    if (isWater) {
                        m_inWater = true;
                        m_waterHeight = std::max(m_waterHeight, submergedHeight);

                        // 检查眼睛是否在水下
                        // 眼睛位置稍微下移 0.11111111 来检测
                        constexpr f32 EYE_OFFSET = 0.11111111f;
                        f32 adjustedEyeY = eyeY - EYE_OFFSET;
                        if (fluidTopY > adjustedEyeY) {
                            m_eyesInWater = true;
                        }
                    } else if (isLava) {
                        m_inLava = true;
                        m_lavaHeight = std::max(m_lavaHeight, submergedHeight);

                        // 检查眼睛是否在岩浆中
                        constexpr f32 EYE_OFFSET = 0.11111111f;
                        f32 adjustedEyeY = eyeY - EYE_OFFSET;
                        if (fluidTopY > adjustedEyeY) {
                            m_eyesInLava = true;
                        }
                    }
                }
            }
        }
    }

    // 兼容旧代码：设置 m_fluidHeight
    m_fluidHeight = std::max(m_waterHeight, m_lavaHeight);
}

bool Entity::isInRain() const
{
    // 检查脚底位置和碰撞盒顶部位置两个位置
    if (m_world == nullptr) {
        return false;
    }

    // 检查世界是否正在下雨
    if (!m_world->isRaining()) {
        return false;
    }

    // 检查实体脚底位置是否可以降雨
    BlockPos footPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    if (m_world->canRainAt(footPos)) {
        return true;
    }

    // 检查实体碰撞盒顶部位置是否可以降雨
    BlockPos topPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.aabbShape->m_aabb.maxY)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    return m_world->canRainAt(topPos);
}

f32 Entity::getBrightness() const
{
    // 对齐 vanilla 1.21.11 Entity.getLightLevelDependentMagicValue()（Entity.java:1643）。
    // vanilla 公式（LevelReader.java:118）：
    //   float f  = getMaxLocalRawBrightness(pos) / 15.0F;   // getMaxLocalRawBrightness 含 getSkyDarken() 时间衰减
    //   float f1 = f / (4.0F - 3.0F * f);                   // 非线性 gamma 曲线
    //   return Mth.lerp(dimensionType.ambientLight(), f1, 1.0F);
    // 此前实现走 IWorld::getBrightness(pos) = getLightSubtracted(pos,0)/15——skyDarkening 硬传 0 无时间衰减，
    // 夜晚露天（skyLight=15）仍返回 1.0，导致 SpiderTargetGoal(brightness<0.5F) 永不触发、蜘蛛夜晚不攻击。
    // 改用 getMaxLocalRawBrightness（含 getSkyDarkening 夜晚≈11 的时间衰减）+ gamma 曲线 + ambientLight。
    if (m_world == nullptr) {
        return 0.0f;
    }

    // 使用眼睛高度位置（vanilla BlockPos.containing(getX, getEyeY, getZ)）
    BlockPos pos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y + static_cast<f64>(eyeHeight()))),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));

    const i32 rawBrightness = m_world->getMaxLocalRawBrightness(pos); // 含 getSkyDarkening 时间衰减
    const f32 f = static_cast<f32>(rawBrightness) / 15.0f;
    // gamma：f=0→0, f=1→1, 中间值被压低（f=0.267→0.083），使"半暗"判定更敏感
    const f32 f1 = f / (4.0f - 3.0f * f);
    // ambientLight：下界=0.1（微弱环境光），主世界/末地=0.0（对齐 DimensionType.cpp:63/103/143）。
    // IWorld 无 dimensionType 访问器，用 dimension() 推导避免新增虚函数改所有子类。
    // 维度 id：主世界 OVERWORLD=0，下界 NETHER=-1，末地 THE_END=1（DimensionManager.hpp:64-70）。
    // 下界/末地均 hasSkyLight=false，getMaxLocalRawBrightness 只返回 blockLight，此分支实际不影响
    // 亡灵燃烧判定（无天空光维度无露天），但 ambientLight 须与 vanilla 一致以保其他光照判定正确。
    const f32 ambientLight = (m_world->dimension() == -1) ? 0.1f : 0.0f;
    // MathUtils::lerp(a,b,t) 参数顺序≠Mth.lerp(t,a,b)：Mth.lerp(ambient,f1,1.0) == lerp(f1,1.0,ambient)
    return mc::math::lerp(f1, 1.0f, ambientLight);
}

void Entity::syncMetadataFromDataManager()
{
    // 第四批：flags/air/customName/customNameVisible/silent/noGravity/pose 七字段均已迁入
    // ecs::EntityFlagsComponent / EntityStateComponent（真相源），DataParameter 退为同步镜像。
    // 组件不从镜像回填，避免双写时序错乱。延续第二批 freeze / 第三批 health/arrows 模式。
    // m_ticksFrozen 同理（FreezeComponent 真相源，DATA_TICKS_FROZEN_PARAM 退为镜像），
    // 所有写入统一走 setTicksFrozen()（同时写组件 + DataParameter）。
}

void Entity::updateFallDistance(f32 actualMovementY)
{
    // 对齐 vanilla Entity#checkFallDamage（Entity.java:1420-1440）。
    // vanilla 在 Entity.move:754 调 checkFallDamage(vec3.y, onGround, ...)，vec3=collide() 结果即
    // 碰撞后实际 y 位移（actualMovement.y），且在 Y 速度清零前累积。两个判断为独立 if（非 else）：
    //   1. !isInWater() && y<0 → fallDistance -= y（累积本帧下落量，着地帧也累积）
    //   2. onGround && fallDistance>0 → fallOn(伤害) + resetFallDistance
    // 关键差异（修复 #264）：此前 Cubium 用 velocity.y 且 else 分支，着地帧 velocity.y 已被
    // 碰撞清零不累积本帧下落量，少算着地帧的部分下落量致摔落伤害绝对值偏低。改用
    // actualMovement.y（着地帧为碰撞截断后的小量，与 vanilla vec3.y 一致）+ 独立 if 着地帧先累积。
    // 水中守卫对齐 vanilla !isInWater()（m_inWater 由 baseTick→updateEnvironmentState 先于 move 设置）。
    if (!isInWater() && actualMovementY < 0.0f) {
        m_builtIn.physicsState->m_fallDistance -= actualMovementY;
    }

    if (m_builtIn.physicsState->m_onGround && m_builtIn.physicsState->m_fallDistance > 0.0f) {
        // 着地时触发踩上方块的 onFallenUpon 回调
        // Block::onFallenUpon 默认实现会调用 entity.causeFallDamage() 施加普通摔落伤害
        // 子类（如 PointedDripstoneBlock）可替代默认摔落伤害
        _handleLandingOnBlock();
        m_builtIn.physicsState->m_fallDistance = 0.0f;
    }
}

void Entity::handleFallDamage(f32 /* distance */, f32 /* damageMultiplier */)
{
    // 基础实体不处理摔落伤害
    // LivingEntity 会重写此方法
}

void Entity::_checkFallDamageResettingBlocks(const Vector3& actualMovement)
{
    // 对齐 vanilla Entity.move（Entity.java:718-725）的 FALLDAMAGE_RESETTING 射线检测。
    // vanilla 逻辑：
    //   double d0 = vec3.lengthSqr();  // vec3 = collide() 碰撞后实际位移
    //   if ((d0 > 1.0E-7 || p_19974_.lengthSqr() - d0 < 1.0E-7) && this.fallDistance != 0.0 && d0 >= 1.0) {
    //       double d1 = Math.min(vec3.length(), 8.0);
    //       Vec3 vec32 = this.position().add(vec3.normalize().scale(d1));
    //       BlockHitResult blockhitresult = this.level().clip(new ClipContext(
    //           this.position(), vec32, ClipContext.Block.FALLDAMAGE_RESETTING, ClipContext.Fluid.WATER, this));
    //       if (blockhitresult.getType() != HitResult.Type.MISS) this.resetFallDistance();
    //   }
    // 注意：vanilla position() 此处是移动前位置（setPos 在 :731 才调）。Cubium moveWithCollision
    // 在 :1233-1241 已先把位置更新到碰撞后，故此处 position() 是碰撞后位置。射线起点取当前位置、
    // 沿 actualMovement 方向延伸，语义等价（命中路径上的摔落重置方块即重置）。
    //
    // Cubium 用 IWorld::isBlockInLine（DDA 逐格遍历，不做形状相交）替代 ClipContext 射线：
    // vanilla 的 ClipContext.Block.FALLDAMAGE_RESETTING ShapeGetter 对 FALL_DAMAGE_RESETTING
    // 标签方块返回 Shapes.block()（完整形状），对其他方块返回 empty shape，故射线只可能命中标签
    // 方块。isBlockInLine 对路径上每个方块调 predicate，predicate 判定是否为标签方块，语义一致，
    // 且能命中空碰撞形状方块（蜘蛛网/甜浆果丛）——这正是 vanilla 用 FALLDAMAGE_RESETTING 而非
    // 普通碰撞射线的原因。仅 ServerWorld 实现该方法（客户端返回 false）。
    const f32 lengthSqr = actualMovement.lengthSquared();
    if (m_world == nullptr) {
        return;
    }
    if (m_builtIn.physicsState->m_fallDistance == 0.0f) {
        return;
    }
    if (lengthSqr < 1.0f) {
        return;
    }

    const f32 moveLength = std::sqrt(lengthSqr);
    const f32 rayLength = std::min(moveLength, 8.0f);
    // 单位方向 × 射线长度。actualMovement 为 f32，转 f64 供 isBlockInLine（Vector3d）。
    const Vector3 dir = actualMovement.normalized();
    const Vector3 fromPos = position();
    const Vector3d from(static_cast<f64>(fromPos.x), static_cast<f64>(fromPos.y), static_cast<f64>(fromPos.z));
    const Vector3d to(static_cast<f64>(fromPos.x + dir.x * rayLength),
        static_cast<f64>(fromPos.y + dir.y * rayLength),
        static_cast<f64>(fromPos.z + dir.z * rayLength));

    const bool hit = m_world->isBlockInLine(
        from, to, [](const BlockState& state) { return BlockTags::FALL_DAMAGE_RESETTING().contains(state); });
    if (hit) {
        m_builtIn.physicsState->m_fallDistance = 0.0f;
    }
}

void Entity::causeFallDamage(f32 distance, f32 damageMultiplier, const DamageSource& source)
{
    // MC 1.21.11: Entity.causeFallDamage 首先传播摔落伤害给所有乘客，基类不处理自身伤害
    // 参考: net.minecraft.world.entity.Entity.causeFallDamage → propagateFallToPassengers
    // LivingEntity.causeFallDamage 调用 super.causeFallDamage（传播给乘客）后自行计算伤害
    propagateFallToPassengers(distance, damageMultiplier, source);
}

void Entity::propagateFallToPassengers(f32 distance, f32 damageMultiplier, const DamageSource& source)
{
    // MC 1.21.11: 当载具受到摔落伤害时，所有乘客也受到相同的摔落伤害
    // 参考: net.minecraft.world.entity.Entity.propagateFallToPassengers
    if (!hasPassengers() || m_world == nullptr) {
        return;
    }
    // 拷贝乘客列表后再遍历，避免乘客在 causeFallDamage 过程中死亡
    // 导致从 m_passengers 移除时迭代器失效
    auto passengers = m_passengers;
    for (EntityInstanceId passengerId : passengers) {
        Entity* passenger = m_world->getEntity(passengerId);
        if (passenger != nullptr) {
            passenger->causeFallDamage(distance, damageMultiplier, source);
        }
    }
}

void Entity::_handleLandingOnBlock()
{
    // 着地时踩上所在方块的 onFallenUpon 回调
    // Block::onFallenUpon 默认实现会调用 entity.causeFallDamage() 施加普通摔落伤害
    // 子类可重写 onFallenUpon 以自定义摔落行为（如石笋增加伤害、蜂蜜块取消伤害等）
    if (m_world == nullptr) {
        return;
    }
    // 获取实体脚下方块
    BlockPos landingPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(m_builtIn.stateVector->m_pos.y) - 1,
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    const BlockState* state = m_world->getBlockState(landingPos);
    if (state != nullptr) {
        // onFallenUpon 是非 const 方法，需要通过 getBlockMutable() 获取可变引用
        state->getBlockMutable().onFallenUpon(
            *m_world, landingPos, *state, *this, m_builtIn.physicsState->m_fallDistance);
    }
}

void Entity::update()
{
    // 保存上一帧位置
    m_builtIn.stateVector->m_posPrev = m_builtIn.stateVector->m_pos;
    m_builtIn.rotation->m_rotPrev.x = m_builtIn.rotation->m_rot.x;
    m_builtIn.rotation->m_rotPrev.y = m_builtIn.rotation->m_rot.y;
}

void Entity::move(f32 dx, f32 dy, f32 dz)
{
    m_builtIn.stateVector->m_posPrev = m_builtIn.stateVector->m_pos;
    m_builtIn.stateVector->m_pos.x += dx;
    m_builtIn.stateVector->m_pos.y += dy;
    m_builtIn.stateVector->m_pos.z += dz;
    reapplyPosition();
}

void Entity::move(entity::MoverType type, const Vector3& delta)
{
    MC_UNUSED(type);
    // 移动类型用于区分移动来源
    // 目前简单委托给无碰撞版本，后续可添加碰撞检测
    move(delta.x, delta.y, delta.z);
}

void Entity::rotate(f32 deltaYaw, f32 deltaPitch)
{
    m_builtIn.rotation->m_rotPrev.x = m_builtIn.rotation->m_rot.x;
    m_builtIn.rotation->m_rotPrev.y = m_builtIn.rotation->m_rot.y;
    m_builtIn.rotation->m_rot.x += deltaYaw;
    m_builtIn.rotation->m_rot.y += deltaPitch;

    // 限制俯仰角范围
    m_builtIn.rotation->m_rot.y = std::clamp(m_builtIn.rotation->m_rot.y, -90.0f, 90.0f);

    // 规范化偏航角到 [0, 360) 范围
    m_builtIn.rotation->m_rot.x = math::wrapDegreesPositive(m_builtIn.rotation->m_rot.x);
}

/**
 * @brief 带碰撞检测的移动
 *
 * 参考MC的Entity.move()实现。
 * 核心流程：
 * 1. 使用物理引擎执行碰撞检测和移动
 * 2. 更新实体位置（从碰撞箱计算）
 * 3. 更新碰撞状态和地面状态
 * 4. 更新坠落距离
 */
Vector3 Entity::moveWithCollision(f32 dx, f32 dy, f32 dz)
{
    Vector3 desiredMovement(dx, dy, dz);

    // noClip 检查 - 无视碰撞的实体直接移动
    if (m_noClip) {
        // 无碰撞模式：直接更新位置，不检测碰撞
        m_builtIn.stateVector->m_pos.x += dx;
        m_builtIn.stateVector->m_pos.y += dy;
        m_builtIn.stateVector->m_pos.z += dz;
        reapplyPosition();

        // 即使 noClip=true 也要触发方块碰撞
        doBlockCollisions();
        return desiredMovement;
    }

    // 应用运动速度乘数（甜浆果丛、蜘蛛网等减速效果）
    if (m_hasMotionMultiplier) {
        desiredMovement.x *= m_motionMultiplier.x;
        desiredMovement.y *= m_motionMultiplier.y;
        desiredMovement.z *= m_motionMultiplier.z;
    }

    // 重置碰撞状态
    m_builtIn.physicsState->m_collidedHorizontally = false;
    m_builtIn.physicsState->m_collidedVertically = false;

    // 优先使用 World 的物理引擎
    PhysicsEngine* physics = physicsEngine();

    if (!physics) {
        // 无物理引擎，直接移动
        move(dx, dy, dz);
        // 尝试通过 World 检测地面
        checkOnGround();
        return desiredMovement;
    }

    // 获取当前碰撞箱
    AxisAlignedBB entityBox = boundingBox();

    // 使用物理引擎执行碰撞检测移动
    // 传 this 构造实体碰撞上下文，使需要按实体区分碰撞形状的方块（如细雪 PowderSnowBlock：
    // 可行走实体得完整碰撞箱、下落实体得半穿透形状）在物理移动中正确生效。
    Vector3 actualMovement = physics->moveEntity(this, entityBox, desiredMovement, stepHeight());

    // 从碰撞箱更新位置
    // 实体位置 = 碰撞箱底部中心
    m_builtIn.stateVector->m_pos = Vector3((entityBox.minX + entityBox.maxX) / 2.0f, // 中心X
        entityBox.minY,                                                              // 底部Y
        (entityBox.minZ + entityBox.maxZ) / 2.0f                                     // 中心Z
    );
    reapplyPosition();

    // 更新碰撞状态（从物理引擎获取）
    m_builtIn.physicsState->m_collidedHorizontally = physics->collidedHorizontally();
    m_builtIn.physicsState->m_collidedVertically = physics->collidedVertically();

    // 更新地面状态（对齐 vanilla Entity.move:741-742）。
    // vanilla onGround = verticalCollisionBelow = verticalCollision && movement.y < 0.0，
    // 即仅当本帧确实发生垂直碰撞（下落被方块截断）且向下移动时才判在地面。vanilla 无"接触探测"，
    // 字段语义与 checkFallDamage 着地分支用的是同一个 onGround（Entity.java:754 传入 this.onGround()）。
    //
    // 此前 Cubium 额外用 groundedByContact = physics->isOnGround(entityBox)（entityBox 下移
    // EPSILON_GROUND_PROBE 探测接触）作为 onGround 的第二来源：在实体接近地面但本帧未实际发生 Y
    // 碰撞（如站立帧浮点抖动、水平移动跨方块边界）时提前判 onGround=true。理论上这会令
    // updateFallDistance 的着地分支（if(onGround && fallDistance>0)）提前一帧触发，fallDistance
    // 少算着地帧的累积量（任务 #273 原始假设）。
    //
    // 任务 #273 诊断结论：fall_tower 场景 fallDistance≈9 是真实几何落差（中心柱 y=0 tuff+y=1
    // cobblestone 两层完整方块，落点顶面 y=2.0，落差 9 格），并非接触探测提前停止累积——着地帧
    // collidedVertically 稳定为 true，groundedByContact 与 groundedByCollision 同时为 true，
    // 移除前者后伤害值不变。但 groundedByContact 作为非 vanilla 的提前判定来源仍是潜在正确性
    // 风险（站立抖动/跨方块边界场景可能提前触发着地分支），故移除以对齐 vanilla 语义。此为预防性
    // 对齐修复，不改变 fall_tower 场景行为。
    //
    // 经分析：站立稳定时重力每帧施加 velocity.y = -GRAVITY(0.08)，moveWithCollision 的
    // calculateYOffset 会把下落截断到接触面（resolved.y≈0），|resolved.y - movement.y|≈0.08 远大于
    // EPSILON_COLLISION，故 collidedVertically 稳定为 true，groundedByCollision 可靠，无需接触探测。
    // 接触探测能力仍由 PhysicsEngine::isOnGround 保留，供 stepUp（PhysicsEngine.cpp:121）独立调用
    // （stepUp 不读 m_onGround 字段，走自己的 isOnGround 探测，本改动不影响 stepUp）。
    bool groundedByCollision = m_builtIn.physicsState->m_collidedVertically && desiredMovement.y < 0.0f;
    bool wasOnGround = m_builtIn.physicsState->m_onGround;
    m_builtIn.physicsState->m_onGround = groundedByCollision;

    // 落地时清空攀爬位置
    if (m_builtIn.physicsState->m_onGround && !wasOnGround) {
        m_lastClimbPos = std::nullopt;
    }

    // 如果某轴发生碰撞（实际移动 != 期望移动），清零该轴速度
    // 注意：使用 MC 的 MathHelper.epsilonEquals 比较，阈值约 1e-7
    if (std::abs(desiredMovement.x - actualMovement.x) > math::EPSILON_COLLISION) {
        // X轴碰撞，清零X速度
        m_builtIn.velocity->m_velocity.x = 0.0f;
    }
    if (std::abs(desiredMovement.z - actualMovement.z) > math::EPSILON_COLLISION) {
        // Z轴碰撞，清零Z速度
        m_builtIn.velocity->m_velocity.z = 0.0f;
    }
    // Y轴：对齐 vanilla Entity.move（Entity.java:740-770）。vanilla 的 move 从不在 Y 碰撞时
    // 无条件清零 Y 速度，而是调用 Block.updateEntityMovementAfterFallOn 让方块决定 Y 速度
    // （SlimeBlock 反弹取反为正、普通方块 super 归零）。Cubium 的 onLanded
    // （doBlockCollisionsAfterMove 内 :1424）对应此回调。此前此处无条件清零 Y 速度，导致
    // onLanded 被调用时 velocity.y 已为 0，SlimeBlock::onLanded 的 `velocity.y < 0` 反弹条件
    // 永不成立（粘液块不弹跳）。修复：Y 碰撞时不在 onLanded 之前清零，先调
    // doBlockCollisionsAfterMove→onLanded（用未清零的下落速度反弹）；onLanded 后若 velocity.y
    // 仍为下落（<0，普通方块未反弹）才清零，反弹的正速度（SlimeBlock）保留。
    bool yCollision = std::abs(desiredMovement.y - actualMovement.y) > math::EPSILON_COLLISION;

    // 方块碰撞回调（含 onLanded，对齐 vanilla updateEntityMovementAfterFallOn，在 Y 速度清零前调用）
    doBlockCollisionsAfterMove(actualMovement, desiredMovement);

    // onLanded 之后的 Y 速度收尾：Y 碰撞且 onLanded 未把 velocity.y 改为正（普通方块未反弹）时清零。
    // SlimeBlock::onLanded 已 setVelocity(vx, -vy*bounceFactor, vz) 把 Y 改为正（反弹），此处保留。
    if (yCollision && m_builtIn.velocity->m_velocity.y < 0.0f) {
        m_builtIn.velocity->m_velocity.y = 0.0f;
    }

    // 更新摔落距离并处理摔落伤害（对齐 vanilla checkFallDamage(vec3.y, ...)，用碰撞后实际 y 位移）。
    // 先做 FALLDAMAGE_RESETTING 射线检测：若本帧路径穿过蜘蛛网/甜浆果丛/可攀爬方块等摔落重置
    // 方块，resetFallDistance（对齐 vanilla Entity.move:718-725，在 checkFallDamage 之前）。
    _checkFallDamageResettingBlocks(actualMovement);
    updateFallDistance(actualMovement.y);

    return actualMovement;
}

PhysicsEngine* Entity::physicsEngine()
{
    // 优先使用 World 的物理引擎
    if (m_world) {
        PhysicsEngine* engine = m_world->physicsEngine();
        if (engine) return engine;
    }
    // 备用：显式设置的物理引擎（客户端兼容）
    return m_physicsEngine;
}

const PhysicsEngine* Entity::physicsEngine() const
{
    // 优先使用 World 的物理引擎
    if (m_world) {
        const PhysicsEngine* engine = m_world->physicsEngine();
        if (engine) return engine;
    }
    // 备用：显式设置的物理引擎（客户端兼容）
    return m_physicsEngine;
}

void Entity::checkOnGround()
{
    const AxisAlignedBB box = boundingBox();

    if (m_world) {
        // 检测实体下方是否有方块
        AxisAlignedBB groundProbe = box;
        groundProbe.minY -= 0.1f;                   // 向下延伸一点
        groundProbe.maxY = groundProbe.minY + 0.1f; // 扁平的检测区域

        m_builtIn.physicsState->m_onGround = m_world->hasBlockCollision(groundProbe);
        return;
    }

    if (m_physicsEngine) {
        m_builtIn.physicsState->m_onGround = m_physicsEngine->isOnGround(this, box);
        return;
    }

    m_builtIn.physicsState->m_onGround = false;
}

void Entity::doBlockCollisions()
{
    // 遍历实体碰撞箱覆盖的所有方块，调用方块的 onEntityCollision 方法

    if (m_world == nullptr) {
        return;
    }

    // MC Java: applyEffectsFromBlocks() - 记录方块碰撞前的火焰计时器
    // 用于判断方块碰撞是否点燃了实体，如果未被点燃且不处于燃烧状态，则设置火焰免疫期
    i32 fireTicksBeforeCollision = getRemainingFireTicks();

    // 获取碰撞箱范围，稍微收缩避免边界精度问题
    AxisAlignedBB box = m_builtIn.aabbShape->m_aabb.shrink(0.001);
    BlockPos minPos(static_cast<i32>(std::floor(box.minX)),
        static_cast<i32>(std::floor(box.minY)),
        static_cast<i32>(std::floor(box.minZ)));
    BlockPos maxPos(static_cast<i32>(std::floor(box.maxX)),
        static_cast<i32>(std::floor(box.maxY)),
        static_cast<i32>(std::floor(box.maxZ)));

    // 遍历碰撞箱覆盖的所有方块
    // 注意：getBlockState 在区块未加载时返回 nullptr，因此不需要预先检查 isAreaLoaded
    for (i32 x = minPos.x; x <= maxPos.x; ++x) {
        for (i32 y = minPos.y; y <= maxPos.y; ++y) {
            for (i32 z = minPos.z; z <= maxPos.z; ++z) {
                BlockPos pos(x, y, z);
                const BlockState* blockState = m_world->getBlockState(pos);
                if (blockState != nullptr && !blockState->isAir()) {
                    const Block& block = blockState->getBlock();

                    // 参考 MC Java: Entity.checkInsideBlocks() 使用 getEntityInsideCollisionShape
                    // 检查实体 AABB 是否与方块的实体内部碰撞形状重叠。
                    // 大多数方块返回完整方块形状（快速路径），特殊方块如炼药锅
                    // 返回更精确的形状，只有实体进入内容区域时才触发 onEntityCollision。
                    const CollisionShape& insideShape = block.getEntityInsideCollisionShape(*blockState);
                    bool isInsideBlock = false;
                    if (insideShape.isFullBlock()) {
                        // 快速路径：完整方块形状，AABB 与方块网格重叠即视为在方块内部
                        isInsideBlock = true;
                    } else if (!insideShape.isEmpty()) {
                        // 精确路径：使用形状的 AABB 进行相交检测
                        isInsideBlock = insideShape.intersects(m_builtIn.aabbShape->m_aabb, pos.x, pos.y, pos.z);
                    }

                    if (isInsideBlock) {
                        // 调用方块的实体碰撞回调
                        block.onEntityCollision(*blockState, *m_world, pos, *this);

                        // 派发自定义方块组件回调 - onEntity
                        auto& blockReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
                        std::string typeId = block.blockLocation().toString();
                        if (blockReg.hasEntityCallback(typeId)) {
                            mc::mod::bedrock::addon::BlockComponentEntityEvent event;
                            event.blockTypeId = typeId;
                            event.blockX = pos.x;
                            event.blockY = pos.y;
                            event.blockZ = pos.z;
                            event.dimensionId = m_world->dimension();
                            event.entitySourceId = id();
                            blockReg.dispatchEntity(typeId, event);
                        }

                        // 调用实体的"在方块内部"回调
                        onInsideBlock(*blockState);
                    }
                }
            }
        }
    }

    // MC Java: applyEffectsFromBlocks() - 方块碰撞后检查火焰免疫期
    // 如果实体不在燃烧，且方块碰撞没有增加火焰计时器，则设置火焰免疫期
    // 这防止实体刚离开火方块时被立即重新点燃
    bool fireTicksIncreased = getRemainingFireTicks() > fireTicksBeforeCollision;
    if (m_world != nullptr && !m_world->isClientSide() && !isOnFire() && !fireTicksIncreased) {
        setFireImmunityCooldown();
    }
}

void Entity::doBlockCollisionsAfterMove(const Vector3& actualMovement, const Vector3& desiredMovement)
{
    // 方块碰撞回调处理
    // 参考: Entity.java 行 610-616

    if (m_world == nullptr) {
        return;
    }

    // 获取实体脚下所在的方块位置
    // 使用碰撞箱底部的中心坐标
    BlockPos blockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.aabbShape->m_aabb.minY - 0.001f)), // 稍微向下偏移
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));

    // 派发自定义方块组件回调 - onStepOff
    // 检测实体是否离开了之前所站的方块
    BlockPos prevBlockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_posPrev.x)),
        static_cast<i32>(std::floor(m_builtIn.aabbShape->m_aabb.minY - 0.001f -
            (m_builtIn.stateVector->m_pos.y - m_builtIn.stateVector->m_posPrev.y))),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_posPrev.z)));
    if (prevBlockPos != blockPos) {
        const BlockState* prevBlockState = m_world->getBlockState(prevBlockPos);
        if (prevBlockState != nullptr && !prevBlockState->isAir()) {
            auto& blockReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
            std::string prevTypeId = prevBlockState->getBlock().blockLocation().toString();
            if (blockReg.hasStepOffCallback(prevTypeId)) {
                mc::mod::bedrock::addon::BlockComponentStepOffEvent event;
                event.blockTypeId = prevTypeId;
                event.blockX = prevBlockPos.x;
                event.blockY = prevBlockPos.y;
                event.blockZ = prevBlockPos.z;
                event.dimensionId = m_world->dimension();
                event.entityId = id();
                blockReg.dispatchStepOff(prevTypeId, event);
            }
        }
    }

    // 获取方块状态
    const BlockState* blockState = m_world->getBlockState(blockPos);
    if (blockState == nullptr) {
        return;
    }

    const Block& block = blockState->getBlock();

    // 1. onLanded 回调 - 当垂直位置发生变化时
    if (std::abs(desiredMovement.y - actualMovement.y) > 1.0e-7f) {
        // Y轴发生了碰撞，说明着陆了
        block.onLanded(*blockState, *m_world, blockPos, *this);

        // 派发自定义方块组件回调 - onEntityFallOn
        // 仅当实体有下落距离时才触发
        if (m_builtIn.physicsState->m_fallDistance > 0.0f) {
            auto& blockReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
            std::string typeId = block.blockLocation().toString();
            if (blockReg.hasEntityFallOnCallback(typeId)) {
                mc::mod::bedrock::addon::BlockComponentEntityFallOnEvent event;
                event.blockTypeId = typeId;
                event.blockX = blockPos.x;
                event.blockY = blockPos.y;
                event.blockZ = blockPos.z;
                event.dimensionId = m_world->dimension();
                event.entityId = id();
                event.fallDistance = m_builtIn.physicsState->m_fallDistance;
                blockReg.dispatchEntityFallOn(typeId, event);
            }
        }
    }

    // 2. onEntityWalk 回调 - 当在地面行走时
    if (m_builtIn.physicsState->m_onGround && !isSteppingCarefully()) {
        block.onEntityWalk(*blockState, *m_world, blockPos, *this);

        // 派发自定义方块组件回调 - onStepOn
        auto& blockReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
        std::string typeId = block.blockLocation().toString();
        if (blockReg.hasStepOnCallback(typeId)) {
            mc::mod::bedrock::addon::BlockComponentStepOnEvent event;
            event.blockTypeId = typeId;
            event.blockX = blockPos.x;
            event.blockY = blockPos.y;
            event.blockZ = blockPos.z;
            event.dimensionId = m_world->dimension();
            event.entityId = id();
            blockReg.dispatchStepOn(typeId, event);
        }
    }

    // 3. onInsideBlock 回调 - 遍历碰撞箱内所有方块
    AxisAlignedBB box = m_builtIn.aabbShape->m_aabb.shrink(0.001);
    BlockPos minPos(static_cast<i32>(std::floor(box.minX)),
        static_cast<i32>(std::floor(box.minY)),
        static_cast<i32>(std::floor(box.minZ)));
    BlockPos maxPos(static_cast<i32>(std::floor(box.maxX)),
        static_cast<i32>(std::floor(box.maxY)),
        static_cast<i32>(std::floor(box.maxZ)));

    for (i32 x = minPos.x; x <= maxPos.x; ++x) {
        for (i32 y = minPos.y; y <= maxPos.y; ++y) {
            for (i32 z = minPos.z; z <= maxPos.z; ++z) {
                BlockPos pos(x, y, z);
                const BlockState* insideState = m_world->getBlockState(pos);
                if (insideState != nullptr && !insideState->isAir()) {
                    // 进入方块回调
                    onInsideBlock(*insideState);
                }
            }
        }
    }
}

void Entity::applyPhysics(f32 /*deltaTime*/)
{
    // Entity 物理更新
    // 注意：重力应该始终应用（除非 noGravity），碰撞检测会处理停止

    // 重力始终应用（除非 noGravity 标志为 true）
    if (!hasNoGravity()) {
        m_builtIn.velocity->m_velocity.y -= physics::GRAVITY;
    }

    // 应用空气阻力
    m_builtIn.velocity->m_velocity.x *= physics::DRAG_AIR;
    m_builtIn.velocity->m_velocity.y *= physics::DRAG_AIR;
    m_builtIn.velocity->m_velocity.z *= physics::DRAG_AIR;

    // 注意：MC 物理是基于 tick 的，deltaTime 参数被忽略
}

// ============================================================================
// 乘客/骑乘系统
// ============================================================================

bool Entity::isPassenger(EntityInstanceId entityId) const
{
    for (EntityInstanceId passenger : m_passengers) {
        if (passenger == entityId) {
            return true;
        }
    }
    return false;
}

bool Entity::addPassenger(Entity& passenger)
{
    // addPassenger 不进行循环检测，仅操作乘客列表。
    // 循环检测由 startRiding() 负责，addPassenger 验证 passenger 已正确关联到此载具。
    // 前置条件：passenger.getVehicle() 应该已经指向 this（由 startRiding 在调用前设置）
    // 对齐 MC Java：当 passenger.getVehicle() != this 时抛出 IllegalStateException，
    // 因为 addPassenger 必须通过 startRiding 调用，而 startRiding 会在调用 addPassenger
    // 之前设置 passenger.vehicle = this。直接调用 addPassenger 是编程错误。

    // 检查是否已经是乘客
    if (isPassenger(passenger.id())) {
        return false;
    }

    // 验证 passenger 的 vehicle 已正确指向此载具
    // （由 startRiding 在调用 addPassenger 之前设置）
    // 对齐 MC Java Entity.addPassenger: passenger.getVehicle() != this 时抛出 IllegalStateException。
    // C++ 中使用 Debug 断言 + 日志 + 返回 false 的方式对齐此行为，
    // 而非直接终止程序（保持可恢复性）。
    if (passenger.getVehicle() != m_id) {
        MC_ASSERT_MSG(passenger.getVehicle() == m_id,
            "Entity::addPassenger: passenger's vehicle must already point to this entity. "
            "Use passenger.startRiding(vehicle) instead of vehicle.addPassenger(passenger).");
        spdlog::error("Entity::addPassenger: passenger (id={}) vehicle (id={}) does not match "
                      "expected vehicle (id={}). Use startRiding() instead.",
            passenger.id(),
            static_cast<i32>(passenger.getVehicle()),
            m_id);
        return false;
    }

    // 检查是否可以接受乘客（硬门槛）
    if (!couldAcceptPassenger()) {
        return false;
    }

    // 检查是否可以添加此特定乘客（软门槛）
    if (!canAddPassenger(passenger)) {
        return false;
    }

    // 设置乘客姿态为站立
    passenger.setPose(EntityPose::Standing);

    // 服务端玩家优先插入列表头部
    bool isServerSide = m_world && !m_world->isClientSide();
    bool isPlayer = passenger.entityType() == entity::VanillaEntityTypeKeys::PLAYER;
    EntityInstanceId controllingId = getControllingPassenger();
    bool controllingIsPlayer = false;
    if (controllingId != INVALID_ENTITY_ID && m_world) {
        Entity* controlling = m_world->getEntity(controllingId);
        controllingIsPlayer =
            controlling != nullptr && controlling->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
    }

    if (isServerSide && isPlayer && !controllingIsPlayer) {
        // 服务端：玩家优先插入头部，成为控制者
        m_passengers.insert(m_passengers.begin(), passenger.id());
    } else {
        // 其他情况：追加到末尾
        m_passengers.push_back(passenger.id());
    }

    // 对齐 vanilla：addPassenger 不设置骑乘冷却（vanilla Entity.addPassenger 无此机制）。
    // 原实现设 passenger.m_rideCooldown = 60（项目自定义防抖），但该冷却被 canBeRidden 用于
    // 阻止 startRiding，破坏了马驯服等需反复快速上下骑乘的核心玩法（RunAroundLikeCrazyGoal 甩人后
    // 玩家需立即重新骑上以继续累积 temper；60 tick 冷却使每次重新骑乘延迟 3 秒，驯服链路形同失效）。
    // m_rideCooldown 已整体移除（见 canBeRidden / baseTick / removePassenger），TODO 若未来确需
    // 防抖应在载具侧按 vanilla 的 mount/dismount 动画时序实现，而非全局骑乘冷却。

    // 触发回调
    // 子类可重写 onAddedPassenger() 来处理特殊逻辑

    // 服务端：乘客列表已变更，广播 SetPassengers 给追踪本载具的玩家，
    // 补齐客户端骑乘渲染同步（旧实现仅改服务端模型无网络发包，客户端看不到骑乘关系）。
    if (isServerSide) {
        m_world->broadcastPassengersChanged(m_id);
    }

    return true;
}

void Entity::removePassenger(Entity& passenger)
{
    // removePassenger 的职责是操作乘客列表。
    // dismount() 在调用 removePassenger 之前已经清空了 passenger 的 m_vehicle，
    // 然后从 passengers 列表中移除。
    // 对齐 MC Java Entity.removePassenger: passenger.getVehicle() == this 时抛出
    // IllegalStateException，因为 stopRiding/dismount 会先清空 m_vehicle 再调用
    // removePassenger。如果 vehicle 链接仍指向 this，说明调用顺序错误。

    // 查找并移除乘客
    auto it = std::find(m_passengers.begin(), m_passengers.end(), passenger.id());
    if (it != m_passengers.end()) {
        m_passengers.erase(it);

        // 验证 passenger 的 vehicle 引用已被 dismount 清空
        // 对齐 MC Java Entity.removePassenger: 如果 vehicle 仍指向 this，
        // 说明调用者未通过 stopRiding/dismount 正确下骑。
        // C++ 中使用 Debug 断言 + 安全清空 + 日志的方式对齐此行为。
        if (passenger.getVehicle() == m_id) {
            MC_ASSERT_MSG(passenger.getVehicle() != m_id,
                "Entity::removePassenger: passenger's vehicle should have been cleared by "
                "stopRiding/dismount before calling removePassenger. "
                "Use passenger.stopRiding() instead of vehicle.removePassenger(passenger).");
            spdlog::warn("Entity::removePassenger: passenger (id={}) vehicle still points to "
                         "this entity (id={}). Auto-clearing. Use stopRiding() instead.",
                passenger.id(),
                m_id);
            passenger.setVehicle(INVALID_ENTITY_ID);
        }

        // 对齐 vanilla：removePassenger 不设置骑乘冷却（vanilla Entity.removePassenger 无此机制）。
        // m_rideCooldown 已整体移除（详见 addPassenger 注释），此处不再设置。

        // 触发回调

        // 服务端：乘客列表已变更，广播 SetPassengers 给追踪本载具的玩家，
        // 补齐客户端骑乘渲染同步（旧实现仅改服务端模型无网络发包，客户端看不到骑乘关系）。
        if (m_world && !m_world->isClientSide()) {
            m_world->broadcastPassengersChanged(m_id);
        }
    }
}

bool Entity::startRiding(Entity& vehicle)
{
    // startRiding 的逻辑顺序：
    // 1. 不能骑乘自己
    // 2. 不能已在骑乘同一载具
    // 3. 硬门槛：couldAcceptPassenger()
    // 4. 循环检测
    // 5. 软门槛：canBeRidden() + canAddPassenger()
    // 6. 先 stopRiding 当前载具
    // 7. 先设置 vehicle，再调用 addPassenger

    // 1. 不能骑乘自己
    // 对齐 MC Java：原版 startRiding 通过 this.vehicle == p_19966_ 的对象引用比较
    // 间接拒绝自骑（vehicle 字段不可能指向自身），并未单独按 id 判定。
    // 本项目以 EntityInstanceId 关联，INVALID_ENTITY_ID == 0，新建实体 id 可能为 0，
    // 若按 id 比较则两个 id 均为 0 的不同实体会被误判为"自骑"而拒绝（反序列化
    // Passengers 时 vehicle 尚未 spawn、id 仍为 0 即触发此坑）。故改用对象地址比较，
    // 既精确拒绝真正的自骑，又消除 id==INVALID_ENTITY_ID 的歧义。
    if (&vehicle == this) {
        return false;
    }

    // 2. 检查是否已经在骑乘此载具（避免重复骑乘）
    // 对齐 MC Java: if (p_19966_ == this.vehicle) return false（this.vehicle == null 即未骑乘）。
    // 仅在确实处于骑乘状态（m_vehicle != INVALID_ENTITY_ID）时才判重，
    // 否则未骑乘的乘客（m_vehicle == INVALID_ENTITY_ID == 0）与 id 为 0 的载具
    // 会被 0==0 误判为"已在骑乘该载具"。
    if (isRiding() && getVehicle() == vehicle.id()) {
        return false;
    }

    // 3. 硬门槛：检查载具是否根本可以接受乘客
    // 对应 MC Java 的 Entity.couldAcceptPassenger()
    if (!vehicle.couldAcceptPassenger()) {
        return false;
    }

    // 4. 循环检测：从载具开始沿 vehicle 链向上遍历，
    // 检查是否形成 A骑B、B骑A 的循环
    if (m_world) {
        Entity* current = &vehicle;
        while (current != nullptr) {
            EntityInstanceId currentVehicle = current->getVehicle();
            if (currentVehicle == INVALID_ENTITY_ID) {
                break;
            }
            if (currentVehicle == m_id) {
                // 检测到循环：载具链中的某个实体正在骑乘我们
                return false;
            }
            current = m_world->getEntity(currentVehicle);
        }
    }

    // 5. 检查是否可骑乘（潜行状态、冷却等）
    // 对应 MC Java 的 canRide() + canAddPassenger()
    if (!canBeRidden(vehicle)) {
        return false;
    }

    // 6. 软门槛：检查载具是否可以添加此特定乘客
    // 对应 MC Java 的 Entity.canAddPassenger(Entity)
    if (!vehicle.canAddPassenger(*this)) {
        return false;
    }

    // 7. 如果已经在骑乘其他载具，先停止
    if (isRiding()) {
        stopRiding();
    }

    // 8. 设置姿态为站立
    setPose(EntityPose::Standing);

    // 9. 先设置 vehicle 引用，再调用 addPassenger
    // 此顺序保证 addPassenger 内部可以验证 passenger.getVehicle() == this
    m_vehicle = vehicle.id();

    // 10. 添加到载具的乘客列表
    if (!vehicle.addPassenger(*this)) {
        // 添加失败，回滚 vehicle 引用
        m_vehicle = INVALID_ENTITY_ID;
        return false;
    }

    return true;
}

void Entity::stopRiding()
{
    dismount();
}

void Entity::dismount()
{
    if (!isRiding()) {
        return;
    }

    // 获取车辆实体
    if (m_world) {
        Entity* vehicle = m_world->getEntity(m_vehicle);
        if (vehicle != nullptr) {
            // 注意：先清空vehicle引用，再调用removePassenger
            m_vehicle = INVALID_ENTITY_ID;
            vehicle->removePassenger(*this);
        }
    } else {
        m_vehicle = INVALID_ENTITY_ID;
    }
}

void Entity::removePassengers()
{
    // 没有世界引用时无法操作乘客
    if (m_world == nullptr) {
        return;
    }

    // 从后向前遍历，避免索引问题
    for (i32 i = static_cast<i32>(m_passengers.size()) - 1; i >= 0; --i) {
        EntityInstanceId passengerId = m_passengers[i];
        Entity* passenger = m_world->getEntity(passengerId);
        if (passenger != nullptr) {
            passenger->stopRiding();
        }
    }
}

bool Entity::canBeRidden(const Entity& vehicle) const
{
    // 对齐 vanilla：canBeRidden（对应 MC Java Entity.canRide）仅检查不处于潜行状态。
    // 原实现额外检查 m_rideCooldown <= 0（项目自定义骑乘冷却），但该冷却非 vanilla 机制且
    // 破坏了马驯服等反复骑乘玩法（详见 addPassenger 注释），已整体移除。
    MC_UNUSED(vehicle);
    return !isSneaking();
}

bool Entity::dismountsUnderwater() const
{
    // MC Java: return this.getType().is(EntityTypeTags.DISMOUNTS_UNDERWATER)
    // 通过实体类型标签判断此载具是否在水中强制乘客下坐骑。
    // 马、猪、骆驼等陆地骑乘实体返回 true，船不在标签中返回 false。
    // 数据包中的 dismounts_underwater.json 定义了完整的实体列表。
    return EntityTypeTags::DISMOUNTS_UNDERWATER().contains(getTypeId());
}

ProjectileDeflection Entity::deflection(const entity::ProjectileEntity& /*projectile*/) const
{
    // MC Java: Entity.deflection(Projectile)
    // 默认实现：如果实体类型属于 #minecraft:deflects_projectiles 标签则返回 Reverse，否则返回 None。
    // 子类可重写以自定义偏转行为（如 BreezeEntity 排除风弹）。
    return EntityTypeTags::DEFLECTS_PROJECTILES().contains(getTypeId()) ? ProjectileDeflection::Reverse
                                                                        : ProjectileDeflection::None;
}

bool Entity::isRidingSameEntity(const Entity& other) const
{
    return getLowestRidingEntity() == other.getLowestRidingEntity();
}

Entity* Entity::getLowestRidingEntity()
{
    Entity* entity = this;
    while (entity->isRiding()) {
        EntityInstanceId vehicleId = entity->getVehicle();
        if (vehicleId == INVALID_ENTITY_ID) {
            break;
        }
        // 使用当前实体的世界指针，而不是起始实体的
        IWorld* world = entity->world();
        if (world == nullptr) {
            break;
        }
        Entity* vehicle = world->getEntity(vehicleId);
        if (vehicle == nullptr) {
            break;
        }
        entity = vehicle;
    }
    return entity;
}

const Entity* Entity::getLowestRidingEntity() const
{
    const Entity* entity = this;
    while (entity->isRiding()) {
        EntityInstanceId vehicleId = entity->getVehicle();
        if (vehicleId == INVALID_ENTITY_ID) {
            break;
        }
        // 使用当前实体的世界指针，而不是起始实体的
        const IWorld* world = entity->world();
        if (world == nullptr) {
            break;
        }
        const Entity* vehicle = world->getEntity(vehicleId);
        if (vehicle == nullptr) {
            break;
        }
        entity = vehicle;
    }
    return entity;
}

bool Entity::isRidingOrBeingRiddenBy(const Entity& other) const
{
    // 双向检查骑乘关系：
    // 1. 向下：检查 other 是否是 this 的（间接）乘客
    // 2. 向上：检查 other 是否是 this 的（间接）载具
    if (m_world == nullptr) {
        return false;
    }

    // 快速路径：直接检查
    if (m_id == other.id()) {
        return true;
    }

    // 向下搜索：检查 other 是否是 this 的间接乘客
    // （other 骑乘 this，或 other 骑乘 this 的某个乘客，以此类推）
    {
        std::vector<EntityInstanceId> toCheck(m_passengers.begin(), m_passengers.end());
        std::unordered_set<EntityInstanceId> visited;

        while (!toCheck.empty()) {
            EntityInstanceId passengerId = toCheck.back();
            toCheck.pop_back();

            if (passengerId == other.id()) {
                return true;
            }

            if (visited.count(passengerId) > 0) {
                continue;
            }
            visited.insert(passengerId);

            const Entity* passenger = m_world->getEntity(passengerId);
            if (passenger != nullptr) {
                const auto& subPassengers = passenger->getPassengers();
                toCheck.insert(toCheck.end(), subPassengers.begin(), subPassengers.end());
            }
        }
    }

    // 向上搜索：检查 other 是否是 this 的间接载具
    // （this 骑乘 other，或 this 的载具骑乘 other，以此类推）
    {
        EntityInstanceId currentVehicle = m_vehicle;
        std::unordered_set<EntityInstanceId> visited;

        while (currentVehicle != INVALID_ENTITY_ID) {
            if (currentVehicle == other.id()) {
                return true;
            }

            if (visited.count(currentVehicle) > 0) {
                break; // 防止循环
            }
            visited.insert(currentVehicle);

            const Entity* vehicleEntity = m_world->getEntity(currentVehicle);
            if (vehicleEntity == nullptr) {
                break;
            }
            currentVehicle = vehicleEntity->getVehicle();
        }
    }

    return false;
}

void Entity::detach()
{
    // 先移除所有乘客，再下车
    if (isBeingRidden()) {
        removePassengers();
    }
    if (isRiding()) {
        stopRiding();
    }
}

f64 Entity::getMountedYOffset() const
{
    return static_cast<f64>(height()) * 0.75;
}

Vector3 Entity::getRidingPosition() const
{
    // 默认骑乘位置在实体顶部中心
    return Vector3(m_builtIn.stateVector->m_pos.x,
        m_builtIn.stateVector->m_pos.y + static_cast<f32>(getMountedYOffset()),
        m_builtIn.stateVector->m_pos.z);
}

void Entity::updatePassengers()
{
    // 效率优化：提前检查世界指针，避免在循环内重复检查
    if (m_world == nullptr) {
        return;
    }

    // 遍历所有乘客并更新位置
    for (EntityInstanceId passengerId : m_passengers) {
        Entity* passenger = m_world->getEntity(passengerId);
        if (passenger == nullptr) {
            continue;
        }

        // 更新单个乘客位置
        positionRider(*passenger);
    }
}

void Entity::positionRider(Entity& passenger)
{
    if (!isPassenger(passenger.id())) {
        return;
    }

    // 计算骑乘位置
    f64 y = static_cast<f64>(m_builtIn.stateVector->m_pos.y) + getMountedYOffset() + passenger.getYOffset();

    // 设置乘客位置
    passenger.setPosition(m_builtIn.stateVector->m_pos.x, static_cast<f32>(y), m_builtIn.stateVector->m_pos.z);
}

void Entity::updateRidden()
{
    // 设置速度为零
    setVelocity(Vector3(0.0f, 0.0f, 0.0f));

    // 执行tick
    if (canUpdate()) {
        tick();
    }

    // 如果还在骑乘，更新位置
    if (isRiding()) {
        Entity* vehicle = m_world ? m_world->getEntity(m_vehicle) : nullptr;
        if (vehicle != nullptr) {
            vehicle->updatePassengerPosition(*this);
        }
    }
}

void Entity::updatePassengerPosition(Entity& passenger)
{
    positionRider(passenger);
}

void Entity::applyOrientationToEntity(Entity& passenger)
{
    // 默认实现：同步旋转
    // 子类（如BoatEntity）可以重写此方法以限制旋转范围
    passenger.setRotation(m_builtIn.rotation->m_rot.x, passenger.pitch());
}

bool Entity::canPassengerSteer() const
{
    EntityInstanceId controllerId = getControllingPassenger();
    if (controllerId == INVALID_ENTITY_ID) {
        return false;
    }

    // 如果有世界引用，获取控制者并检查
    if (m_world) {
        Entity* controller = m_world->getEntity(controllerId);
        if (controller != nullptr) {
            // 检查控制者是否是玩家
            if (controller->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
                // 玩家需要检查是否是本地玩家
                Player* player = dynamic_cast<Player*>(controller);
                if (player != nullptr) {
                    return player->isLocalPlayer();
                }
            }
            // 非玩家实体：服务端可以控制，客户端不能
            return !m_world->isClientSide();
        }
    }

    return false;
}

bool Entity::canSee(const Entity& other) const
{
    // 检查目标是否存活
    if (!other.isAlive()) {
        return false;
    }

    // 计算到目标的距离
    f32 distSq = distanceSqTo(other);

    // 如果距离超过视线范围（64格），返回false
    constexpr f32 SIGHT_RANGE_SQ = 64.0f * 64.0f;
    if (distSq > SIGHT_RANGE_SQ) {
        return false;
    }

    // 检查世界是否存在
    if (!m_world) {
        return false;
    }

    // 使用射线检测检查视线是否被方块阻挡
    Vector3 eyePos = Vector3(x(), static_cast<f32>(getEyeY()), z());
    Vector3 targetEyePos = Vector3(other.x(), static_cast<f32>(other.getEyeY()), other.z());

    // 计算射线方向和距离
    Vector3 direction = targetEyePos - eyePos;
    f32 distance = direction.length();
    direction = direction.normalized();

    // 创建射线检测上下文
    Ray ray(eyePos, direction);
    RaycastContext context(ray, distance);

    // 执行射线检测
    BlockRaycastResult result = raycastBlocks(context, *m_world);

    // 如果射线没有击中方块（或击中点超出目标位置），则表示可见
    return result.isMiss();
}

// ============================================================================
// 闪电击中处理
// ============================================================================

void Entity::onStruckByLightning(entity::LightningBoltEntity* lightning)
{
    // 复刻 vanilla Entity#thunderHit（Entity.java:2725-2732）：
    //   setRemainingFireTicks(remainingFireTicks + 1);
    //   if (remainingFireTicks == 0) igniteForSeconds(8.0F);
    //   hurtServer(level, lightningBolt(), 5.0F);
    // 基类默认：引燃判定 + 5 伤害。子类重写决定是否调本基类（对齐 vanilla 各实体 thunderHit 语义）。
    if (lightning == nullptr) {
        return;
    }

    // 1. 火焰计时器 +1。普通实体（0）→1 不引燃；刚灭火免疫期（-1）→0 才触发引燃。
    //    故 JE 闪电直接伤害通常不引燃（引燃来自闪电生成的火方块），与 wiki tech_闪电束.txt 一致。
    setRemainingFireTicks(getRemainingFireTicks() + 1);

    // 2. 若 +1 后归零（刚灭火免疫期边界），引燃 8 秒。
    if (getRemainingFireTicks() == 0) {
        igniteForSeconds(8.0f);
    }

    // 3. 闪电伤害 5.0。基类 Entity::hurt 对非 LivingEntity 返回 false（不生效），
    //    LivingEntity 重写实际减血。伤害源为闪电实体（DamageSources::lightningBolt(lightning)）。
    auto damageSource = DamageSources::lightningBolt(lightning);
    hurt(damageSource, 5.0f);
}

// ============================================================================
// 伤害处理
// ============================================================================

bool Entity::hurt(DamageSource& source, f32 amount)
{
    // 基类实现：检查无敌状态
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 默认实现：简单标记为已移除（非生物实体的默认行为）
    // 生物实体（LivingEntity）会重写此方法实现更复杂的伤害逻辑
    MC_UNUSED(source);
    MC_UNUSED(amount);
    return false;
}

bool Entity::isInvulnerableTo(DamageSource& source) const
{
    // 0. 已移除实体对所有伤害免疫（对齐 vanilla Entity.isInvulnerableToBase:2919 首项 isRemoved()）。
    //    remove()/discard() 标记 m_removed=true 后，实体将在本 tick 末从世界移除；此窗口内
    //    （僵尸窗口，见 world/entity/README.md:143）若被 hurt 应免疫，避免对正在清理的实体
    //    施加伤害（重复死亡链路/UAF）。vanilla 把此守卫放在 isInvulnerableToBase final 首项
    //    做最底层兜底；Cubium 此前仅在遍历层过滤，hurt 入口缺兜底，新增 hurt 调用点若忘查
    //    isAlive 即穿透。此处补齐对齐 vanilla。
    if (m_removed) {
        return true;
    }
    // 1. 检查实体是否处于无敌状态
    if (m_invulnerable) {
        // 虚空伤害和创造模式玩家可以绕过无敌
        return !source.bypassesInvulnerability();
    }
    return false;
}

bool Entity::isImmuneToFire() const
{
    // 默认实现：查询实体类型的火焰免疫标志
    // 子类可以重写此方法提供运行时可变的免疫状态
    const std::string typeId = getTypeId();
    const entity::EntityType* type = entity::EntityRegistry::instance().getType(typeId);
    if (type != nullptr) {
        return type->immuneToFire();
    }
    return false;
}

void Entity::lavaIgnite()
{
    if (!isImmuneToFire()) {
        igniteForSeconds(15.0f); // 15 秒 = 300 ticks
    }
}

void Entity::lavaHurt()
{
    if (isImmuneToFire()) {
        return;
    }

    // 仅在服务端处理伤害和音效
    if (m_world != nullptr && !m_world->isClientSide()) {
        auto damageSource = DamageSources::lava();
        if (hurt(damageSource, 4.0f)) {
            if (shouldPlayLavaHurtSound() && !isSilent()) {
                playSound(SoundEvents::ENTITY_GENERIC_BURN, 0.4f, 2.0f + getRandom().nextFloat() * 0.4f);
            }
        }
    }
}

void Entity::clearFire()
{
    // MC Java: setRemainingFireTicks(Math.min(0, getRemainingFireTicks()))
    // 保留负值（火焰免疫期倒计时），仅将正值清零
    if (getRemainingFireTicks() > 0) {
        setRemainingFireTicks(0);
    }
}

void Entity::extinguishFire()
{
    // MC Java: extinguishFire()
    // 如果实体正在燃烧，先播放灭火音效，然后清除火焰
    if (isOnFire()) {
        playExtinguishSound();
    }
    clearFire();
}

void Entity::playExtinguishSound()
{
    playSound(SoundEvents::ENTITY_GENERIC_EXTINGUISH_FIRE,
        0.7f,
        1.6f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.4f);
}

void Entity::setFireImmunityCooldown()
{
    // MC Java: applyEffectsFromBlocks() 中，当实体火焰被方块碰撞系统熄灭时，
    // 设置火焰免疫期倒计时为 -getFireImmuneTicks()。
    // 基类 getFireImmuneTicks() 返回 0（无免疫期），Player 重写返回 20（1 秒）。
    i32 immuneTicks = getFireImmuneTicks();
    if (immuneTicks > 0) {
        setRemainingFireTicks(-immuneTicks);
    }
}

std::string Entity::toString() const
{
    std::stringstream ss;
    ss << "Entity{id=" << m_id << ", type=" << getTypeId() << ", uuid=" << m_uuid << ", position=("
       << m_builtIn.stateVector->m_pos.x << ", " << m_builtIn.stateVector->m_pos.y << ", "
       << m_builtIn.stateVector->m_pos.z << ")"
       << ", velocity=(" << m_builtIn.velocity->m_velocity.x << ", " << m_builtIn.velocity->m_velocity.y << ", "
       << m_builtIn.velocity->m_velocity.z << ")"
       << ", onGround=" << m_builtIn.physicsState->m_onGround << ", inWater=" << m_inWater << ", inLava=" << m_inLava
       << ", flags=" << static_cast<u32>(flags()) << ", air=" << air() << ", customName=\"" << customNameText() << "\""
       << ", customNameVisible=" << isCustomNameVisible() << ", silent=" << isSilent()
       << ", noGravity=" << hasNoGravity() << ", pose=" << static_cast<u32>(pose()) << "}";
    return ss.str();
}

// ============================================================================
// 随机传送
// ============================================================================

bool Entity::attemptTeleport(f64 x, f64 y, f64 z, bool playEffects)
{
    // 保存当前位置作为备份
    f64 originalX = m_builtIn.stateVector->m_pos.x;
    f64 originalY = m_builtIn.stateVector->m_pos.y;
    f64 originalZ = m_builtIn.stateVector->m_pos.z;

    // 如果正在骑乘，先下坐骑
    if (isRiding()) {
        stopRiding();
    }

    // 查找安全的传送位置
    auto safePos = findSafeTeleportPosition(x, y, z, true);
    if (!safePos.has_value()) {
        return false;
    }

    // 传送到安全位置
    setPosition(static_cast<f32>(safePos->x), static_cast<f32>(safePos->y), static_cast<f32>(safePos->z));

    // 检查传送后位置是否有碰撞或液体
    if (m_world != nullptr) {
        AxisAlignedBB box = boundingBox();

        // 检查碰撞
        if (m_world->hasBlockCollision(box)) {
            // 碰撞检测失败，恢复原位置
            setPosition(static_cast<f32>(originalX), static_cast<f32>(originalY), static_cast<f32>(originalZ));
            return false;
        }

        // 检查是否在液体中
        if (m_world->hasFluid(
                static_cast<i32>(safePos->x), static_cast<i32>(safePos->y), static_cast<i32>(safePos->z))) {
            // 在液体中，恢复原位置
            setPosition(static_cast<f32>(originalX), static_cast<f32>(originalY), static_cast<f32>(originalZ));
            return false;
        }
    }

    // 传送成功
    if (playEffects) {
        // 播放传送粒子效果
        // 客户端收到后会播放末影人传送粒子
        if (m_world != nullptr) {
            m_world->broadcastEntityStatus(m_id, static_cast<u8>(network::EntityStatus::TeleportParticles));
        }

        // 播放传送音效
        playSound(SoundEvents::ENTITY_ENDERMAN_TELEPORT, 1.0f, 1.0f);
    }

    return true;
}

bool Entity::randomTeleport(f64 range, bool playEffects, bool avoidFluid)
{
    if (m_world == nullptr) {
        return false;
    }

    // 创建随机数生成器
    math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_ticksExisted) ^
        static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    // 记录原位置用于音效
    f64 originalX = m_builtIn.stateVector->m_pos.x;
    f64 originalY = m_builtIn.stateVector->m_pos.y;
    f64 originalZ = m_builtIn.stateVector->m_pos.z;

    // 尝试最多 16 次传送
    constexpr i32 MAX_ATTEMPTS = 16;

    for (i32 i = 0; i < MAX_ATTEMPTS; ++i) {
        // 计算目标位置
        // X/Z: 原位置 ± range
        // Y: 原位置 ± range，但限制在世界高度范围内
        f64 targetX = originalX + (rng.nextDouble() - 0.5) * range * 2.0;
        f64 targetY = originalY + static_cast<f64>(rng.nextInt(static_cast<i32>(range * 2)) - static_cast<i32>(range));
        f64 targetZ = originalZ + (rng.nextDouble() - 0.5) * range * 2.0;

        // 限制 Y 在世界范围内
        targetY =
            math::clamp(targetY, static_cast<f64>(world::MIN_BUILD_HEIGHT), static_cast<f64>(world::MAX_BUILD_HEIGHT));

        // 尝试传送
        if (attemptTeleport(targetX, targetY, targetZ, false)) {
            // 传送成功，播放音效
            if (playEffects) {
                playSound(SoundEvents::ENTITY_ENDERMAN_TELEPORT, 1.0f, 1.0f);

                // 在原位置也播放音效
                if (m_world != nullptr) {
                    m_world->playSound(SoundEvents::ENTITY_ENDERMAN_TELEPORT,
                        sound::SoundCategory::Neutral,
                        Vector3(static_cast<f32>(originalX), static_cast<f32>(originalY), static_cast<f32>(originalZ)),
                        1.0f,
                        1.0f);
                }
            }
            return true;
        }
    }

    // 所有尝试都失败
    return false;
}

std::optional<Vector3d> Entity::findSafeTeleportPosition(f64 x, f64 y, f64 z, bool avoidFluid) const
{
    if (m_world == nullptr) {
        return std::nullopt;
    }

    BlockPos blockPos(
        static_cast<i32>(std::floor(x)), static_cast<i32>(std::floor(y)), static_cast<i32>(std::floor(z)));

    // 检查区块是否加载（简化实现：检查目标区块是否可用）
    ChunkCoord chunkX = world::toChunkCoord(blockPos.x);
    ChunkCoord chunkZ = world::toChunkCoord(blockPos.z);
    const ChunkData* chunk = m_world->getChunk(chunkX, chunkZ);
    if (chunk == nullptr) {
        return std::nullopt;
    }

    // 从目标位置向下寻找固体地面
    f64 adjustedY = y;
    bool foundGround = false;

    while (blockPos.y > world::MIN_BUILD_HEIGHT && !foundGround) {
        BlockPos belowPos = blockPos.down();
        const BlockState* belowState = m_world->getBlockState(belowPos);

        if (belowState != nullptr && belowState->blocksMovement()) {
            // 找到固体地面
            foundGround = true;
            adjustedY = static_cast<f64>(belowPos.y + 1);
        } else {
            --blockPos.y;
            adjustedY = static_cast<f64>(blockPos.y);
        }
    }

    if (!foundGround) {
        return std::nullopt;
    }

    // 检查找到的位置是否安全
    if (isSafeTeleportPosition(x, adjustedY, z, avoidFluid)) {
        return Vector3d(x, adjustedY, z);
    }

    return std::nullopt;
}

bool Entity::isSafeTeleportPosition(f64 x, f64 y, f64 z, bool avoidFluid) const
{
    if (m_world == nullptr) {
        return false;
    }

    // 创建实体的碰撞箱
    f32 halfWidth = width() / 2.0f;
    AxisAlignedBB box(static_cast<f32>(x - halfWidth),
        static_cast<f32>(y),
        static_cast<f32>(z - halfWidth),
        static_cast<f32>(x + halfWidth),
        static_cast<f32>(y + height()),
        static_cast<f32>(z + halfWidth));

    // 检查碰撞
    if (m_world->hasBlockCollision(box)) {
        return false;
    }

    // 检查液体
    if (avoidFluid) {
        // 检查脚底和身体是否在液体中
        BlockPos pos(static_cast<i32>(std::floor(x)), static_cast<i32>(std::floor(y)), static_cast<i32>(std::floor(z)));

        if (m_world->isWaterAt(pos.x, pos.y, pos.z) || m_world->isLavaAt(pos.x, pos.y, pos.z)) {
            return false;
        }

        // 检查上半身
        BlockPos posUp = pos.up();
        if (m_world->isWaterAt(posUp.x, posUp.y, posUp.z) || m_world->isLavaAt(posUp.x, posUp.y, posUp.z)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// 队伍相关
// ============================================================================

bool Entity::isOnScoreboardTeam(const scoreboard::Team* team) const
{
    const scoreboard::Team* myTeam = getTeam();
    if (myTeam == nullptr || team == nullptr) {
        return false;
    }
    // 通过对象引用相等性判断
    return myTeam == team;
}

bool Entity::isAlliedTo(const Entity& other) const
{
    // 自身视为盟友
    if (this == &other) {
        return true;
    }
    // 双向检查：this 认为 other 是盟友，或 other 认为 this 是盟友
    return considersEntityAsAlly(other) || other.considersEntityAsAlly(*this);
}

bool Entity::isAlliedTo(const scoreboard::Team* team) const
{
    const scoreboard::Team* myTeam = getTeam();
    if (myTeam == nullptr || team == nullptr) {
        return false;
    }
    return myTeam == team;
}

bool Entity::considersEntityAsAlly(const Entity& other) const
{
    return isAlliedTo(other.getTeam());
}

bool Entity::isOnSameTeam(const Entity& other) const
{
    return isOnScoreboardTeam(other.getTeam());
}

// ============================================================================
// 玩家交互
// ============================================================================

ActionResultType Entity::processInitialInteract(Player& player, Hand hand)
{
    // 基类默认实现：返回 Pass，表示不处理交互
    // 子类（如 MobEntity、BoatEntity、ItemFrameEntity 等）可重写此方法处理特定交互
    (void)player;
    (void)hand;
    return ActionResultType::Pass;
}

ActionResultType Entity::applyPlayerInteraction(Player& player, const Vector3& hitPosition, Hand hand)
{
    // 基类默认实现：直接调用 processInitialInteract
    // 子类（如 ArmorStandEntity）可重写此方法处理基于点击位置的交互
    (void)hitPosition; // 基类不使用点击位置
    return processInitialInteract(player, hand);
}

// ============================================================================
// NBT 序列化
// ============================================================================

void Entity::writeToNBT(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // UUID (UUIDMost/UUIDLeast) — 纯 OOP 基类字段，保留直写
    nbt_helper::putUuid(tag, m_uuid);

    // 无敌标记 (byte 0/1) — 纯 OOP 基类字段，保留直写
    tag.put(nbt_keys::INVULNERABLE, static_cast<i8>(m_invulnerable ? 1 : 0));

    // 发光 (byte 0/1) — 纯 OOP 基类字段（m_glowing 未组件化），保留直写
    tag.put(nbt_keys::GLOWING, static_cast<i8>(m_glowing ? 1 : 0));

    // Tags (字符串列表) — 纯 OOP 基类字段，保留直写
    if (!m_tags.empty()) {
        auto tagsList = std::make_unique<nbt::tags::string_list_tag>();
        for (const auto& tagStr : m_tags) {
            tagsList->value.push_back(tagStr);
        }
        tag.value.emplace(nbt_keys::TAGS, std::move(tagsList));
    }

    // 已组件化字段（Pos/Motion/Rotation/FallDistance/Fire/Air/OnGround/PortalCooldown/
    // TicksFrozen/CustomName/CustomNameVisible/Silent/NoGravity/FallFlying 共 14 字段，
    // 9 个序列化器对）经组件序列化器注册表写出。批次6 子目标1。
    components::ComponentSerializerRegistry::instance().saveAll(*this, tag);

    // 调用子类特有数据序列化（剩余纯 OOP 字段：LivingEntity 的 HurtByTimestamp/
    // ActiveEffects/Attributes；Player 的 GameMode/Food/XP/Inventory/...；MobEntity 全层）
    addAdditionalSaveData(tag);

    // Passengers (乘客列表)
    // 参考: net.minecraft.world.entity.Entity.saveWithoutId → 若 isVehicle 则写 Passengers 列表。
    // 每个 passenger 通过 saveAsPassenger 递归保存：写 "id" 后调用 saveWithoutId，
    // 从而支持多层骑乘（Boat → Zombie → BabyZombie）的嵌套序列化。
    // 注意：passenger 的 Pos 取 vehicle 的 x/z 与 passenger 的 y（MC Java 行为），
    // 此处仅保存 passenger 的原始位置，反序列化时由 readFromNBT 直接读取。
    if (hasPassengers() && m_world != nullptr) {
        auto passengersList = std::make_unique<nbt::tags::compound_list_tag>();
        for (EntityInstanceId passengerId : m_passengers) {
            Entity* passenger = m_world->getEntity(passengerId);
            if (passenger == nullptr) {
                continue;
            }

            nbt::tags::compound_tag passengerTag;
            passengerTag.put(nbt_keys::ID, passenger->getTypeId());
            passenger->writeToNBT(passengerTag);
            passengersList->value.push_back(std::move(passengerTag));
        }

        if (!passengersList->value.empty()) {
            tag.value.emplace(nbt_keys::PASSENGERS, std::move(passengersList));
        }
    }
}

Result<void> Entity::readFromNBT(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    // UUID — 纯 OOP 基类字段，保留直读
    std::string uuid = nbt_helper::getUuid(tag);
    if (!uuid.empty()) {
        m_uuid = uuid;
    }

    // 无敌 — 纯 OOP 基类字段，保留直读
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::INVULNERABLE)) {
        m_invulnerable = *val;
    }

    // 发光 — 纯 OOP 基类字段（m_glowing 未组件化），保留直读
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::GLOWING)) {
        m_glowing = *val;
    }

    // Tags — 纯 OOP 基类字段，保留直读
    if (auto* tagsList = nbt_helper::tryGetList(tag, nbt_keys::TAGS)) {
        if (tagsList->element_id() == nbt::TagId::String) {
            auto& stringList = dynamic_cast<const nbt::tags::string_list_tag&>(*tagsList);
            m_tags.clear();
            for (const auto& tagStr : stringList.value) {
                m_tags.insert(tagStr);
            }
        }
    }

    // 已组件化字段（Pos/Motion/Rotation/FallDistance/Fire/Air/OnGround/PortalCooldown/
    // TicksFrozen/CustomName/CustomNameVisible/Silent/NoGravity/FallFlying 共 14 字段）
    // 经组件序列化器注册表读回。序列化器经 tryGetComponent 直写 Pos/Rotation/OnGround 组件，
    // 与原直写 m_builtIn.* 语义一致。批次6 子目标1。
    MC_TRY(components::ComponentSerializerRegistry::instance().loadAll(*this, tag));

    // 更新碰撞箱（Pos 经 loadAll 直写组件后重建 AABB）
    reapplyPosition();

    // 调用子类特有数据反序列化（剩余纯 OOP 字段）
    return readAdditionalSaveData(tag);
}

void Entity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 默认空实现，子类按需重写
    (void)tag;
}

Result<void> Entity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 默认空实现，子类按需重写
    (void)tag;
    return Result<void>::ok();
}

bool Entity::canFreeze() const
{
    // 基类实现：检查实体类型是否不在冰冻免疫标签中
    // 免疫实体类型：流浪者、北极熊、雪傀儡、凋灵
    // 安全检查：如果 EntityTypeTags 尚未初始化，默认允许冰冻
    if (!EntityTypeTags::isInitialized()) {
        return true;
    }
    return !EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().contains(getTypeId());
}

} // namespace mc
