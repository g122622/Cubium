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

#include "ProjectileEntity.hpp"

#include "ProjectileHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/ecs/components/ProjectileOwnerComponent.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileDeflection.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/ray/Ray.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/gamerule/GameRules.hpp"

#include <cmath>
#include <string>

namespace mc {
namespace entity {

namespace {

// 辅助函数：基于实体ID和tick创建随机数生成器
math::Random createRandomFromEntity(const Entity& entity)
{
    // 使用实体ID和存活时间作为种子
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

} // anonymous namespace

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = Entity::classInfo()）。
// 本类无同步字段，classInfo 仅作父链遍历节点：子类 ClassRegisterGuard 沿父链查找最高 id
// 时穿过本类（lastAssignedId=-1）直达父链已分配 id 的基类，子类首字段续接其后。
const EntityClassInfo& ProjectileEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"ProjectileEntity", &Entity::classInfo()};
    return s_classInfo;
}

ProjectileEntity::ProjectileEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : Entity(id, nullptr, registry)
{
    // 批次6 子目标2 Step1：attach ProjectileOwnerComponent（发射者追踪组件）。
    // 所有 ProjectileEntity 子树（含 Throwable/DamagingProjectile/AbstractArrow/
    // ShulkerBullet/FireworkRocket 支系）经此自动获得 owner 组件。Step2 将把
    // m_shooterUuid/m_shooterEntityId/m_leftShooter/m_lastDeflectedById 读写改走组件。
    // FishingBobber/EvokerFangs/EyeOfEnder 直接继承 Entity 不经本类，各自独立 owner。
    m_entityContext->enttRegistry().emplace<ecs::ProjectileOwnerComponent>(m_entityContext->entity());
}

bool ProjectileEntity::hurt(DamageSource& source, f32 /*amount*/)
{
    // 投掷物不可被伤害，但当来源非无敌时标记 hurtMarked 以同步速度到客户端。
    // 这使得投掷物在被击中时会产生击退效果（如恶魂火球被反射时的速度同步）。
    if (!isInvulnerableTo(source)) {
        markHurt();
    }
    return false;
}

void ProjectileEntity::tick()
{
    tryUpdateLeftShooter();

    const RayTraceResult result = performRayTrace();
    if (result.type != RayTraceResultType::Miss) {
        onImpact(result);
        if (isRemoved()) {
            Entity::tick();
            return;
        }
    }

    Vector3 velocity = m_builtIn.velocity->m_velocity;
    if (isInWater()) {
        // 水中阻力（子类可重写 getWaterDrag()）
        // 水中气泡粒子由子类（ThrowableEntity、AbstractArrowEntity 等）自行处理
        velocity = velocity * getWaterDrag();
    } else {
        velocity = velocity * getAirDrag();
    }

    if (!hasNoGravity()) {
        velocity.y -= getGravity();
    }

    m_builtIn.velocity->m_velocity = velocity;
    m_builtIn.stateVector->m_posPrev = m_builtIn.stateVector->m_pos;
    m_builtIn.stateVector->m_pos = m_builtIn.stateVector->m_pos + velocity;

    updateRotation();
    Entity::tick();
}

Entity* ProjectileEntity::getShooter() const
{
    auto* owner = tryGetComponent<ecs::ProjectileOwnerComponent>();
    if (owner == nullptr || m_world == nullptr || owner->m_shooterEntityId == INVALID_ENTITY_ID) {
        return nullptr;
    }

    return m_world->getEntity(owner->m_shooterEntityId);
}

void ProjectileEntity::setShooter(Entity* shooter)
{
    auto* owner = tryGetComponent<ecs::ProjectileOwnerComponent>();
    MC_ASSERT_RELEASE(owner != nullptr); // attach 是硬约束，ProjectileEntity 构造已 attach
    if (owner == nullptr) {
        return; // 防御：Release 断言被剥离时仍不崩
    }
    if (shooter == nullptr) {
        owner->m_shooterUuid.clear();
        owner->m_shooterEntityId = INVALID_ENTITY_ID;
        return;
    }

    owner->m_shooterUuid = shooter->uuid();
    owner->m_shooterEntityId = shooter->id();
}

void ProjectileEntity::shoot(f32 x, f32 y, f32 z, f32 velocity, f32 inaccuracy)
{
    f32 length = std::sqrt(x * x + y * y + z * z);
    if (length > 0.0f) {
        x /= length;
        y /= length;
        z /= length;
    }

    // 高斯随机散布
    // 取绝对值以支持负 inaccuracy：MC 原版部分实体（如旋风人风弹）在普通/困难难度下
    // 会传入负的 inaccuracy（公式 5 - difficulty*4），但由于分布的对称性，
    // 负值与同绝对值的正值产生相同的散布效果。使用 abs 确保行为与 MC 原版一致。
    const f32 absInaccuracy = std::abs(inaccuracy);
    if (absInaccuracy > 0.0f) {
        math::Random rng = createRandomFromEntity(*this);
        f32 gaussianX = static_cast<f32>(rng.nextGaussian()) * 0.0075f * absInaccuracy;
        f32 gaussianY = static_cast<f32>(rng.nextGaussian()) * 0.0075f * absInaccuracy;
        f32 gaussianZ = static_cast<f32>(rng.nextGaussian()) * 0.0075f * absInaccuracy;
        x += gaussianX;
        y += gaussianY;
        z += gaussianZ;
    }

    m_builtIn.velocity->m_velocity = Vector3(x * velocity, y * velocity, z * velocity);

    const f32 horizontalLength = std::sqrt(x * x + z * z);
    m_builtIn.rotation->m_rot.x = std::atan2(x, z) * math::RAD_TO_DEG;
    m_builtIn.rotation->m_rot.y = std::atan2(y, horizontalLength) * math::RAD_TO_DEG;
    m_builtIn.rotation->m_rotPrev.x = m_builtIn.rotation->m_rot.x;
    m_builtIn.rotation->m_rotPrev.y = m_builtIn.rotation->m_rot.y;
}

void ProjectileEntity::shootFrom(Entity& shooter, f32 pitch, f32 yaw, f32 pitchOffset, f32 velocity, f32 inaccuracy)
{
    const f32 radPitch = pitch * math::DEG_TO_RAD;
    const f32 radYaw = yaw * math::DEG_TO_RAD;
    const f32 radOffset = pitchOffset * math::DEG_TO_RAD;

    const f32 cosYaw = std::cos(radYaw);
    const f32 sinYaw = std::sin(radYaw);
    const f32 cosPitch = std::cos(radPitch);
    const f32 sinPitch = std::sin(radPitch);
    const f32 sinOffset = std::sin(radOffset);

    // 计算方向向量
    const f32 x = -sinYaw * cosPitch;
    const f32 y = -sinPitch - sinOffset;
    const f32 z = cosYaw * cosPitch;

    shoot(x, y, z, velocity, inaccuracy);

    // 添加发射者速度
    const Vector3 shooterVelocity = shooter.velocity();
    if (!shooter.onGround()) {
        m_builtIn.velocity->m_velocity.y += shooterVelocity.y;
    }
    m_builtIn.velocity->m_velocity.x += shooterVelocity.x;
    m_builtIn.velocity->m_velocity.z += shooterVelocity.z;
}

bool ProjectileEntity::canHitEntity(const mc::Entity& target) const
{
    // 对应 MC Java Projectile.canHitEntity:
    // 首先检查目标是否可被弹射物命中（综合判断存活状态、碰撞箱可交互性、旁观者模式等）
    const bool canBeHit = target.canBeHitByProjectile();
    if (!canBeHit) {
        return false;
    }

    // 无条件排除发射者自身及与其骑乘同一载具的实体。
    // 对应 MC Java Projectile.getEntityHitResult 的命中谓词：
    //   entity -> !entity.isSpectator() && entity.isPickable() && entity != this.getOwner()
    // 射线追踪对发射者自身无条件排除（不依赖 leftOwner 状态）。
    //
    // 此前仅判断 isRidingSameEntity 且加了 hasLeftShooter 条件——当羊驼贴脸吐口水时，
    // 口水生成点位于羊驼前方约 0.95 格（_spit 偏移），首 tick tryUpdateLeftShooter 即判定
    // 碰撞箱不相交而 leftShooter=true，致排除逻辑被跳过；随后 rayTraceEntities 的 searchBox
    // 覆盖到羊驼碰撞箱，口水命中发射者自身（24 次全命中羊驼、0 次命中狼）。
    // 修正为无条件排除，与 vanilla 射线追踪谓词一致。
    const Entity* shooter = getShooter();
    if (shooter != nullptr) {
        if (&target == shooter || shooter->isRidingSameEntity(target)) {
            return false;
        }
    }

    return true;
}

bool ProjectileEntity::mayInteract(IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(pos);
    const Entity* shooter = getShooter();
    if (shooter != nullptr) {
        auto* player = dynamic_cast<const Player*>(shooter);
        if (player != nullptr) {
            // 发射者是玩家：委托给玩家的 mayInteract
            return player->mayInteract(world, pos);
        }
        // 发射者是非玩家实体：取决于 MOB_GRIEFING 游戏规则
        return world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);
    }
    // 无主投掷物（如发射器发射的）：允许交互
    return true;
}

