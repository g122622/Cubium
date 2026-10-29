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

#include "PatrollerEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/special/PatrolGoals.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <nlohmann/json.hpp>

namespace mc {

namespace {

/// 自然生成时「自己当上巡逻队长」的概率（vanilla `PatrollingMonster.finalizeSpawn` 的 0.06F）。
constexpr f32 WILD_LEADER_CHANCE = 0.06f;

} // namespace

// ============================================================================
// 继承链标识（parent = MonsterEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& PatrollerEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"PatrollerEntity", &MonsterEntity::classInfo()};
    return s_classInfo;
}

PatrollerEntity::PatrollerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{}

void PatrollerEntity::setPatrolTarget(const BlockPos& patrolTarget)
{
    m_patrolTarget = patrolTarget;
    m_isPatrolling = true;
}

const BlockPos& PatrollerEntity::getPatrolTarget() const
{
    MC_ASSERT_RELEASE(m_patrolTarget.has_value());
    return *m_patrolTarget;
}

void PatrollerEntity::setLeader(bool isLeader)
{
    m_isPatrolLeader = isLeader;
    m_isPatrolling = true;
}

void PatrollerEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    MonsterEntity::finalizeSpawn(world, difficulty, spawnReason);

    // 对齐 MC 1.21.11 PatrollingMonster.finalizeSpawn：巡逻队（PATROL）、袭击波次（EVENT）
    // 与结构生成（STRUCTURE）不掷「野生队长」骰，其余原因（自然生成、刷怪笼、繁殖……）
    // 有 6% 概率让一个够格的袭掠者自己当上巡逻队长。
    if (spawnReason != world::spawn::SpawnReason::Patrol && spawnReason != world::spawn::SpawnReason::Event &&
        spawnReason != world::spawn::SpawnReason::Structure && getRandom().nextFloat() < WILD_LEADER_CHANCE &&
        canBeLeader()) {
        m_isPatrolLeader = true;
    }

    // 原版在此处给队长戴上不祥旗帜（并把头盔掉落率设为 200%），无论队长标记是这次掷出来
    // 的还是调用方先前设好的（巡逻队/袭击波次的队长都由调用方设定）。
    if (m_isPatrolLeader) {
        equipOminousBanner();
        // TODO(raider-drop-chance): 原版同时执行 setDropChance(HEAD, 2.0F)；Cubium 的
        //   装备掉落率接口尚未接到实体上，旗帜掉率目前仍是默认值。
    }
}

void PatrollerEntity::equipOminousBanner()
{
    // 对齐 MC 1.21.11 `Raid.getOminousBannerInstance`：白底旗帜 + 固定 7 层图案。
    const nlohmann::json layers = nlohmann::json::array({
        {{"pattern", "minecraft:rhombus"}, {"color", "cyan"}},
        {{"pattern", "minecraft:stripe_bottom"}, {"color", "light_gray"}},
        {{"pattern", "minecraft:stripe_center"}, {"color", "gray"}},
        {{"pattern", "minecraft:border"}, {"color", "light_gray"}},
        {{"pattern", "minecraft:stripe_middle"}, {"color", "black"}},
        {{"pattern", "minecraft:half_horizontal"}, {"color", "light_gray"}},
        {{"pattern", "minecraft:circle"}, {"color", "light_gray"}},
        {{"pattern", "minecraft:border"}, {"color", "black"}},
    });

    ItemStack banner(Items::WHITE_BANNER, 1);
    banner.setComponent(item::component::DataComponentType::BannerPatterns,
        item::component::DataComponentPayload{
            std::in_place_index<item::component::ComponentPayloadIndex::Json>, layers});
    setEquipment(EquipmentSlot::Head, banner);
}

bool PatrollerEntity::wearsOminousBanner() const
{
    const ItemStack& head = getEquipment(EquipmentSlot::Head);
    if (head.isEmpty() || head.getItem() != Items::WHITE_BANNER) {
        return false;
    }
    // 原版用 `ItemStack.matches` 比对整份不祥旗帜（白底 + 8 层固定图案 + 名字 + 稀有度）。
    // 我们的旗帜是同一份数据造出来的，因此这里只要确认「带图案」，就等价于戴了不祥旗帜；
    // 未经染色的普通白旗没有 banner_patterns 组件。
    return head.hasComponent(item::component::DataComponentType::BannerPatterns);
}

void PatrollerEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    MonsterEntity::addAdditionalSaveData(tag);
    // 对齐 MC 1.21.11 PatrollingMonster.addAdditionalSaveData 的 PatrolLeader/Patrolling。
    // NBT 没有布尔标签，原版 putBoolean 写的就是 byte。
    tag.put("PatrolLeader", static_cast<std::int8_t>(m_isPatrolLeader ? 1 : 0));
    tag.put("Patrolling", static_cast<std::int8_t>(m_isPatrolling ? 1 : 0));
}

Result<void> PatrollerEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(MonsterEntity::readAdditionalSaveData(tag));
    // 对齐 MC 1.21.11 PatrollingMonster.readAdditionalSaveData：这里直接写字段而不是走
    // setLeader()——后者会顺带把 patrolling 置真，与存档里独立的 Patrolling 值冲突。
    m_isPatrolLeader = entity::serialization::nbt_helper::tryGetBool(tag, "PatrolLeader").value_or(false);
    m_isPatrolling = entity::serialization::nbt_helper::tryGetBool(tag, "Patrolling").value_or(false);
    return {};
}

void PatrollerEntity::resetPatrolTarget()
{
    auto& random = getRandom();
    const BlockPos currentPos(position());
    setPatrolTarget(currentPos + BlockPos(-500 + random.nextInt(1000), 0, -500 + random.nextInt(1000)));
}

void PatrollerEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 队员速度 0.7，队长速度 0.595（队长较慢以便队员跟随）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::PatrolGoal>(this, 0.7, 0.595));
}

} // namespace mc
