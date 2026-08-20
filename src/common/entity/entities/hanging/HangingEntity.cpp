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

#include "HangingEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/explosion/ExplosionImmunityContext.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace entity {

// ==================== HangingEntity ====================

HangingEntity::HangingEntity(ecs::EntityRegistry& registry)
    : Entity(EntityInstanceId(0), nullptr, registry)
{}

HangingEntity::HangingEntity(BlockPos pos, Direction direction, ecs::EntityRegistry& registry)
    : Entity(EntityInstanceId(0), nullptr, registry)
    , m_hangingPos(pos)
    , m_direction(direction)
{
    updateBoundingBox();
}

void HangingEntity::tick()
{
    Entity::tick();

    // 定期检查悬挂位置是否有效
    if (++m_checkInterval >= CHECK_INTERVAL) {
        m_checkInterval = 0;
        if (!isValidPosition()) {
            dropItem();
            remove();
        }
    }
}

void HangingEntity::setHangingPosition(BlockPos pos, Direction direction)
{
    m_hangingPos = pos;
    m_direction = direction;
    updateBoundingBox();
}

bool HangingEntity::isValidPosition() const
{
    return canPlaceOn();
}

bool HangingEntity::canPlaceOn() const
{
    // 检查背后的方块是否可以支撑悬挂实体
    if (m_world == nullptr) {
        return false;
    }

    // 获取悬挂方向对应的 MC Direction
    // HangingEntity::Direction: SOUTH=0, WEST=1, NORTH=2, EAST=3
    // 悬挂实体面向 SOUTH 时，背面是 NORTH，需要检查 NORTH 面是否可依附
    mc::Direction attachDir;
    switch (m_direction) {
        case Direction::SOUTH:
            attachDir = mc::Direction::North; // 面向南方，背面是北方
            break;
        case Direction::WEST:
            attachDir = mc::Direction::East; // 面向西方，背面是东方
            break;
        case Direction::NORTH:
            attachDir = mc::Direction::South; // 面向北方，背面是南方
            break;
        case Direction::EAST:
            attachDir = mc::Direction::West; // 面向东方，背面是西方
            break;
        default:
            return false;
    }

    // 计算支撑方块的位置（悬挂位置的背后）
    BlockPos attachPos = m_hangingPos.offset(attachDir);

    // 检查支撑方块是否有足够的固体面
    // 使用 Direction::opposite 获取我们面对的方向
    mc::Direction solidCheckDir = Directions::opposite(attachDir);
    return Block::hasEnoughSolidSide(*m_world, attachPos, solidCheckDir);
}

bool HangingEntity::hurt(DamageSource& source, f32 /*amount*/)
{
    // 悬挂实体被任何伤害一击即毁

    // 1. 检查无敌状态
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 2. 检查 mobGriefing 游戏规则：如果伤害来源是 Mob 且 mobGriefing 关闭，则不受伤害
    if (m_world != nullptr) {
        Entity* sourceEntity = source.getEntity();
        if (sourceEntity != nullptr && dynamic_cast<MobEntity*>(sourceEntity) != nullptr) {
            if (!m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
                return false;
            }
        }
    }

    // 3. 如果未被移除，销毁悬挂实体并掉落物品
    if (!isRemoved()) {
        dropItem();
        remove();
        markHurt();
    }

    return true;
}

bool HangingEntity::ignoreExplosion(const world::explosion::ExplosionImmunityContext& ctx) const
{
    // 直接源在水中的爆炸不破坏悬挂实体（如水下 TNT 不毁画框）。
    if (ctx.directSource != nullptr && ctx.directSource->isInWater()) {
        return true;
    }
    // 否则仅当爆炸影响方块类实体时才受影响。
    return ctx.shouldAffectBlocklikeEntities ? Entity::ignoreExplosion(ctx) : true;
}

