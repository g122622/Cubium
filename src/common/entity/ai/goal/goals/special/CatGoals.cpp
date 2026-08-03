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

#include "CatGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/special/MoveToBlockGoal.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/entities/passive/tamable/CatEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include <cmath>
#include <string>

namespace mc::entity::ai::goal {

using namespace CatGoalConstants;

// ============================================================================
// CatLieOnBedGoal
// ============================================================================

CatLieOnBedGoal::CatLieOnBedGoal(CatEntity* cat, f64 speed)
    : MoveToBlockGoal(static_cast<CreatureEntity*>(cat), speed, LIE_ON_BED_SEARCH_RANGE, /*verticalSearchRange=*/2)
    , m_cat(cat)
{
    m_verticalSearchStart = LIE_ON_BED_VERTICAL_START;
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Jump, GoalFlag::Move});
}

bool CatLieOnBedGoal::shouldExecute()
{
    // 只有驯服且未坐下的猫才会寻找床
    if (m_cat == nullptr || !m_cat->isTamed() || m_cat->isSitting() || m_cat->isLieDown()) {
        return false;
    }
    return MoveToBlockGoal::shouldExecute();
}

void CatLieOnBedGoal::startExecuting()
{
    MoveToBlockGoal::startExecuting();
    // 猫在走向床的过程中不应该保持坐姿
    m_cat->setSitting(false);
}

void CatLieOnBedGoal::tick()
{
    MoveToBlockGoal::tick();

    if (isWithinDistance(m_destinationBlock, getTargetDistanceSq())) {
        // 到达床：设置躺下状态
        if (!m_cat->isLieDown()) {
            m_cat->setLieDown(true);
        }
    } else {
        // 未到达床：取消躺下状态
        m_cat->setLieDown(false);
    }
}

void CatLieOnBedGoal::resetTask()
{
    MoveToBlockGoal::resetTask();
    m_cat->setLieDown(false);
}

bool CatLieOnBedGoal::shouldMoveTo(IWorld* world, const BlockPos& pos)
{
    // 检查该位置是否是床方块
    if (!blocks::BedBlock::isBed(*world, pos)) {
        return false;
    }
    // 检查床上方是否有空间
    const BlockState* aboveState = world->getBlockState(pos.up());
    return aboveState != nullptr && !aboveState->isSolid();
}

i32 CatLieOnBedGoal::nextStartTick() const
{
    // 猫躺在床上的重新尝试间隔（40 tick + 随机延迟）
    return LIE_ON_BED_MOVE_INTERVAL + m_cat->getRandom().nextInt(LIE_ON_BED_MOVE_INTERVAL);
}

// ============================================================================
// CatRelaxOnOwnerGoal
// ============================================================================

CatRelaxOnOwnerGoal::CatRelaxOnOwnerGoal(CatEntity* cat, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Jump, GoalFlag::Move, GoalFlag::Look})
    , m_cat(cat)
    , m_speed(speed)
{}

bool CatRelaxOnOwnerGoal::shouldExecute()
{
    if (m_cat == nullptr || !m_cat->isTamed() || m_cat->isSitting()) {
        return false;
    }

    // 获取主人
    Player* owner = m_cat->getOwner();
    if (owner == nullptr || !owner->isAlive() || !owner->isSleeping()) {
        return false;
    }

    // 检查距离
    f64 distSq = m_cat->distanceSqTo(*owner);
    if (distSq >= RELAX_ON_OWNER_SEARCH_DIST_SQ) {
        return false;
    }

    // 检查主人位置是否有床
    auto sleepPos = owner->getSleepingPosition();
    if (!sleepPos.has_value() || m_cat->world() == nullptr) {
        return false;
    }
    if (!blocks::BedBlock::isBed(*m_cat->world(), sleepPos.value())) {
        return false;
    }

    // 计算目标位置：床的对面方向
    Direction bedFacing = blocks::BedBlock::getBedOrientation(*m_cat->world(), sleepPos.value());
    if (bedFacing == Direction::None) {
        return false;
    }
    Direction oppositeFacing = Directions::opposite(bedFacing);
    m_goalPos = sleepPos.value().offset(oppositeFacing);

    m_owner = owner;

    // 检查目标位置附近是否有其他猫占据
    return !_isSpaceOccupied();
}

