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

#include "ClientEntity.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/boss/WitherEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp"
#include "common/entity/entities/passive/special/PolarBearEntity.hpp"
#include "common/entity/entities/passive/tamable/CatEntity.hpp"
#include "common/entity/entities/passive/tamable/OcelotEntity.hpp"
#include "common/entity/entities/passive/tamable/TameableEntity.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/network/codec/EntityMetadataSerializer.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace mc::trace;

namespace mc::client {

ClientEntity::ClientEntity(EntityInstanceId id, const std::string& typeId)
    : m_id(id)
    , m_typeId(typeId)
{}

const entity::EntityType* ClientEntity::entityType() const
{
    // 懒查询：缓存未命中且 m_typeId 非空时，按名查注册表填充。
    // 返回指针指向 EntityRegistry::m_types 内对象，与 VanillaEntityTypeKeys::* 同源，
    // 地址稳定（deque 不失效），可安全指针比较。与服务端 Entity::entityType() 一致。
    // 注意：客户端实体类型可能未注册（模组实体），此时返回 nullptr。
    if (m_entityType == nullptr && !m_typeId.empty()) {
        m_entityType = entity::EntityRegistry::instance().getType(m_typeId);
    }
    return m_entityType;
}

void ClientEntity::setInterpolationSpeed(f32 speed)
{
    m_interpolationSpeed = std::clamp(speed, 0.01f, 1.0f);
}

void ClientEntity::setPosition(f32 x, f32 y, f32 z)
{
    // 每60次log一次位置更新
    // static u32 setPositionCounter = 0;
    // setPositionCounter++;
    // if (setPositionCounter % 120 == 0) {
    //     spdlog::info("[ClientEntity] Entity {} setPosition: ({:.2f}, {:.2f}, {:.2f}) -> ({:.2f}, {:.2f}, {:.2f})",
    //                  m_id, m_position.x, m_position.y, m_position.z, x, y, z);
    // }

    m_prevPosition = m_position;
    m_position = Vector3(x, y, z);
    m_targetPosition = m_position;
}

void ClientEntity::setTargetPosition(f32 x, f32 y, f32 z)
{
    m_targetPosition = Vector3(x, y, z);
}

void ClientEntity::tickPosition()
{
    // 保存当前位置作为上一帧位置（用于渲染插值）
    m_prevPosition = m_position;
    // 不在这里做平滑插值，平滑插值在 updateInterpolation 中每帧执行
}

void ClientEntity::updateInterpolation(f32 deltaTime)
{
    if (m_smoothInterpolation) {
        // 平滑插值位置
        // 使用 deltaTime 归一化到 20 TPS 的等效插值速度
        // 例如：deltaTime=0.016s (60 FPS), interpolationSpeed=0.3
        // 实际插值 = 1 - pow(1 - 0.3, deltaTime * 20) ≈ 0.093
        // 这样无论帧率如何，插值速度都保持一致的感觉
        const f32 tickRate = 20.0f;
        const f32 alpha = 1.0f - std::pow(1.0f - m_interpolationSpeed, deltaTime * tickRate);

        Vector3 diff = m_targetPosition - m_position;
        m_position = m_position + diff * alpha;

        // 平滑插值旋转，需要处理角度环绕
        // Yaw
        f32 yawDiff = m_targetYaw - m_yaw;
        while (yawDiff > 180.0f)
            yawDiff -= 360.0f;
        while (yawDiff < -180.0f)
            yawDiff += 360.0f;
        m_yaw += yawDiff * alpha;
        while (m_yaw > 180.0f)
            m_yaw -= 360.0f;
        while (m_yaw < -180.0f)
            m_yaw += 360.0f;

        // Pitch (范围 -90 到 90)
        f32 pitchDiff = m_targetPitch - m_pitch;
        m_pitch += pitchDiff * alpha;
        m_pitch = std::clamp(m_pitch, -90.0f, 90.0f);

        // HeadYaw
        f32 headYawDiff = m_targetHeadYaw - m_headYaw;
        while (headYawDiff > 180.0f)
            headYawDiff -= 360.0f;
        while (headYawDiff < -180.0f)
            headYawDiff += 360.0f;
        m_headYaw += headYawDiff * alpha;
        while (m_headYaw > 180.0f)
            m_headYaw -= 360.0f;
        while (m_headYaw < -180.0f)
            m_headYaw += 360.0f;
    } else {
        // 禁用平滑插值时，直接跳到目标位置
        m_position = m_targetPosition;
        m_yaw = m_targetYaw;
        m_pitch = m_targetPitch;
        m_headYaw = m_targetHeadYaw;
    }
}

Vector3 ClientEntity::getInterpolatedPosition(f32 partialTick) const
{
    return Vector3(m_prevPosition.x + (m_position.x - m_prevPosition.x) * partialTick,
        m_prevPosition.y + (m_position.y - m_prevPosition.y) * partialTick,
        m_prevPosition.z + (m_position.z - m_prevPosition.z) * partialTick);
}

void ClientEntity::setRotation(f32 yaw, f32 pitch)
{
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_yaw = yaw;
    m_pitch = pitch;
    m_targetYaw = yaw;
    m_targetPitch = pitch;
}

void ClientEntity::setTargetRotation(f32 yaw, f32 pitch)
{
    m_targetYaw = yaw;
    m_targetPitch = pitch;
}

void ClientEntity::setHeadRotation(f32 headYaw)
{
    m_prevHeadYaw = m_headYaw;
    m_headYaw = headYaw;
    m_targetHeadYaw = headYaw;
}

void ClientEntity::setTargetHeadRotation(f32 headYaw)
{
    m_targetHeadYaw = headYaw;
}

void ClientEntity::tickRotation()
{
    // 保存当前旋转作为上一帧旋转（用于渲染插值）
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_prevHeadYaw = m_headYaw;
    // 不在这里做平滑插值，平滑插值在 updateInterpolation 中每帧执行
}

f32 ClientEntity::getInterpolatedYaw(f32 partialTick) const
{
    return m_prevYaw + (m_yaw - m_prevYaw) * partialTick;
}

f32 ClientEntity::getInterpolatedPitch(f32 partialTick) const
{
    return m_prevPitch + (m_pitch - m_prevPitch) * partialTick;
}

f32 ClientEntity::getInterpolatedHeadYaw(f32 partialTick) const
{
    return m_prevHeadYaw + (m_headYaw - m_prevHeadYaw) * partialTick;
}

void ClientEntity::setVelocity(f32 x, f32 y, f32 z)
{
    m_velocity = Vector3(x, y, z);
}

void ClientEntity::setMetadata(const std::vector<u8>& metadata)
{
    // ItemEntity 物品本体经元数据 serializerId 7（ITEM_STACK）同步：服务端 ItemEntity 注册
    // DATA_ITEM_PARAM（ItemStackView），EntityMetadataSerializer 序列化为 1.21.11 Slot 字节，
    // 此处反序列化进 m_dataManager，再由 syncMetadataFromDataManager 的 item 分支经
    // fromItemStackView 还原为业务侧 ItemStack 并 setItemStack 触发渲染。
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Client.Entity, "ClientEntity::setMetadata", "entityId", m_id, "size", metadata.size());

