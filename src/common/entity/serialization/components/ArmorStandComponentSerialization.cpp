#include "common/entity/serialization/components/ArmorStandComponentSerialization.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/components/ArmorStandStateComponent.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <vector>

namespace mc::entity::serialization::components {

using namespace nbt_keys;

namespace {

/// 把 [x, y, z] 三元组写入姿态复合标签的指定子键
void _writePose(const char* key, const std::array<f32, 3>& pose, nbt::tags::compound_tag& poseTag)
{
    nbt_helper::putFloatList(poseTag, key, {pose[0], pose[1], pose[2]});
}

/// 从姿态复合标签读取指定子键到三元组（缺失或长度不为 3 时保持原值）
void _readPose(const nbt::tags::compound_tag& poseTag, const char* key, std::array<f32, 3>& pose)
{
    const std::vector<f32> values = nbt_helper::getFloatList(poseTag, key);
    if (values.size() != 3) {
        return;
    }
    pose = {values[0], values[1], values[2]};
}

} // namespace

// ============================================================================
// ArmorStandStateComponent — Invisible / Small / ShowArms / DisabledSlots / NoBasePlate / Pose
// 对应 MC 1.21.11 ArmorStand.addAdditionalSaveData/readAdditionalSaveData。
// 装备由 LivingEntity 层 EquipmentComponent 序列化器负责。
// ============================================================================

static void saveArmorStandState(const Entity& entity, nbt::tags::compound_tag& tag)
{
    auto* armorStand = dynamic_cast<const ArmorStandEntity*>(&entity);
    if (armorStand == nullptr) {
        return;
    }
    const auto& state = armorStand->armorStandState();
    tag.put(ARMOR_STAND_INVISIBLE, static_cast<i8>(state.m_invisible ? 1 : 0));
    tag.put(ARMOR_STAND_SMALL, static_cast<i8>(state.m_small ? 1 : 0));
    tag.put(ARMOR_STAND_SHOW_ARMS, static_cast<i8>(state.m_arms ? 1 : 0));
    tag.put(ARMOR_STAND_DISABLED_SLOTS, state.m_disabledSlots);
    tag.put(ARMOR_STAND_NO_BASE_PLATE, static_cast<i8>(state.m_basePlate ? 0 : 1));

    nbt::tags::compound_tag poseTag;
    _writePose(ARMOR_STAND_POSE_HEAD, state.m_pose.head, poseTag);
    _writePose(ARMOR_STAND_POSE_BODY, state.m_pose.body, poseTag);
    _writePose(ARMOR_STAND_POSE_LEFT_ARM, state.m_pose.leftArm, poseTag);
    _writePose(ARMOR_STAND_POSE_RIGHT_ARM, state.m_pose.rightArm, poseTag);
    _writePose(ARMOR_STAND_POSE_LEFT_LEG, state.m_pose.leftLeg, poseTag);
    _writePose(ARMOR_STAND_POSE_RIGHT_LEG, state.m_pose.rightLeg, poseTag);
    tag.value.emplace(ARMOR_STAND_POSE, std::make_unique<nbt::tags::compound_tag>(std::move(poseTag)));
}

static Result<void> loadArmorStandState(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* armorStand = dynamic_cast<ArmorStandEntity*>(&entity);
    if (armorStand == nullptr) {
        return {};
    }

    auto& state = armorStand->mutableArmorStandState();
    state.m_invisible = nbt_helper::tryGetBool(tag, ARMOR_STAND_INVISIBLE).value_or(false);
    state.m_small = nbt_helper::tryGetBool(tag, ARMOR_STAND_SMALL).value_or(false);
    state.m_arms = nbt_helper::tryGetBool(tag, ARMOR_STAND_SHOW_ARMS).value_or(false);
    state.m_disabledSlots = nbt_helper::tryGetInt(tag, ARMOR_STAND_DISABLED_SLOTS).value_or(0);
    state.m_basePlate = !nbt_helper::tryGetBool(tag, ARMOR_STAND_NO_BASE_PLATE).value_or(false);

    if (const auto* poseTag = nbt_helper::tryGetCompound(tag, ARMOR_STAND_POSE)) {
        _readPose(*poseTag, ARMOR_STAND_POSE_HEAD, state.m_pose.head);
        _readPose(*poseTag, ARMOR_STAND_POSE_BODY, state.m_pose.body);
        _readPose(*poseTag, ARMOR_STAND_POSE_LEFT_ARM, state.m_pose.leftArm);
        _readPose(*poseTag, ARMOR_STAND_POSE_RIGHT_ARM, state.m_pose.rightArm);
        _readPose(*poseTag, ARMOR_STAND_POSE_LEFT_LEG, state.m_pose.leftLeg);
        _readPose(*poseTag, ARMOR_STAND_POSE_RIGHT_LEG, state.m_pose.rightLeg);
    }

    // 读盘后把状态推送到客户端数据参数：反序列化发生在实体 spawn 之前（addEntity 绑定世界
    // 之前的裸实体阶段），客户端标志与姿态必须在下一追踪 tick 前就位，否则首帧 AddEntity
    // 携带的元数据会遗漏这些值。
    armorStand->syncClientState();
    return {};
}

void registerArmorStandComponentSerializers(ComponentSerializerRegistry& registry)
{
    registry.registerSerializer<ecs::ArmorStandStateComponent>(saveArmorStandState, loadArmorStandState);
}

} // namespace mc::entity::serialization::components