void HangingEntity::updateBoundingBox()
{
    // 根据方向和尺寸更新边界框
    // 设置位置
    setPosition(static_cast<f64>(m_hangingPos.x) + 0.5,
        static_cast<f64>(m_hangingPos.y) + 0.5,
        static_cast<f64>(m_hangingPos.z) + 0.5);

    // 设置朝向
    switch (m_direction) {
        case Direction::SOUTH:
            setRotation(0.0f, 0.0f);
            break;
        case Direction::WEST:
            setRotation(90.0f, 0.0f);
            break;
        case Direction::NORTH:
            setRotation(180.0f, 0.0f);
            break;
        case Direction::EAST:
            setRotation(270.0f, 0.0f);
            break;
    }
}

// ==================== PaintingEntity ====================

const std::vector<PaintingEntity::PaintingType> PaintingEntity::PAINTING_TYPES = {{"Kebab", 1, 1},
    {"Aztec", 1, 1},
    {"Alban", 1, 1},
    {"Aztec2", 1, 1},
    {"Bomb", 1, 1},
    {"Plant", 1, 1},
    {"Wasteland", 1, 1},
    {"Pool", 2, 1},
    {"Courbet", 2, 1},
    {"Sea", 2, 1},
    {"Sunset", 2, 1},
    {"Creebet", 2, 1},
    {"Wanderer", 1, 2},
    {"Graham", 1, 2},
    {"Match", 2, 2},
    {"Bust", 2, 2},
    {"Stage", 2, 2},
    {"Void", 2, 2},
    {"SkullAndRoses", 2, 2},
    {"Wither", 2, 2},
    {"Fighters", 4, 2},
    {"Skeleton", 4, 3},
    {"DonkeyKong", 4, 3},
    {"Pointer", 4, 4},
    {"Pigscene", 4, 4},
    {"BurningSkull", 4, 4},
    {"Skeleton2", 3, 4},
    {"Bust2", 3, 4}};

std::unique_ptr<Entity> PaintingEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PaintingEntity>(registry);
}

PaintingEntity::PaintingEntity(ecs::EntityRegistry& registry)
    : HangingEntity(registry)
{}

PaintingEntity::PaintingEntity(
    BlockPos pos, Direction direction, const std::string& motive, ecs::EntityRegistry& registry)
    : HangingEntity(pos, direction, registry)
{
    setMotive(motive);
}

void PaintingEntity::dropItem()
{
    // 生成画作物品
    if (m_world == nullptr) {
        return;
    }

    // 检查游戏规则 doEntityDrops：当该规则为 false 时，画被破坏不产生掉落物品
    if (!m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
        return;
    }

    // 创建画作物品堆
    if (Items::PAINTING != nullptr) {
        ItemStack stack(*Items::PAINTING, 1);
        math::Random& rng = m_world->getRandom();
        ItemDropHelper::spawnItemEntity(m_world, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
    }
}

i32 PaintingEntity::getWidth() const
{
    for (const auto& type : PAINTING_TYPES) {
        if (type.name == m_motive) {
            return type.width;
        }
    }
    return 1;
}

i32 PaintingEntity::getHeight() const
{
    for (const auto& type : PAINTING_TYPES) {
        if (type.name == m_motive) {
            return type.height;
        }
    }
    return 1;
}

void PaintingEntity::setMotive(const std::string& motive)
{
    m_motive = motive;
    updateBoundingBox();
}

// ==================== ItemFrameEntity ====================

std::unique_ptr<Entity> ItemFrameEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ItemFrameEntity>(registry);
}

ItemFrameEntity::ItemFrameEntity(ecs::EntityRegistry& registry)
    : HangingEntity(registry)
{}

ItemFrameEntity::ItemFrameEntity(BlockPos pos, Direction direction, ecs::EntityRegistry& registry)
    : HangingEntity(pos, direction, registry)
{}

void ItemFrameEntity::tick()
{
    HangingEntity::tick();
    // 物品展示框不需要特殊的 tick 逻辑
    // ItemStack 是值类型，不需要检查存活状态
}