    m_metadata = metadata;
    if (!m_metadata.empty()) {
        (void)network::EntityMetadataSerializer::deserialize(m_metadata, m_dataManager);
        syncMetadataFromDataManager();
    }
}

void ClientEntity::syncMetadataFromDataManager()
{
    // 物品实体特殊处理：从 DATA_ITEM_PARAM（ItemStackView）还原物品本体并刷新渲染。
    if (m_typeId == entity::EntityTypeKeys::ITEM || m_typeId == "minecraft:item") {
        if (const auto view = _readMetadata<network::ir::play::ItemStackView>(ItemEntity::DATA_ITEM_PARAM.id());
            view.has_value()) {
            auto stackResult = network::ir::fromItemStackView(*view);
            if (stackResult.success()) {
                setItemStack(stackResult.value());
            }
        }

        return;
    }

    // 北极熊站立状态同步
    if (m_typeId == "minecraft:polar_bear" || m_typeId == "polar_bear") {
        if (const auto standing = _readMetadata<bool>(PolarBearEntity::getStandingParamId()); standing.has_value()) {
            setStanding(*standing);
        }
    }

    // 河豚膨胀状态同步
    if (m_typeId == "minecraft:pufferfish" || m_typeId == "pufferfish") {
        if (const auto puffState = _readMetadata<i32>(::mc::PufferfishEntity::getPuffStateParamId());
            puffState.has_value()) {
            setPuffState(*puffState);
        }
    }

    // 豹猫信任状态同步
    if (m_typeId == "minecraft:ocelot" || m_typeId == "ocelot") {
        if (const auto trusting = _readMetadata<bool>(::mc::OcelotEntity::getTrustingParamId()); trusting.has_value()) {
            setTrusting(*trusting);
        }
        // 注意：豹猫逃跑状态（isFleeing）通过 OcelotEntity::DATA_FLEEING_PARAM 同步，
        // 渲染器直接从 OcelotEntity::isFleeing() 读取，无需在 ClientEntity 中维护副本
    }

    // 猫动画状态同步
    // vanilla Cat wire 字段顺序: variant(19)/lying(20)/relax(21)/collar(22)。
    // 服务端 CatEntity 现已对齐下发全部 4 字段;客户端当前仅消费 lying/relax 驱动躺下动画,
    // variant(皮肤)/collar(颈圈色)未镜像渲染——TODO: 待 CatRenderer 接入变体纹理与颈圈色。
    if (m_typeId == "minecraft:cat" || m_typeId == "cat") {
        if (const auto lying = _readMetadata<bool>(::mc::CatEntity::getLyingParamId()); lying.has_value()) {
            setCatLieDown(*lying);
        }
        if (const auto relax = _readMetadata<bool>(::mc::CatEntity::getRelaxStateOneParamId()); relax.has_value()) {
            setCatRelaxStateOne(*relax);
        }
        // TODO: 消费 DATA_VARIANT_ID_PARAM(HolderVariantValue) 与 DATA_COLLAR_COLOR_PARAM(i32),
        //       镜像到客户端猫皮肤变体与颈圈颜色,驱动 CatRenderer 选纹理。
    }

    // 狼状态同步（驯服状态、颈圈颜色、兴趣状态、愤怒状态）
    // 服务端 WolfEntity/TameableEntity 通过 DataParameter 写入，
    // 由 EntityTracker 广播 ir::play::SetEntityData 到客户端，
    // 客户端在此处读取并调用 setWolfTamed/setWolfCollarColor/setWolfIsInterested/setWolfIsAngry 更新镜像状态。
    if (m_typeId == "minecraft:wolf" || m_typeId == "wolf") {
        // 驯服/坐下状态（通过 TameableEntity::DATA_FLAGS_PARAM 同步，Byte 类型）
        // 对齐 vanilla TamableAnimal.DATA_FLAGS_ID：bit2(mask 0x4)=tame，bit0(mask 0x1)=sitting。
        // 旧实现误读为 bool（对应旧 DATA_TAMED_PARAM），现改为读 Byte 并解位。
        if (const auto flags = _readMetadata<i8>(::mc::TameableEntity::getTamedParamId()); flags.has_value()) {
            setWolfTamed((*flags & 0x04) != 0);
            setSitting((*flags & 0x01) != 0);
        }
        // 颈圈颜色（通过 WolfEntity::DATA_COLLAR_COLOR_PARAM 同步）
        if (const auto colorValue = _readMetadata<i32>(::mc::WolfEntity::getCollarColorParamId());
            colorValue.has_value()) {
            if (*colorValue >= 0 && *colorValue <= 15) {
                setWolfCollarColor(static_cast<DyeColor>(*colorValue));
            }
        }
        // 兴趣状态（乞求食物头部倾斜动画，通过 WolfEntity::DATA_INTERESTED_PARAM 同步）
        if (const auto interested = _readMetadata<bool>(::mc::WolfEntity::getInterestedParamId());
            interested.has_value()) {
            setWolfIsInterested(*interested);
        }
        // 愤怒状态（尾巴抬起/停止摆动 + angry 纹理变体，通过 WolfEntity::DATA_ANGER_TIME_PARAM 同步）
        // 该参数对齐 vanilla Wolf.DATA_ANGER_END_TIME，wire 类型为 Long(i64)，故读 i64。
        if (m_dataManager.hasParam(::mc::WolfEntity::getAngerTimeParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::WolfEntity::getAngerTimeParamId()); value != nullptr) {
                const i64 angerTime = value->get<i64>();
                setWolfIsAngry(angerTime > 0);
            }
        }
    }

    // 末影人状态同步（搬方块状态、注视状态）
    // 服务端 EndermanEntity 通过 DataParameter 写入，
    // 由 EntityTracker 广播 ir::play::SetEntityData 到客户端，
    // 客户端在此处读取并调用 setEndermanHeldBlockState/setEndermanScreaming 更新镜像状态。
    if (m_typeId == "minecraft:enderman" || m_typeId == "enderman") {
        // 搬方块状态（通过 EndermanEntity::DATA_CARRIED_BLOCK_STATE_ID_PARAM 同步）
        // 该参数对齐 vanilla EnderMan.DATA_CARRY_STATE，wire 类型为 Optional<BlockState>
        // (OptionalBlockStateValue, serializerId=15)，present 表示持有方块。
        if (m_dataManager.hasParam(::mc::EndermanEntity::getCarriedBlockStateIdParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::EndermanEntity::getCarriedBlockStateIdParamId());
                value != nullptr) {
                const auto obs = value->get<mc::entity::OptionalBlockStateValue>();
                if (obs.present) {
                    setEndermanHeldBlockState(::mc::BlockRegistry::instance().getBlockState(obs.stateId));
                } else {
                    setEndermanHeldBlockState(nullptr);
                }
            }
        }
        // 注视状态（通过 EndermanEntity::DATA_SCREAMING_PARAM 同步）
        if (m_dataManager.hasParam(::mc::EndermanEntity::getScreamingParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::EndermanEntity::getScreamingParamId());
                value != nullptr) {
                setEndermanScreaming(value->get<bool>());
            }
        }
    }

    // 下落方块状态同步
    // 对齐 MC 1.21.11：FallingBlock 的 BlockState 不走 SynchedEntityData，而是经
    // AddEntity 包 data 字段下发 stateId。客户端在 ClientPlayVisitor 处理 AddEntity 时
    // 据此调用 setFallingBlockState（见 ClientPlayVisitor AddEntity 分支），故此处不再
    // 从 metadata 读取。SynchedEntityData 仅有 DATA_START_POS(BlockPos,id8)，客户端
    // 下落方块渲染不依赖 startPos，无需在此消费。

    // TNT 实体状态同步（引信、方块状态）
    // 服务端 TNTEntity 通过 DataParameter 写入 DATA_FUSE_PARAM 和 DATA_BLOCK_STATE_PARAM，
    // 由 EntityTracker 广播 ir::play::SetEntityData 到客户端，
    // 客户端在此处读取并调用 setTntFuse/setTntBlockState 更新镜像状态。
    // 对应 MC 1.21.11 PrimedTnt.getFuse() / getBlockState()。
    if (m_typeId == "minecraft:tnt" || m_typeId == "tnt") {
        // 引信（通过 TNTEntity::DATA_FUSE_PARAM 同步）
        if (m_dataManager.hasParam(::mc::entity::TNTEntity::getFuseParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::entity::TNTEntity::getFuseParamId()); value != nullptr) {
                setTntFuse(value->get<i32>());
            }
        }
        // 方块状态（通过 TNTEntity::DATA_BLOCK_STATE_PARAM 同步，BlockStateValue → BLOCK_STATE id14）
        if (m_dataManager.hasParam(::mc::entity::TNTEntity::getBlockStateParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::entity::TNTEntity::getBlockStateParamId());
                value != nullptr) {
                const u32 stateId = value->get<::mc::entity::BlockStateValue>().stateId;
                if (stateId > 0) {
                    setTntBlockState(::mc::BlockRegistry::instance().getBlockState(stateId));
                } else {
                    setTntBlockState(nullptr);
                }
            }
        }
    }

    // 骷髅拉弓状态同步（普通骷髅 skeleton、流浪者 stray、沼骸骨 bogged）
    // 通过 AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM 同步。
    // 由 AbstractSkeletonEntity::tick 根据 isUsingItem + 持弓状态写入，
    // 客户端读取后驱动 SkeletonModel 的 BowAndArrow 姿态（拉弓动画）。
    // 凋灵骷髅不持弓，不注册此参数，hasParam 返回 false，分支自然跳过。
    if (m_typeId == "minecraft:skeleton" || m_typeId == "skeleton" || m_typeId == "minecraft:stray" ||
        m_typeId == "stray" || m_typeId == "minecraft:bogged" || m_typeId == "bogged") {
        if (m_dataManager.hasParam(::mc::AbstractSkeletonEntity::getChargingBowParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::AbstractSkeletonEntity::getChargingBowParamId());
                value != nullptr) {
                const bool charging = value->get<bool>();
                setChargingBow(charging);
            }
        }
    }

    // Mob 激怒/攻击中状态同步（僵尸、尸壳、溺尸、僵尸村民等所有 Mob）
    // 通过 MobEntity::DATA_MOB_FLAGS_PARAM 的位 2 (MOB_FLAG_AGGRESSIVE) 同步。
    // 对应 MC 1.21.11 Mob.isAggressive() / DATA_MOB_FLAGS_ID 位 2。
    // 服务端写入路径：MeleeAttackGoal::startExecuting → setAggroed(true) →
    //   MobEntity::setAggressive(true) → 数据参数置位 MOB_FLAG_AGGRESSIVE。
    // 客户端读取后驱动 ZombieModel::setAggressive，进而影响 animateZombieArms
    // 的空手攻击抬臂基础角度（aggressive 时 -PI/1.5，否则 -PI/2.25）。
    // 注意：DATA_MOB_FLAGS_PARAM 由 MobEntity::registerData 注册，所有继承自
    // MobEntity 的实体（含动物、怪物）都会拥有此参数，因此无需按 typeId 过滤，
    // 仅以 hasParam 判断即可，缺失时（如非 Mob 实体）自然跳过。
    if (m_dataManager.hasParam(::mc::MobEntity::getMobFlagsParamId())) {
        if (const auto* value = m_dataManager.getRaw(::mc::MobEntity::getMobFlagsParamId()); value != nullptr) {
            const i8 flags = value->get<i8>();
            const bool aggressive = (flags & static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask())) != 0;
            setIsAggressive(aggressive);
        }
    }

    // 实体 flags 同步（slot 0，i8）：游泳、鞘翅飞行、潜行、疾跑、发光、着火等共用一个字节。
    // 对应 MC 1.21.11 Entity.getSharedFlag(int) / DATA_FLAGS_ID。
    // 此处只同步 Swimming 位（bit 4）到 ClientEntity::setSwimming，驱动：
    //   1) refreshEyeHeight（游泳时眼睛高度降低到爬行尺寸）
    //   2) isVisuallySwimming / swimAmount 渐入渐出（驱动 DrownedModel 游泳手臂/腿部覆盖动画）
    // 鞘翅飞行（bit 7）已由 ClientEntity::isFallFlying() 动态读取，无需在此重复同步。
    if (const auto flags = _readMetadata<i8>(0); flags.has_value()) {
        const bool swimming = (*flags & static_cast<i8>(EntityFlags::Swimming)) != 0;
        setSwimming(swimming);
    }

    // 凋灵侧头目标同步：HEAD_TARGET_1/2/3 通过元数据同步自服务端 WitherEntity。
    // 客户端在 tickWitherSideHeads() 中使用这些目标 ID 查找目标实体位置，
    // 本地镜像 MC 1.21.11 WitherBoss.aiStep() 的侧头朝向计算逻辑。
    // 注意：侧头朝向角度本身不通过网络同步（与 MC 设计一致），仅同步目标 ID。
    //   HEAD_TARGET_1 = 主头目标（index 0，不参与侧头朝向计算）
    //   HEAD_TARGET_2 = 左头目标（index 1，对应 m_witherSideHeadYaw[0]/Pitch[0]）
    //   HEAD_TARGET_3 = 右头目标（index 2，对应 m_witherSideHeadYaw[1]/Pitch[1]）
    if (m_typeId == "minecraft:wither" || m_typeId == "wither") {
        // 目标 ID 缓存到 m_witherHeadTargetId，供 tickWitherSideHeads 使用
        // 主头目标不参与侧头朝向，但此处仍读取以保持元数据消费一致性
        if (m_dataManager.hasParam(::mc::entity::WitherEntity::getHeadTarget1ParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::entity::WitherEntity::getHeadTarget1ParamId());
                value != nullptr) {
                m_witherHeadTargetId[0] = value->get<i32>();
            }
        }
        if (m_dataManager.hasParam(::mc::entity::WitherEntity::getHeadTarget2ParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::entity::WitherEntity::getHeadTarget2ParamId());
                value != nullptr) {
                m_witherHeadTargetId[1] = value->get<i32>();
            }
        }
        if (m_dataManager.hasParam(::mc::entity::WitherEntity::getHeadTarget3ParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::entity::WitherEntity::getHeadTarget3ParamId());
                value != nullptr) {
                m_witherHeadTargetId[2] = value->get<i32>();
            }
        }
    }

    // 钓鱼浮标状态同步：DATA_HOOKED_ENTITY / DATA_BITING 通过元数据同步自服务端
    // FishingBobberEntity。对应 MC 1.21.11 FishingHook.onSyncedDataUpdated()。
    //   DATA_HOOKED_ENTITY: 被钩住实体 ID（+1 偏移，0=无），缓存到
    //     m_fishingHookedEntityId，供渲染器在生成钓线时查找另一端实体。
    //   DATA_BITING: 是否咬钩，缓存到 m_fishingBiting，供渲染器播放咬钩下沉动画。
    if (m_typeId == "minecraft:fishing_bobber" || m_typeId == "fishing_bobber") {
        if (m_dataManager.hasParam(::mc::entity::FishingBobberEntity::getHookedEntityParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::entity::FishingBobberEntity::getHookedEntityParamId());
                value != nullptr) {
                m_fishingHookedEntityId = value->get<i32>();
            }
        }
        if (m_dataManager.hasParam(::mc::entity::FishingBobberEntity::getBitingParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::entity::FishingBobberEntity::getBitingParamId());
                value != nullptr) {
                m_fishingBiting = value->get<bool>();
            }
        }
    }
}