bool ProjectileEntity::mayBreak(IWorld& world) const
{
    // 对应 MC Java 的 Projectile.mayBreak(ServerLevel):
    // return this.getType().is(EntityTypeTags.IMPACT_PROJECTILES)
    //     && p_376471_.getGameRules().get(GameRules.PROJECTILES_CAN_BREAK_BLOCKS);

    // 检查投射物类型是否属于 IMPACT_PROJECTILES 标签。
    // EntityTypeTag::contains(const EntityType&) 内部即 contains(entityType.name())，
    // 这里直接用 typeId 字符串重载，避免一次 EntityRegistry 全局查询（与 MC Java
    // getType() 返回本地字段的语义一致）。
    const std::string typeId = getTypeId();
    if (!EntityTypeTags::IMPACT_PROJECTILES().contains(typeId)) {
        return false;
    }

    // 检查游戏规则
    return world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::PROJECTILES_CAN_BREAK_BLOCKS);
}

void ProjectileEntity::onEntityHit(const RayTraceResult& result)
{
    (void)result;
}

void ProjectileEntity::onBlockHit(const RayTraceResult& result)
{
    m_builtIn.velocity->m_velocity = Vector3(0.0f, 0.0f, 0.0f);

    // 通知命中方块有投掷物命中
    if (m_world != nullptr && result.type == RayTraceResultType::Block) {
        const BlockState* state = m_world->getBlockState(result.blockPos);
        if (state != nullptr) {
            Block& block = state->getBlockMutable();
            BlockRaycastResult hitResult =
                BlockRaycastResult::hit(result.hitPosition, result.blockPos, result.face, 0.0f);
            block.onProjectileHit(*m_world, *state, hitResult, *this);
        }
    }
}