ActionResultType ItemFrameEntity::processInitialInteract(Player& player, Hand hand)
{
    if (m_world != nullptr && !m_world->isClientSide()) {
        ItemStack& heldItem = player.getHeldItem(hand);

        if (m_displayedItem.isEmpty()) {
            // 展示框为空：放入玩家手中的物品
            if (!heldItem.isEmpty()) {
                setDisplayedItem(heldItem, true);
                if (!player.abilities().creativeMode) {
                    heldItem.shrink(1);
                }
                m_world->gameEvent(
                    gameevent::GameEvents::BLOCK_CHANGE, m_hangingPos, gameevent::GameEvent::Context::of(&player));
                return ActionResultType::Success;
            }
        } else if (player.isSneaking()) {
            // 潜行+右键有物品的展示框：取出物品
            if (m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
                math::Random& rng = m_world->getRandom();
                ItemDropHelper::spawnItemEntity(m_world, m_displayedItem, x(), y(), z(), rng);
            }
            setDisplayedItem(ItemStack(), true);
            m_world->gameEvent(
                gameevent::GameEvents::BLOCK_CHANGE, m_hangingPos, gameevent::GameEvent::Context::of(&player));
            return ActionResultType::Success;
        } else {
            // 右键有物品的展示框（不潜行）：旋转物品
            setItemRotation(m_rotation + 1, true);
            m_world->gameEvent(
                gameevent::GameEvents::BLOCK_CHANGE, m_hangingPos, gameevent::GameEvent::Context::of(&player));
            return ActionResultType::Success;
        }
    }

    // 客户端直接返回成功
    if (m_world != nullptr && m_world->isClientSide()) {
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

void ItemFrameEntity::dropItem()
{
    // 检查游戏规则 doEntityDrops
    // 当 doEntityDrops 为 false 时不掉落任何物品，当为 true 时掉落物品展示框和内含物品
    if (m_world != nullptr && m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
        // 掉落物品展示框本身
        if (Items::ITEM_FRAME != nullptr) {
            ItemStack frameStack(*Items::ITEM_FRAME, 1);
            math::Random& rng = m_world->getRandom();
            ItemDropHelper::spawnItemEntity(
                m_world, frameStack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
        }

        // 掉落展示框内的物品
        if (!m_displayedItem.isEmpty()) {
            math::Random& rng = m_world->getRandom();
            ItemDropHelper::spawnItemEntity(m_world, m_displayedItem, x(), y(), z(), rng);
        }
    }

    // 无论 doEntityDrops 是否为 true，都清空展示物品
    m_displayedItem = ItemStack();

    // 展示框内容变化，通知红石比较器更新
    notifyComparatorUpdate();

    // 触发方块变化游戏事件（幽匿感测体等可感知）
    if (m_world != nullptr) {
        m_world->gameEvent(gameevent::GameEvents::BLOCK_CHANGE, m_hangingPos, nullptr);
    }
}

void ItemFrameEntity::setDisplayedItem(const ItemStack& stack, bool updateComparator)
{
    if (!stack.isEmpty()) {
        // 复制物品堆并设置数量为1
        m_displayedItem = stack;
        m_displayedItem.setCount(1);
    } else {
        m_displayedItem = ItemStack();
    }
    // 旋转重置为0
    m_rotation = 0;

    if (updateComparator) {
        notifyComparatorUpdate();
    }
}

void ItemFrameEntity::setItemRotation(i32 rotation, bool updateComparator)
{
    // 旋转值限制在 0-7 范围内
    m_rotation = rotation % 8;
    if (m_rotation < 0) {
        m_rotation += 8;
    }

    if (updateComparator) {
        notifyComparatorUpdate();
    }
}

void ItemFrameEntity::rotateItem()
{
    // 右键交互时旋转物品
    setItemRotation(m_rotation + 1, true);
}

void ItemFrameEntity::notifyComparatorUpdate()
{
    // 通知悬挂位置周围的红石比较器重新计算输入信号
    // 物品展示框的内容变化（放入/取出/旋转物品）会影响比较器输出
    if (m_world != nullptr) {
        world::redstone::RedstoneSystem::instance().updateComparators(*m_world, m_hangingPos);
    }
}

i32 ItemFrameEntity::getAnalogOutput() const
{
    // 无物品时返回 0，有物品时返回 rotation % 8 + 1
    return m_displayedItem.isEmpty() ? 0 : (m_rotation % 8 + 1);
}

mc::Direction ItemFrameEntity::getHorizontalFacing() const
{
    // 将内部方向转换为 mc::Direction
    // HangingEntity::Direction: SOUTH=0, WEST=1, NORTH=2, EAST=3
    // mc::Direction: North=2, South=3, West=4, East=5
    switch (m_direction) {
        case HangingEntity::Direction::SOUTH:
            return mc::Direction::South;
        case HangingEntity::Direction::WEST:
            return mc::Direction::West;
        case HangingEntity::Direction::NORTH:
            return mc::Direction::North;
        case HangingEntity::Direction::EAST:
            return mc::Direction::East;
        default:
            return mc::Direction::South;
    }
}

// ==================== LeashKnotEntity ====================

std::unique_ptr<Entity> LeashKnotEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<LeashKnotEntity>(registry);
}

LeashKnotEntity::LeashKnotEntity(ecs::EntityRegistry& registry)
    : HangingEntity(registry)
{}

LeashKnotEntity::LeashKnotEntity(BlockPos pos, Direction direction, ecs::EntityRegistry& registry)
    : HangingEntity(pos, direction, registry)
{}

LeashKnotEntity* LeashKnotEntity::getOrCreateKnot(IWorld& world, const BlockPos& pos)
{
    // 搜索该位置附近是否已有拴绳结实体
    Vector3 centerPos(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f);
    auto entities = world.getEntitiesInRange(centerPos, 2.0f);

    for (Entity* entity : entities) {
        auto* knot = dynamic_cast<LeashKnotEntity*>(entity);
        if (knot != nullptr && knot->isAlive()) {
            BlockPos knotHangingPos = knot->getHangingBlockPos();
            if (knotHangingPos == pos) {
                return knot;
            }
        }
    }

    // 创建新的拴绳结
    // 经 IWorld::entityRegistry() 取 ECS 注册表（服务端非空，客户端返回 nullptr）
    auto* registry = world.entityRegistry();
    MC_ASSERT_RELEASE(registry != nullptr);
    auto newKnot = std::make_unique<LeashKnotEntity>(pos, HangingEntity::Direction::SOUTH, *registry);
    // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
    newKnot->setTypeId(EntityTypeKeys::LEASH_KNOT);
    auto* rawPtr = newKnot.get();
    EntityInstanceId id = world.spawnEntity(std::move(newKnot));
    if (id == INVALID_ENTITY_ID) {
        return nullptr;
    }

    // 播放放置音效
    rawPtr->playPlacementSound();
    return rawPtr;
}

ActionResultType LeashKnotEntity::interact(Player& player, Hand hand)
{
    if (m_world != nullptr && !m_world->isClientSide()) {
        bool transferredToKnot = false;
        bool transferredToPlayer = false;

        // 将玩家手中拴着的生物转移到栅栏结上
        ItemStack& heldItem = player.getHeldItem(hand);
        if (heldItem.getItem() != nullptr && heldItem.getItem() == Items::LEAD) {
            // 搜索被当前玩家拴住的生物
            Vector3 centerPos(m_hangingPos.x + 0.5f, m_hangingPos.y + 0.5f, m_hangingPos.z + 0.5f);
            auto entities = m_world->getEntitiesInRange(centerPos, 16.0f);

            for (Entity* entity : entities) {
                auto* mob = dynamic_cast<MobEntity*>(entity);
                if (mob == nullptr || !mob->isAlive()) {
                    continue;
                }

                if (!mob->isLeashed()) {
                    continue;
                }

                const auto& holderUuid = mob->leashHolderUuid();
                if (!holderUuid.has_value() || *holderUuid != player.uuid()) {
                    continue;
                }

                // 检查距离
                constexpr f64 MAX_LEASH_DISTANCE = 12.0;
                Vector3d mobPos(mob->x(), mob->y(), mob->z());
                Vector3d knotPos(m_hangingPos.x + 0.5, m_hangingPos.y + 0.5, m_hangingPos.z + 0.5);
                f64 distance = mobPos.distance(knotPos);
                if (distance > MAX_LEASH_DISTANCE) {
                    continue;
                }

                if (!mob->canBeLeashed()) {
                    continue;
                }

                // 将生物从拴在玩家身上改为拴在栅栏结上
                mob->setLeashedToFence(m_hangingPos);
                attachLeash(mob);
                transferredToKnot = true;
            }
        }

        // 如果没有转移到栅栏结，且玩家不潜行，将栅栏结上的生物取回
        if (!transferredToKnot && !player.isSneaking()) {
            for (auto it = m_leashedEntities.begin(); it != m_leashedEntities.end();) {
                auto* mob = dynamic_cast<MobEntity*>(*it);
                if (mob != nullptr && mob->isAlive() && mob->canBeLeashed()) {
                    // 将生物从栅栏结转移到玩家身上
                    mob->setLeashedToEntity(player.uuid());
                    it = m_leashedEntities.erase(it);
                    transferredToPlayer = true;
                } else {
                    ++it;
                }
            }
        }

        if (transferredToKnot || transferredToPlayer) {
            m_world->gameEvent(gameevent::GameEvents::BLOCK_ATTACH, m_hangingPos, nullptr);
            playSound(SoundEvents::ENTITY_LEASH_KNOT_PLACE, 1.0f, 1.0f);
            return ActionResultType::Success;
        }
    }

    // 客户端直接返回成功
    if (m_world != nullptr && m_world->isClientSide()) {
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

ActionResultType LeashKnotEntity::processInitialInteract(Player& player, Hand hand)
{
    return interact(player, hand);
}

void LeashKnotEntity::tick()
{
    HangingEntity::tick();

    // 检查栅栏方块是否仍然存在，如果栅栏被破坏则拴绳结也应销毁
    if (!survives()) {
        // 将绑定的生物释放回自由状态（不掉落拴绳，因为栅栏被破坏时应掉落）
        for (Entity* entity : m_leashedEntities) {
            auto* mob = dynamic_cast<MobEntity*>(entity);
            if (mob != nullptr && mob->isAlive()) {
                mob->dropLeash();
            }
        }
        m_leashedEntities.clear();
        dropItem();
        remove();
        return;
    }

    // 检查绑定的实体
    for (auto it = m_leashedEntities.begin(); it != m_leashedEntities.end();) {
        if (!(*it)->isAlive()) {
            it = m_leashedEntities.erase(it);
        } else {
            ++it;
        }
    }

    // 如果没有绑定的实体，移除自己
    if (m_leashedEntities.empty()) {
        dropItem();
        remove();
    }
}

void LeashKnotEntity::dropItem()
{
    // 掉落拴绳物品
    if (m_world == nullptr) {
        return;
    }

    // 检查游戏规则 doEntityDrops：当该规则为 false 时，拴绳结被破坏不产生掉落物品
    if (!m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
        return;
    }

    // 创建拴绳物品堆
    if (Items::LEAD != nullptr) {
        ItemStack stack(*Items::LEAD, 1);
        math::Random& rng = m_world->getRandom();
        ItemDropHelper::spawnItemEntity(m_world, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
    }
}

void LeashKnotEntity::attachLeash(Entity* entity)
{
    if (entity && std::find(m_leashedEntities.begin(), m_leashedEntities.end(), entity) == m_leashedEntities.end()) {
        m_leashedEntities.push_back(entity);
    }
}

void LeashKnotEntity::detachLeash(Entity* entity)
{
    auto it = std::find(m_leashedEntities.begin(), m_leashedEntities.end(), entity);
    if (it != m_leashedEntities.end()) {
        m_leashedEntities.erase(it);
    }
}

bool LeashKnotEntity::survives() const
{
    if (m_world == nullptr) {
        return false;
    }
    const BlockState* state = m_world->getBlockState(m_hangingPos);
    return state != nullptr && BlockTags::FENCES().contains(*state);
}

void LeashKnotEntity::playPlacementSound()
{
    if (m_world != nullptr) {
        playSound(SoundEvents::ENTITY_LEASH_KNOT_PLACE, 1.0f, 1.0f);
    }
}

} // namespace entity
} // namespace mc