bool CatRelaxOnOwnerGoal::shouldContinueExecuting()
{
    if (m_cat == nullptr || !m_cat->isTamed() || m_cat->isSitting()) {
        return false;
    }
    if (m_owner == nullptr || !m_owner->isAlive() || !m_owner->isSleeping()) {
        return false;
    }
    // 检查是否被其他猫占据
    return !_isSpaceOccupied();
}

void CatRelaxOnOwnerGoal::startExecuting()
{
    // 猫在走向主人过程中不应该保持坐姿
    m_cat->setSitting(false);
    m_onBedTicks = 0;
    // 导航到目标位置
    if (m_cat->navigator() != nullptr) {
        (void)m_cat->navigator()->moveTo(static_cast<f64>(m_goalPos.x) + 0.5,
            static_cast<f64>(m_goalPos.y),
            static_cast<f64>(m_goalPos.z) + 0.5,
            m_speed);
    }
}

void CatRelaxOnOwnerGoal::tick()
{
    if (m_cat == nullptr || m_owner == nullptr) {
        return;
    }

    // 导航到目标位置
    if (m_cat->navigator() != nullptr && m_cat->navigator()->noPath()) {
        (void)m_cat->navigator()->moveTo(static_cast<f64>(m_goalPos.x) + 0.5,
            static_cast<f64>(m_goalPos.y),
            static_cast<f64>(m_goalPos.z) + 0.5,
            m_speed);
    }

    // 检查与主人的距离
    f64 distSq = m_cat->distanceSqTo(*m_owner);

    if (distSq < RELAX_ON_OWNER_NEAR_DIST_SQ) {
        // 猫在主人附近
        ++m_onBedTicks;

        if (m_onBedTicks > RELAX_ON_OWNER_DELAY) {
            // 超过延迟时间：设置躺下状态，取消放松状态
            m_cat->setLieDown(true);
            m_cat->setRelaxStateOne(false);
        } else {
            // 延迟期间：看向主人，设置放松状态
            m_cat->lookAt(*m_owner, 45.0f, 45.0f);
            m_cat->setRelaxStateOne(true);
        }
    } else {
        // 不在主人附近：取消躺下状态
        m_cat->setLieDown(false);
    }
}

void CatRelaxOnOwnerGoal::resetTask()
{
    // 目标结束时，如果主人已充分睡眠（>= 100 tick），赠送晨间礼物
    if (m_owner != nullptr && m_owner->isPlayerFullyAsleep()) {
        _giveMorningGift();
    }

    m_cat->setLieDown(false);
    m_cat->setRelaxStateOne(false);
    m_onBedTicks = 0;
    m_owner = nullptr;

    // 停止导航
    if (m_cat->navigator() != nullptr) {
        m_cat->navigator()->clearPath();
    }
}

bool CatRelaxOnOwnerGoal::_isSpaceOccupied() const
{
    if (m_cat == nullptr || m_cat->world() == nullptr) {
        return true;
    }

    // 检查目标位置附近是否有其他猫
    AxisAlignedBB searchBox = m_cat->boundingBox().grow(SPACE_OCCUPIED_CHECK_DIST);
    auto entities = m_cat->world()->getEntitiesInAABB(searchBox);

    for (auto* entity : entities) {
        if (entity == nullptr || entity == m_cat) {
            continue;
        }
        auto* otherCat = dynamic_cast<CatEntity*>(entity);
        if (otherCat != nullptr && (otherCat->isLieDown() || otherCat->isRelaxStateOne())) {
            // 检查距离
            f64 distSq = otherCat->distanceSqTo(
                static_cast<f64>(m_goalPos.x), static_cast<f64>(m_goalPos.y), static_cast<f64>(m_goalPos.z));
            if (distSq < static_cast<f64>(SPACE_OCCUPIED_CHECK_DIST * SPACE_OCCUPIED_CHECK_DIST)) {
                return true;
            }
        }
    }
    return false;
}

