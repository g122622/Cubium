#include "ConduitEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/interfaces/IMob.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/damage/DamageSource.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/sound/SoundCategory.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "util/math/random/Random.hpp"
#include "util/math/MathConstants.hpp"
#include "util/assert/AssertAll.hpp"

#include <cmath>
#include <algorithm>

namespace mc {
namespace blockentity {

namespace {

/// 效果持续时间（tick）
constexpr i32 EFFECT_DURATION = 260;

/// 效果放大器
constexpr i32 EFFECT_AMPLIFIER = 0;

/// 攻击伤害
constexpr f32 ATTACK_DAMAGE = 4.0f;

/// 攻击范围
constexpr f32 ATTACK_RANGE = 8.0f;

/// 激活所需最小框架方块数
constexpr i32 MIN_FRAME_BLOCKS = 16;

/// 睁眼所需框架方块数
constexpr i32 EYE_OPEN_FRAME_BLOCKS = 42;

/// 重新计算结构的间隔（tick）
constexpr i32 UPDATE_INTERVAL = 40;

/// 音效播放间隔范围（tick）
constexpr i64 AMBIENT_SOUND_INTERVAL_MIN = 60;
constexpr i64 AMBIENT_SOUND_INTERVAL_MAX = 100;

/// 效果应用间隔（tick）
constexpr i64 EFFECT_APPLY_INTERVAL = 40;

/// 攻击检测间隔（tick）
constexpr i64 ATTACK_INTERVAL = 40;

/// 计算效果范围
/// MC 1.16.5: frameCount / 7 * 16
[[nodiscard]] i32 calculateEffectRange(i32 frameCount) {
    return (frameCount / 7) * 16;
}

} // namespace

ConduitEntity::ConduitEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Conduit, pos) {
}

f32 ConduitEntity::getActiveRotation(f32 partialTick) const {
    return (m_activeRotation + partialTick) * -0.0375f;
}

i32 ConduitEntity::getEffectRange() const {
    return calculateEffectRange(static_cast<i32>(m_prismarinePositions.size()));
}

void ConduitEntity::tick(IWorld& world) {
    ++m_ticksExisted;

    const i64 gameTime = static_cast<i64>(world.getGameTime());

    // 每40tick重新检测激活状态
    if (gameTime % UPDATE_INTERVAL == 0) {
        const bool wasActive = m_active;
        setActive(world, shouldBeActive(world));

        // 服务端：激活时应用效果和攻击
        if (!world.isClientSide() && m_active) {
            addEffectsToPlayers(world);
            attackMobs(world);
        }
    }

    // 客户端：更新旋转和粒子
    if (world.isClientSide()) {
        if (m_active) {
            ++m_activeRotation;
        }
        spawnParticles(world);
    }

    // 激活状态下播放环境音效
    // MC 1.16.5: 每隔随机间隔（60-100 ticks）播放一次
    if (m_active && !world.isClientSide()) {
        if (m_ambientSoundCounter <= 0) {
            // 播放环境音效
            world.playSound(
                SoundEvents::BLOCK_CONDUIT_AMBIENT,
                sound::SoundCategory::Blocks,
                m_pos.center(),
                1.0f,
                1.0f
            );
            // 重置计数器为随机间隔
            m_ambientSoundCounter = AMBIENT_SOUND_INTERVAL_MIN +
                world.getRandom().nextInt(
                    static_cast<i32>(AMBIENT_SOUND_INTERVAL_MAX - AMBIENT_SOUND_INTERVAL_MIN + 1)
                );
        } else {
            --m_ambientSoundCounter;
        }
    }
}

bool ConduitEntity::shouldBeActive(IWorld& world) {
    m_prismarinePositions.clear();

    // MC 1.16.5: 检测中心周围3x3x3是否全部是水
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                const BlockPos checkPos(m_pos.x + dx, m_pos.y + dy, m_pos.z + dz);
                if (!isWaterAt(world, checkPos)) {
                    return false;
                }
            }
        }
    }

    // MC 1.16.5: 检测5x5x5范围内的框架方块
    // 框架位置：距离中心2格，且在坐标轴上
    // 判断条件: |dx| > 1 || |dy| > 1 || |dz| > 1
    //         且 (dx == 0 && (|dy| == 2 || |dz| == 2)) ||
    //             (dy == 0 && (|dx| == 2 || |dz| == 2)) ||
    //             (dz == 0 && (|dx| == 2 || |dy| == 2))
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dz = -2; dz <= 2; ++dz) {
                const i32 adx = std::abs(dx);
                const i32 ady = std::abs(dy);
                const i32 adz = std::abs(dz);

                // 跳过中心3x3x3区域
                if (adx <= 1 && ady <= 1 && adz <= 1) {
                    continue;
                }

                // 检查是否为框架位置（在坐标轴上且距离为2）
                const bool isFramePosition =
                    (dx == 0 && (ady == 2 || adz == 2)) ||
                    (dy == 0 && (adx == 2 || adz == 2)) ||
                    (dz == 0 && (adx == 2 || ady == 2));

                if (!isFramePosition) {
                    continue;
                }

                const BlockPos framePos(m_pos.x + dx, m_pos.y + dy, m_pos.z + dz);
                if (isValidFrameBlock(world, framePos)) {
                    m_prismarinePositions.push_back(framePos);
                }
            }
        }
    }

    // 激活条件：至少16个框架方块
    const i32 frameCount = static_cast<i32>(m_prismarinePositions.size());

    // 眼睛状态：42个或更多框架方块时睁开
    setEyeOpen(frameCount >= EYE_OPEN_FRAME_BLOCKS);

    return frameCount >= MIN_FRAME_BLOCKS;
}