void ClientEntity::setItemStack(const ItemStack& stack)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity,
        "ClientEntity::setItemStack",
        "entityId",
        m_id,
        "count",
        stack.getCount(),
        "isEmpty",
        stack.isEmpty());

    const bool changed = m_itemStack == nullptr || *m_itemStack != stack;
    m_itemStack = std::make_unique<ItemStack>(stack);
    if (changed) {
        _updateItemRenderStateVersion();
    }
}

void ClientEntity::clearItemStack()
{
    if (m_itemStack != nullptr) {
        m_itemStack.reset();
        _updateItemRenderStateVersion();
    }
}

void ClientEntity::updateAnimation(f32 distanceMoved)
{
    // 保存上一帧状态（用于渲染插值）
    m_prevLimbSwing = m_limbSwing;
    m_prevLimbSwingAmount = m_limbSwingAmount;

    // 更新 limbSwingAmount（移动强度）
    m_limbSwingAmount = distanceMoved;

    // 更新 limbSwing（摆动进度）
    // 摆动速度与移动距离成正比
    m_limbSwing += distanceMoved * 0.6f;

    // 保持 limbSwing 在合理范围内
    // 但不需要严格的 2π 限制，因为 sin/cos 可以处理任意值
    constexpr f32 LIMB_SWING_MAX = math::TWO_PI * 100.0f;
    if (m_limbSwing > LIMB_SWING_MAX) {
        m_limbSwing -= LIMB_SWING_MAX;
    }

    // 更新相机偏航角
    // cameraYaw 用于披风摆动强度计算
    m_cameraYaw += (distanceMoved - m_cameraYaw) * 0.4f;
}