void CatRelaxOnOwnerGoal::_giveMorningGift()
{
    if (m_cat == nullptr || m_cat->world() == nullptr) {
        return;
    }

    // 检查主人是否已充分睡眠（睡眠计时器 >= 100 tick）
    // MC 原版：主人睡眠时间足够长才赠送礼物
    if (m_owner != nullptr && !m_owner->isPlayerFullyAsleep()) {
        return;
    }

    // 概率检查
    if (m_cat->getRandom().nextFloat() >= MORNING_GIFT_CHANCE) {
        return;
    }

    // 尝试使用战利品表系统生成礼物
    const loot::LootTableManager* ltm = m_cat->world()->lootTableManager();
    if (ltm != nullptr) {
        const loot::LootTable* table = ltm->getTable("minecraft:gameplay/cat_morning_gift");
        if (table != nullptr) {
            // 构建战利品上下文
            BlockPos catPos(static_cast<i32>(std::floor(m_cat->x())),
                static_cast<i32>(std::floor(m_cat->y())),
                static_cast<i32>(std::floor(m_cat->z())));

            auto context = loot::LootContextBuilder(*m_cat->world())
                               .withRandom(m_cat->getRandom())
                               .withParameter(loot::LootParams::THIS_ENTITY, static_cast<Entity*>(m_cat))
                               .withOwnedValue(loot::LootParams::BLOCK_POS, catPos)
                               .withLootTableResolver(
                                   [ltm](const std::string& id) -> const loot::LootTable* { return ltm->getTable(id); })
                               .withPredicateResolver([ltm](const std::string& id) -> const loot::LootCondition* {
                                   return ltm->getPredicate(id);
                               })
                               .build(loot::LootParameterSets::gift());

            if (context != nullptr) {
                // 生成战利品并掉落
                auto drops = table->generate(*context);
                for (const auto& stack : drops) {
                    if (!stack.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(m_cat, stack, 0.5f, m_cat->getRandom());
                    }
                }
                return;
            }
        }
    }

    // 战利品表不可用时的后备方案：从预定义列表中随机选择礼物
    // 对应 MC 原版 cat_morning_gift 战利品表中的物品：
    //   rabbit_hide(10), rabbit_foot(10), chicken(10), feather(10),
    //   rotten_flesh(10), string(10), phantom_membrane(2)
    struct GiftItem {
        const Item* item;
        i32 weight;
    };
    const GiftItem giftItems[] = {
        {Items::RABBIT_HIDE, 10},     // 兔子皮
        {Items::RABBIT_FOOT, 10},     // 兔子脚
        {Items::CHICKEN, 10},         // 生鸡肉
        {Items::FEATHER, 10},         // 羽毛
        {Items::ROTTEN_FLESH, 10},    // 腐肉
        {Items::STRING, 10},          // 线
        {Items::PHANTOM_MEMBRANE, 2}, // 幻翼膜
    };

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& gi : giftItems) {
        if (gi.item != nullptr) {
            totalWeight += gi.weight;
        }
    }

    if (totalWeight > 0) {
        i32 roll = m_cat->getRandom().nextInt(totalWeight);
        i32 cumulative = 0;
        for (const auto& gi : giftItems) {
            if (gi.item == nullptr) {
                continue;
            }
            cumulative += gi.weight;
            if (roll < cumulative) {
                ItemStack stack(gi.item, 1);
                ItemDropHelper::spawnItemAtEntity(m_cat, stack, 0.5f, m_cat->getRandom());
                return;
            }
        }
    }
}

} // namespace mc::entity::ai::goal