void ConduitEntity::addEffectsToPlayers(IWorld& world) {
    const i32 frameCount = static_cast<i32>(m_prismarinePositions.size());
    const i32 range = calculateEffectRange(frameCount);

    // 计算效果范围（轴对齐包围盒）
    const Vector3 center = m_pos.center();

    // 获取范围内的玩家
    const std::vector<Entity*> entities = world.getEntitiesInRange(
        center,
        static_cast<f32>(range),
        nullptr  // 不过滤类型，后面自己筛选
    );

    for (Entity* entity : entities) {
        // 只对玩家生效
        auto* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // MC 1.16.5: 玩家必须在水中（isWet = isInWater || isInRain）
        if (!player->isWet()) {
            continue;
        }

        // 添加潮涌能量效果
        const entity::effect::EffectInstance effect(
            entity::effect::EffectType::ConduitPower,
            EFFECT_DURATION,
            EFFECT_AMPLIFIER,
            true,   // ambient
            true,   // visible
            true    // showIcon
        );
        player->addEffect(effect);
    }
}

void ConduitEntity::attackMobs(IWorld& world) {
    const i32 frameCount = static_cast<i32>(m_prismarinePositions.size());

    // 需要至少42个框架方块才能攻击
    if (frameCount < EYE_OPEN_FRAME_BLOCKS) {
        m_target = nullptr;
        m_targetUuid = std::nullopt;
        return;
    }

    // 尝试从UUID恢复目标（UUID当前为String类型）
    // TODO: 实现通过UUID查找实体
    // if (m_target == nullptr && m_targetUuid.has_value()) {
    //     m_target = world.getEntityByUUID(*m_targetUuid);
    // }

    // 选择新目标
    if (m_target == nullptr) {
        const Vector3 center = m_pos.center();

        const std::vector<Entity*> entities = world.getEntitiesInRange(
            center,
            ATTACK_RANGE,
            nullptr
        );

        std::vector<LivingEntity*> hostileMobs;
        for (Entity* entity : entities) {
            // 检查是否为 LivingEntity
            auto* living = dynamic_cast<LivingEntity*>(entity);
            if (living == nullptr) {
                continue;
            }

            // 检查是否在水中
            if (!living->isWet()) {
                continue;
            }

            // MC 1.16.5: 检查是否为敌对生物 (IMob 接口)
            // 只有实现 IMob 接口的实体才是敌对生物
            if (dynamic_cast<entity::IMob*>(living) == nullptr) {
                continue;
            }

            hostileMobs.push_back(living);
        }

        if (!hostileMobs.empty()) {
            // 随机选择一个目标
            math::Random& random = world.getRandom();
            const i32 index = random.nextInt(static_cast<i32>(hostileMobs.size()));
            m_target = hostileMobs[static_cast<size_t>(index)];
        }
    }

    // 检查目标是否有效
    if (m_target != nullptr) {
        // 检查目标是否还活着且在范围内
        const Vector3 targetPos = m_target->position();
        const Vector3 center = m_pos.center();

        const f32 distanceSq = center.distanceSquared(targetPos);
        const bool targetValid = m_target->isAlive() && distanceSq <= ATTACK_RANGE * ATTACK_RANGE;

        if (!targetValid) {
            m_target = nullptr;
        } else {
            // MC 1.16.5: 攻击目标 - 使用魔法伤害（绕过护甲）
            auto magicDamage = DamageSources::magic();
            m_target->hurt(magicDamage, ATTACK_DAMAGE);

            // 播放攻击音效
            world.playSound(
                SoundEvents::BLOCK_CONDUIT_ATTACK_TARGET,
                sound::SoundCategory::Blocks,
                m_target->position(),
                1.0f,
                1.0f
            );
        }
    }
}

bool ConduitEntity::isWaterAt(IWorld& world, const BlockPos& pos) const {
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    // MC 1.16.5: 检查是否为水方块或含水的流体状态
    // 参考: world.hasWater(pos)
    // 简化实现：检查是否为水方块
    return state->is(VanillaBlocks::WATER);
}

bool ConduitEntity::isValidFrameBlock(IWorld& world, const BlockPos& pos) const {
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    const Block* block = &state->getBlock();

    // MC 1.16.5: 有效框架方块
    // PRISMARINE, PRISMARINE_BRICKS, SEA_LANTERN, DARK_PRISMARINE
    return block == VanillaBlocks::PRISMARINE ||
           block == VanillaBlocks::PRISMARINE_BRICKS ||
           block == VanillaBlocks::DARK_PRISMARINE ||
           block == VanillaBlocks::SEA_LANTERN;
}