void ClientEntity::triggerSwingAnimation(i32 hand)
{
    m_swingInProgress = true;
    m_swingTickCounter = 0;
    m_swingHand = hand;
    m_prevSwingProgress = m_swingProgress;
    m_swingProgress = 0.0f;
}

void ClientEntity::triggerHurtAnimation()
{
    m_hurtTime = 10;
}

void ClientEntity::triggerLeaveBedAnimation()
{
    m_sleeping = false;
}

void ClientEntity::updateElytraAngles(f32 targetX, f32 targetY, f32 targetZ)
{
    // 使用平滑插值更新鞘翅角度
    constexpr f32 ELYTRA_INTERPOLATION = 0.1f;
    m_rotateElytraX += (targetX - m_rotateElytraX) * ELYTRA_INTERPOLATION;
    m_rotateElytraY += (targetY - m_rotateElytraY) * ELYTRA_INTERPOLATION;
    m_rotateElytraZ += (targetZ - m_rotateElytraZ) * ELYTRA_INTERPOLATION;
}

void ClientEntity::tick()
{
    m_ticksExisted++;

    // 更新位置和旋转的上一帧状态（用于渲染插值）
    tickPosition();
    tickRotation();

    // 更新挥动动画
    if (m_swingInProgress) {
        ++m_swingTickCounter;
        m_prevSwingProgress = m_swingProgress;
        m_swingProgress = static_cast<f32>(m_swingTickCounter) / static_cast<f32>(DEFAULT_SWING_DURATION);

        if (m_swingTickCounter >= DEFAULT_SWING_DURATION) {
            m_swingInProgress = false;
            m_swingProgress = 0.0f;
            m_prevSwingProgress = 0.0f;
        }
    }

    // 更新受伤时间
    if (m_hurtTime > 0) {
        --m_hurtTime;
    }

    // 更新追踪位置系统
    // 用于披风摆动计算
    m_prevChasingPosX = m_chasingPosX;
    m_prevChasingPosY = m_chasingPosY;
    m_prevChasingPosZ = m_chasingPosZ;

    // 平滑追踪实际位置
    f64 dx = static_cast<f64>(m_position.x) - m_chasingPosX;
    f64 dy = static_cast<f64>(m_position.y) - m_chasingPosY;
    f64 dz = static_cast<f64>(m_position.z) - m_chasingPosZ;
    m_chasingPosX += dx * 0.25;
    m_chasingPosY += dy * 0.25;
    m_chasingPosZ += dz * 0.25;

    // 更新相机偏航角
    // 用于披风摆动强度计算
    m_prevCameraYaw = m_cameraYaw;
    // cameraYaw 基于移动距离，在 updateAnimation 中更新

    // 更新北极熊站立动画
    // 仅对北极熊实体有效
    updateStandingAnimation();

    // 更新吃草动画计时器
    if (m_eatAnimationTimer > 0) {
        --m_eatAnimationTimer;
    }

    // 更新 TNT 矿车引信计时器
    if (m_fuseTimer > 0) {
        --m_fuseTimer;
    }

    // 更新铁傀儡攻击动画计时器
    if (m_ironGolemAttackTimer > 0) {
        --m_ironGolemAttackTimer;
        m_ironGolemArmsRaised = true;
        if (m_ironGolemAttackTimer <= 0) {
            m_ironGolemArmsRaised = false;
        }
    }

    // 更新疣猪兽/僵尸疣兽撞飞攻击动画计时器
    if (m_flingAnimationTicks > 0) {
        --m_flingAnimationTicks;
    }

    // 更新狼甩水动画进度（客户端镜像 MC Wolf.tick() 的甩水进度逻辑）
    // 对应 MC Wolf.tick() 第 337-344 行
    if (m_wolfIsShaking) {
        m_wolfShakeAnimO = m_wolfShakeAnim;
        m_wolfShakeAnim += 0.05f;
        if (m_wolfShakeAnimO >= 2.0f) {
            // 甩水完成：重置状态
            m_wolfIsWet = false;
            m_wolfIsShaking = false;
            m_wolfShakeAnimO = 0.0f;
            m_wolfShakeAnim = 0.0f;
        }
    }

    // 更新狼乞求食物头部角度插值（对应 MC Wolf.tick() 第 318-323 行）
    // m_wolfIsInterested 由 syncMetadataFromDataManager 在收到元数据更新时设置，
    // 此处每 tick 推进 m_wolfInterestedAngle 向 1.0（感兴趣）或 0.0（不感兴趣）插值，
    // 渲染时 WolfModel 通过 lerp(partialTick, wolfInterestedAngleO, wolfInterestedAngle) 读取。
    m_wolfInterestedAngleO = m_wolfInterestedAngle;
    if (m_wolfIsInterested) {
        m_wolfInterestedAngle += (1.0f - m_wolfInterestedAngle) * 0.4f;
    } else {
        m_wolfInterestedAngle += (0.0f - m_wolfInterestedAngle) * 0.4f;
    }

    // 更新兔子跳跃动画计时器（对应 MC 1.21.11 Rabbit.aiStep() 中的跳跃推进逻辑）
    // 收到 RabbitJump(1) 状态包时启动 jumpDuration=10，每 tick 递增 jumpTicks，
    // 达到 jumpDuration 后归零并结束本次跳跃。
    // 渲染时 rabbitJumpCompletion(partialTick) 提供 [0, 1] 完成度供 RabbitModel 计算 jumpRotation。
    tickRabbitJump();

    // 推进游泳动画渐变量（客户端镜像 MC LivingEntity.updateSwimAmount()）
    // 对应 MC 1.21.11 LivingEntity.tick() 中的 this.updateSwimAmount()。
    // 客户端不调用 LivingEntity::tick()（ClientEntity::tick 是独立的客户端 tick 路径），
    // 因此需要在此显式推进本地 m_swimAmount/m_swimAmountO 副本，保证与服务端节奏一致。
    // isVisuallySwimming() 的客户端判定依赖 DrownedEntity 重写：isSwimming()（来自
    // syncMetadataFromDataManager 同步的 Swimming 标志位）&& !isRiding()。
    m_swimAmountO = m_swimAmount;
    if (isVisuallySwimming()) {
        m_swimAmount = std::min(1.0f, m_swimAmount + 0.09f);
    } else {
        m_swimAmount = std::max(0.0f, m_swimAmount - 0.09f);
    }
}