void ProjectileEntity::onImpact(const RayTraceResult& result)
{
    if (result.type == RayTraceResultType::Entity && result.hitEntity != nullptr) {
        // 命中实体时，先检查该实体是否偏转弹射物
        // 对应 MC Java 的 Projectile.hitTargetOrDeflectSelf()
        const ProjectileDeflection deflection = result.hitEntity->deflection(*this);
        if (deflection != ProjectileDeflection::None) {
            auto* owner = tryGetComponent<ecs::ProjectileOwnerComponent>();
            const EntityInstanceId lastDeflectedById =
                (owner != nullptr) ? owner->m_lastDeflectedById : INVALID_ENTITY_ID;
            // 防止同一实体连续偏转
            if (result.hitEntity->id() != lastDeflectedById) {
                if (deflect(deflection, *result.hitEntity, false)) {
                    if (owner != nullptr) {
                        owner->m_lastDeflectedById = result.hitEntity->id();
                    }
                }
            }
            // 被偏转后不调用 onEntityHit，直接返回
            return;
        }
    }

    switch (result.type) {
        case RayTraceResultType::Entity:
            onEntityHit(result);
            break;
        case RayTraceResultType::Block:
            onBlockHit(result);
            break;
        case RayTraceResultType::Miss:
        default:
            break;
    }
}

void ProjectileEntity::updateRotation()
{
    ProjectileHelper::rotateTowardsMovement(*this, 0.2f);
}

bool ProjectileEntity::checkLeftShooter()
{
    Entity* shooter = getShooter();
    if (shooter == nullptr) {
        return true;
    }

    // 检查投掷物是否已离开发射者的碰撞箱
    return !boundingBox().intersects(shooter->boundingBox());
}

void ProjectileEntity::tryUpdateLeftShooter()
{
    auto* owner = tryGetComponent<ecs::ProjectileOwnerComponent>();
    if (owner != nullptr && !owner->m_leftShooter) {
        owner->m_leftShooter = checkLeftShooter();
    }
}

bool ProjectileEntity::deflect(ProjectileDeflection deflection, Entity& deflector, bool wasPlayerDeflect)
{
    if (deflection == ProjectileDeflection::None) {
        return false;
    }

    // 应用偏转效果
    if (!applyProjectileDeflection(deflection, *this, deflector)) {
        return false;
    }

    // 偏转后回调
    onDeflection(wasPlayerDeflect);

    return true;
}

RayTraceResult ProjectileEntity::performRayTrace()
{
    const Vector3 start = m_builtIn.stateVector->m_pos;
    Vector3 end = m_builtIn.stateVector->m_pos + m_builtIn.velocity->m_velocity;

    const RayTraceResult blockResult = rayTraceBlocks(start, end);
    if (blockResult.type == RayTraceResultType::Block) {
        end = blockResult.hitPosition;
    }

    const RayTraceResult entityResult = rayTraceEntities(start, end);
    if (entityResult.type == RayTraceResultType::Entity) {
        return entityResult;
    }

    return blockResult;
}

RayTraceResult ProjectileEntity::rayTraceEntities(const Vector3& start, const Vector3& end)
{
    if (m_world == nullptr) {
        return RayTraceResult::miss();
    }

    const AxisAlignedBB searchBox = ProjectileHelper::createMovementSearchBox(*this, end - start, 1.0f);

    return ProjectileHelper::rayTraceEntities(
        *m_world, *this, start, end, searchBox, [this](const Entity& candidate) { return canHitEntity(candidate); });
}

RayTraceResult ProjectileEntity::rayTraceBlocks(const Vector3& start, const Vector3& end)
{
    if (m_world == nullptr) {
        return RayTraceResult::miss();
    }

    const Vector3 delta = end - start;
    if (delta.lengthSquared() <= 1.0e-6f) {
        return RayTraceResult::miss();
    }

    // 使用 COLLIDER 模式进行射线追踪
    const RaycastContext context(Ray(start, delta.normalized()), delta.length());
    const BlockRaycastResult blockResult = raycastBlocks(context, *m_world);
    if (blockResult.isMiss()) {
        return RayTraceResult::miss();
    }

    return RayTraceResult::block(blockResult.hitPosition(), blockResult.blockPos(), blockResult.face());
}

} // namespace entity
} // namespace mc