void ConduitEntity::setActive(IWorld& world, bool active) {
    if (m_active != active) {
        m_active = active;
        setChanged();

        // 播放激活/取消激活音效
        if (active) {
            world.playSound(
                SoundEvents::BLOCK_CONDUIT_ACTIVATE,
                sound::SoundCategory::Blocks,
                m_pos.center(),
                1.0f,
                1.0f
            );
        } else {
            world.playSound(
                SoundEvents::BLOCK_CONDUIT_DEACTIVATE,
                sound::SoundCategory::Blocks,
                m_pos.center(),
                1.0f,
                1.0f
            );
        }
    }
}

void ConduitEntity::setEyeOpen(bool eyeOpen) {
    if (m_eyeOpen != eyeOpen) {
        m_eyeOpen = eyeOpen;
        setChanged();
    }
}

void ConduitEntity::spawnParticles(IWorld& world) {
    // 只在激活状态下生成粒子
    if (!m_active) {
        return;
    }

    // 参考 MC 1.16.5 ConduitTileEntity.spawnParticles()
    math::Random& random = world.getRandom();

    // 计算潮涌核心上方的粒子发射点
    // d0 = sin((ticksExisted + 35) * 0.1) / 2 + 0.5
    // d0 = (d0 * d0 + d0) * 0.3
    const f32 sinValue = std::sin(static_cast<f32>(m_ticksExisted + 35) * 0.1f);
    f32 d0 = (sinValue / 2.0f + 0.5f);
    d0 = (d0 * d0 + d0) * 0.3f;

    // 粒子发射点位置：潮涌核心上方 1.5 格 + d0 的垂直偏移
    const Vector3 particleOrigin(
        static_cast<f32>(m_pos.x) + 0.5f,
        static_cast<f32>(m_pos.y) + 1.5f + d0,
        static_cast<f32>(m_pos.z) + 0.5f
    );

    // 从框架方块向潮涌核心发射粒子
    for (const BlockPos& framePos : m_prismarinePositions) {
        // MC 1.16.5: 每个框架方块有 1/50 的概率发射粒子
        if (random.nextInt(50) != 0) {
            continue;
        }

        // 随机偏移
        const f32 offsetX = -0.5f + random.nextFloat();
        const f32 offsetY = -2.0f + random.nextFloat();
        const f32 offsetZ = -0.5f + random.nextFloat();

        // 计算从框架方块到潮涌核心的相对位置
        const BlockPos relativePos = framePos - m_pos;

        // 粒子速度方向：从潮涌核心向框架方块方向
        const Vector3 velocity(
            static_cast<f32>(relativePos.x) + offsetX,
            static_cast<f32>(relativePos.y) + offsetY,
            static_cast<f32>(relativePos.z) + offsetZ
        );

        // 生成鹦鹉螺粒子
        world.addParticle(
            client::renderer::trident::particle::ParticleTypeId::Nautilus,
            particleOrigin,
            velocity
        );
    }

    // 如果有攻击目标，在目标位置生成粒子
    if (m_target != nullptr && m_target->isAlive()) {
        // 目标眼睛高度位置
        const Vector3 targetEyePos = m_target->position() + Vector3(0.0f, m_target->eyeHeight(), 0.0f);

        // 随机偏移：根据目标尺寸计算
        const f32 targetWidth = m_target->width();
        const f32 targetHeight = m_target->height();

        const f32 offsetX = (-0.5f + random.nextFloat()) * (3.0f + targetWidth);
        const f32 offsetY = -1.0f + random.nextFloat() * targetHeight;
        const f32 offsetZ = (-0.5f + random.nextFloat()) * (3.0f + targetWidth);

        const Vector3 velocity(offsetX, offsetY, offsetZ);

        // 生成鹦鹉螺粒子
        world.addParticle(
            client::renderer::trident::particle::ParticleTypeId::Nautilus,
            targetEyePos,
            velocity
        );
    }
}

bool ConduitEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载目标UUID
    if (data.contains("target_uuid") && data["target_uuid"].is_string()) {
        m_targetUuid = data["target_uuid"].get<std::string>();
    }

    return true;
}

void ConduitEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    // 保存目标UUID
    if (m_target != nullptr) {
        data["target_uuid"] = m_target->uuid();
    }
}

std::unique_ptr<BlockEntity> ConduitEntity::clone() const {
    auto cloned = std::make_unique<ConduitEntity>(m_pos);
    cloned->m_active = m_active;
    cloned->m_eyeOpen = m_eyeOpen;
    cloned->m_ticksExisted = m_ticksExisted;
    cloned->m_activeRotation = m_activeRotation;
    cloned->m_prismarinePositions = m_prismarinePositions;
    cloned->m_targetUuid = m_targetUuid;
    // 注意：m_target 指针不克隆，需要在世界加载后重新查找
    return cloned;
}

} // namespace blockentity
} // namespace mc