void ClientEntity::updateStandingAnimation()
{
    // 保存上一帧动画值
    m_clientSideStandAnimation0 = m_clientSideStandAnimation;

    // 根据站立状态更新动画
    if (m_isStanding) {
        // 站立时动画增加（最大 6.0）
        m_clientSideStandAnimation = std::clamp(m_clientSideStandAnimation + 1.0f, 0.0f, 6.0f);
    } else {
        // 非站立时动画减少（最小 0.0）
        m_clientSideStandAnimation = std::clamp(m_clientSideStandAnimation - 1.0f, 0.0f, 6.0f);
    }
}

bool ClientEntity::isFallFlying() const
{
    // 从元数据管理器读取实体 flags（参数槽位 0）。服务端序列化按 unordered_map 顺序把任意
    // 参数写入字节索引 0，槽位类型不匹配则该实体无 flags，返回 false。
    const auto flags = _readMetadata<i8>(0);
    return flags.has_value() && (*flags & static_cast<i8>(EntityFlags::FallFlying)) != 0;
}

bool ClientEntity::isAngry() const
{
    // TODO: 客户端镜像愤怒状态检测为预存在的占位实现。硬编码槽位 1 不对应任何具体实体的
    // anger 字段（Bee anger wire id=18/i64、Wolf id=21/i64、Enderman/PolarBear 各异），
    // 故此处恒返回 false,从不触发 onEntityAngerStateChanged 音效。正确实现应按实体类型读取
    // 对应 IAngerable 的 anger paramId（参考 Wolf 分支的 hasParam+getRaw 路径）。
    const auto angerTime = _readMetadata<i32>(1);
    return angerTime.has_value() && *angerTime > 0;
}

void ClientEntity::_updateItemRenderStateVersion()
{
    ++m_itemRenderStateVersion;
}

void ClientEntity::refreshEyeHeight()
{
    // 从实体注册表中查找基础眼高
    f32 baseEyeHeight = game::PLAYER_EYE_HEIGHT; // 默认玩家站立眼高

    const auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* type = registry.getType(m_typeId);
    if (type != nullptr) {
        baseEyeHeight = type->size().eyeHeight();
    }

    // 玩家实体根据姿态调整眼高
    // 玩家 type ID 包括 minecraft:player
    if (m_typeId == entity::EntityTypeKeys::PLAYER || m_typeId == "minecraft:player" || m_typeId == "player") {
        if (m_sleeping) {
            m_eyeHeight = 0.2f;
        } else if (m_swimming || isFallFlying()) {
            m_eyeHeight = 0.4f;
        } else if (m_sneaking) {
            m_eyeHeight = 1.27f;
        } else {
            m_eyeHeight = game::PLAYER_EYE_HEIGHT;
        }
        return;
    }

    // 幼年个体眼高减半（与 MC Java 的 AgeableEntity.getAgeScale() = 0.5 一致）
    if (m_child) {
        baseEyeHeight *= 0.5f;
    }

    m_eyeHeight = baseEyeHeight;
}

void ClientEntity::tickWitherSideHeads(const std::function<const ClientEntity*(EntityInstanceId)>& entityLookup)
{
    // 镜像 MC 1.21.11 WitherBoss.aiStep() 中 j=0..1 的侧头朝向计算。
    // 客户端不运行 WitherEntity::aiStep()（ClientEntity 是独立代理类），
    // 因此由 ClientEntityManager::tick() 对凋灵实体调用此方法。

    // 1. 备份 prev 值（对应 MC aiStep() 中 super.aiStep() 之后的
    //    yRotOHeads[i]=yRotHeads[i]; xRotOHeads[i]=xRotHeads[i];）
    for (i32 i = 0; i < 2; ++i) {
        m_prevWitherSideHeadYaw[i] = m_witherSideHeadYaw[i];
        m_prevWitherSideHeadPitch[i] = m_witherSideHeadPitch[i];
    }

    // 2. 计算侧头朝向
    // MC 1.21.11 getHeadX/Y/Z 使用 yBodyRot（Cubium 中为 renderYawOffset = m_yaw）。
    // 凋灵 getScale() 恒为 1.0（无幼体凋灵）。
    const f32 bodyRot = m_yaw; // renderYawOffset() == yaw() on ClientEntity

    for (i32 j = 0; j < 2; ++j) {
        // j=0 对应侧头 1（左头，HEAD_TARGET_2），j=1 对应侧头 2（右头，HEAD_TARGET_3）
        const i32 targetId = m_witherHeadTargetId[j + 1];
        const ClientEntity* targetEntity =
            (targetId > 0 && entityLookup) ? entityLookup(static_cast<EntityInstanceId>(targetId)) : nullptr;

        if (targetEntity != nullptr) {
            // 计算头部位置（对应 MC WitherBoss.getHeadX/Y/Z(j+1)）
            // head <= 0 为主头，此处 j+1 >= 1 为侧头
            const f32 headAngleDeg = bodyRot + 180.0f * static_cast<f32>(j); // j+1-1 = j
            const f32 headAngleRad = headAngleDeg * math::DEG_TO_RAD;
            const f64 headX = static_cast<f64>(m_position.x + std::cos(headAngleRad) * 1.3);
            const f64 headY = static_cast<f64>(m_position.y + 2.2); // 侧头 Y 偏移 2.2
            const f64 headZ = static_cast<f64>(m_position.z + std::sin(headAngleRad) * 1.3);

            // 目标相对头部的偏移
            const f64 dx = static_cast<f64>(targetEntity->x()) - headX;
            // MC: entity1.getEyeY() = entity1.getY() + entity1.getEyeHeight()
            const f64 dy = static_cast<f64>(targetEntity->y() + targetEntity->eyeHeight()) - headY;
            const f64 dz = static_cast<f64>(targetEntity->z()) - headZ;

            const f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

            // 偏航角：atan2(dz, dx) * 180/PI - 90
            const f32 targetYaw = static_cast<f32>(std::atan2(dz, dx) * (180.0 / math::PI) - 90.0);
            // 俯仰角：-(atan2(dy, horizontalDist) * 180/PI)
            const f32 targetPitch = static_cast<f32>(-(std::atan2(dy, horizontalDist) * (180.0 / math::PI)));

            // rotlerp: pitch 最大 40°/tick，yaw 最大 10°/tick
            m_witherSideHeadPitch[j] = math::clampedRotate(m_witherSideHeadPitch[j], targetPitch, 40.0f);
            m_witherSideHeadYaw[j] = math::clampedRotate(m_witherSideHeadYaw[j], targetYaw, 10.0f);
        } else {
            // 无目标：偏航角逐步回正到身体朝向（俯仰角保持不变，与 MC 一致）
            m_witherSideHeadYaw[j] = math::clampedRotate(m_witherSideHeadYaw[j], bodyRot, 10.0f);
        }
    }
}

} // namespace mc::client
