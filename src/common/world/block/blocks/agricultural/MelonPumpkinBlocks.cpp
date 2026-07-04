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

#include "MelonPumpkinBlocks.hpp"
#include "../../../../entity/combat/DifficultyInstance.hpp"
#include "../../../../entity/core/EntityRegistry.hpp"
#include "../../../../entity/core/MobEntity.hpp"
#include "../../../../entity/core/VanillaEntities.hpp"
#include "../../../../entity/entities/item/ItemEntity.hpp"
#include "../../../../entity/entities/passive/golem/IronGolemEntity.hpp"
#include "../../../../entity/entities/passive/golem/SnowGolemEntity.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../../WorldEvents.hpp"
#include "../../BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>

namespace mc {
namespace blocks {

// ============================================================================
// MelonBlock
// ============================================================================

MelonBlock::MelonBlock(const Block* stem, const Block* attachedStem, const BlockProperties& properties)
    : StemGrownBlock(properties)
    , m_stem(stem)
    , m_attachedStem(attachedStem)
{}

// ============================================================================
// PumpkinBlock
// ============================================================================

PumpkinBlock::PumpkinBlock(
    const Block* stem, const Block* attachedStem, const Block* carvedPumpkin, const BlockProperties& properties)
    : StemGrownBlock(properties)
    , m_stem(stem)
    , m_attachedStem(attachedStem)
    , m_carvedPumpkin(carvedPumpkin)
{}

// ============================================================================
// 南瓜雕刻功能
// ============================================================================

BlockActionResult PumpkinBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);

    ItemStack& heldItem = player.getHeldItem(hand);
    const Item* item = heldItem.getItem();
    if (item == nullptr || item != Items::SHEARS) {
        return ActionResultType::Pass;
    }

    if (m_carvedPumpkin == nullptr) {
        return ActionResultType::Pass;
    }

    // 计算雕刻南瓜的朝向
    Direction facing = hit.face();
    if (facing == Direction::Up || facing == Direction::Down) {
        f32 yaw = player.yaw();
        while (yaw < 0.0f)
            yaw += 360.0f;
        while (yaw >= 360.0f)
            yaw -= 360.0f;

        Direction playerFacing;
        if (yaw < 45.0f || yaw >= 315.0f) {
            playerFacing = Direction::South;
        } else if (yaw < 135.0f) {
            playerFacing = Direction::West;
        } else if (yaw < 225.0f) {
            playerFacing = Direction::North;
        } else {
            playerFacing = Direction::East;
        }
        facing = Directions::opposite(playerFacing);
    }

    // 播放雕刻音效
    world.playSound(SoundEvents::BLOCK_PUMPKIN_CARVE, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);

    // 将南瓜替换为雕刻南瓜
    const BlockState* carvedState = &m_carvedPumpkin->defaultState();
    std::optional<Direction> currentFacing = carvedState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    if (currentFacing.has_value()) {
        carvedState = &carvedState->with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }

    // flag 11: 通知邻居 + 更新客户端
    world.setBlockState(pos, carvedState, 11);

    // 生成南瓜种子（4个）
    math::Random rng(static_cast<u64>(pos.x ^ pos.y ^ pos.z));

    f64 seedX = static_cast<f64>(pos.x) + 0.5 + static_cast<f64>(Directions::xOffset(facing)) * 0.65;
    f64 seedY = static_cast<f64>(pos.y) + 0.1;
    f64 seedZ = static_cast<f64>(pos.z) + 0.5 + static_cast<f64>(Directions::zOffset(facing)) * 0.65;

    ItemStack seedStack(*Items::PUMPKIN_SEEDS, 4);

    f32 vx = 0.05f * static_cast<f32>(Directions::xOffset(facing)) + static_cast<f32>(rng.nextDouble() * 0.02);
    f32 vy = 0.05f;
    f32 vz = 0.05f * static_cast<f32>(Directions::zOffset(facing)) + static_cast<f32>(rng.nextDouble() * 0.02);

    ItemDropHelper::spawnItemEntity(
        &world, seedStack, seedX, seedY, seedZ, vx, vy, vz, ItemEntity::DEFAULT_PICKUP_DELAY, "");

    // 消耗剪刀耐久度，若物品损坏则触发 onEquippedItemBroken 回调
    LivingEntity::hurtAndBreak(heldItem, 1, &player, LivingEntity::handToEquipmentSlot(hand));

    return ActionResultType::Success;
}

// ============================================================================
// CarvedPumpkinBlock
// ============================================================================

CarvedPumpkinBlock::CarvedPumpkinBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(FACING(), Direction::North));
}

BlockState CarvedPumpkinBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

void CarvedPumpkinBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    trySpawnGolem(world, pos);
}

// ============================================================================
// 傀儡生成静态方法（供 CarvedPumpkinBlock 和 JackOLanternBlock 共用）
// ============================================================================

bool CarvedPumpkinBlock::trySpawnGolem(IWorld& world, const BlockPos& pos)
{
    // 优先级：雪傀儡 > 铁傀儡
    if (checkSnowGolemPattern(world, pos)) {
        spawnSnowGolem(world, pos);
        return true;
    }

    BlockPos bodyPos;
    bool isEastWestArm = false;
    if (checkIronGolemPattern(world, pos, bodyPos, isEastWestArm)) {
        spawnIronGolem(world, pos, pos.down(), isEastWestArm);
        return true;
    }

    return false;
}

bool CarvedPumpkinBlock::isPumpkinHead(const BlockState* state)
{
    if (state == nullptr) {
        return false;
    }
    const Block& block = state->getBlock();
    return &block == VanillaBlocks::CARVED_PUMPKIN || &block == VanillaBlocks::JACK_O_LANTERN;
}

bool CarvedPumpkinBlock::isAir(const BlockState* state)
{
    if (state == nullptr) {
        return true; // 超出世界边界视为空气
    }
    return state->isAir();
}

bool CarvedPumpkinBlock::checkSnowGolemPattern(IWorld& world, const BlockPos& pos)
{
    // 雪傀儡模式：从上到下依次为南瓜头部、雪块、雪块
    const BlockState* below1 = world.getBlockState(pos.down());
    if (below1 == nullptr || !below1->is(VanillaBlocks::SNOW_BLOCK)) {
        return false;
    }

    const BlockState* below2 = world.getBlockState(pos.down(2));
    if (below2 == nullptr || !below2->is(VanillaBlocks::SNOW_BLOCK)) {
        return false;
    }

    return true;
}

bool CarvedPumpkinBlock::checkIronGolemPattern(
    IWorld& world, const BlockPos& pos, BlockPos& outBodyPos, bool& outIsEastWest)
{
    // 铁傀儡模式 - T形结构
    // 顶层：空气、南瓜头部、空气  (~^~)
    // 中层：铁块、铁块、铁块 (###) - 手臂
    // 底层：空气、铁块、空气 (~#~) - 身体

    // 南瓜头部正下方应该是铁块（手臂中央）
    const BlockState* armCenter = world.getBlockState(pos.down());
    if (armCenter == nullptr || !armCenter->is(VanillaBlocks::IRON_BLOCK)) {
        return false;
    }

    // 再下方应该是铁块（身体）
    const BlockState* body = world.getBlockState(pos.down(2));
    if (body == nullptr || !body->is(VanillaBlocks::IRON_BLOCK)) {
        return false;
    }

    // 检查手臂方向：东西或南北
    const BlockPos armPos = pos.down();

    // 尝试东西方向
    const BlockState* armEast = world.getBlockState(armPos.east());
    const BlockState* armWest = world.getBlockState(armPos.west());
    bool eastWestValid = (armEast != nullptr && armEast->is(VanillaBlocks::IRON_BLOCK)) &&
        (armWest != nullptr && armWest->is(VanillaBlocks::IRON_BLOCK));

    if (eastWestValid) {
        // 检查顶层南瓜两侧和底层身体两侧是否为空气
        if (!isAir(world.getBlockState(pos.east())) || !isAir(world.getBlockState(pos.west()))) {
            eastWestValid = false;
        }
        if (eastWestValid &&
            (!isAir(world.getBlockState(armPos.east().down())) || !isAir(world.getBlockState(armPos.west().down())))) {
            eastWestValid = false;
        }
    }

    if (eastWestValid) {
        outBodyPos = pos.down(2);
        outIsEastWest = true;
        return true;
    }

    // 尝试南北方向
    const BlockState* armNorth = world.getBlockState(armPos.north());
    const BlockState* armSouth = world.getBlockState(armPos.south());
    bool northSouthValid = (armNorth != nullptr && armNorth->is(VanillaBlocks::IRON_BLOCK)) &&
        (armSouth != nullptr && armSouth->is(VanillaBlocks::IRON_BLOCK));

    if (northSouthValid) {
        if (!isAir(world.getBlockState(pos.north())) || !isAir(world.getBlockState(pos.south()))) {
            northSouthValid = false;
        }
        if (northSouthValid &&
            (!isAir(world.getBlockState(armPos.north().down())) ||
                !isAir(world.getBlockState(armPos.south().down())))) {
            northSouthValid = false;
        }
    }

    if (northSouthValid) {
        outBodyPos = pos.down(2);
        outIsEastWest = false;
        return true;
    }

    return false;
}

void CarvedPumpkinBlock::spawnSnowGolem(IWorld& world, const BlockPos& headPos)
{
    BlockPos below1 = headPos.down();
    BlockPos below2 = headPos.down(2);

    const BlockState* airState = BlockRegistry::instance().airState();

    // 移除南瓜头部
    if (airState != nullptr) {
        world.setBlockState(headPos, airState, 2);
    }
    world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, headPos, 0);

    // 移除第一个雪块
    const BlockState* snowBlock1 = world.getBlockState(below1);
    if (airState != nullptr) {
        world.setBlockState(below1, airState, 2);
    }
    if (snowBlock1 != nullptr) {
        world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, below1, static_cast<i32>(snowBlock1->stateId()));
    }

    // 移除第二个雪块
    const BlockState* snowBlock2 = world.getBlockState(below2);
    if (airState != nullptr) {
        world.setBlockState(below2, airState, 2);
    }
    if (snowBlock2 != nullptr) {
        world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, below2, static_cast<i32>(snowBlock2->stateId()));
    }

    // 生成雪傀儡实体
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* snowGolemType = registry.getType(entity::EntityTypes::SNOW_GOLEM);
    if (snowGolemType != nullptr) {
        std::unique_ptr<Entity> entity = snowGolemType->create(&world);
        if (entity != nullptr) {
            // 位置设置在底部雪块的中心，Y偏移0.05
            entity->setPosition(static_cast<f32>(below2.x) + 0.5f,
                static_cast<f32>(below2.y) + 0.05f,
                static_cast<f32>(below2.z) + 0.5f);
            entity->setRotation(0.0f, 0.0f);

            auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
            if (mobEntity != nullptr) {
                entity::combat::DifficultyInstance difficultyInstance =
                    entity::combat::DifficultyInstance::at(world, below2);
                mobEntity->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Event);
            }

            world.spawnEntity(std::move(entity));
        }
    }
}

void CarvedPumpkinBlock::spawnIronGolem(
    IWorld& world, const BlockPos& headPos, const BlockPos& armCenterPos, bool isEastWest)
{
    const BlockState* airState = BlockRegistry::instance().airState();

    // 收集所有需要移除的位置
    BlockPos bodyPos = armCenterPos.down();
    std::vector<BlockPos> blocksToRemove;

    if (isEastWest) {
        // 东西方向手臂
        blocksToRemove = {
            bodyPos,             // 身体（底层中央）
            armCenterPos,        // 手臂中央（中层）
            armCenterPos.east(), // 手臂东
            armCenterPos.west(), // 手臂西
            headPos,             // 南瓜头部
        };
    } else {
        // 南北方向手臂
        blocksToRemove = {
            bodyPos,              // 身体（底层中央）
            armCenterPos,         // 手臂中央（中层）
            armCenterPos.north(), // 手臂北
            armCenterPos.south(), // 手臂南
            headPos,              // 南瓜头部
        };
    }

    // 移除所有方块
    for (const BlockPos& blockPos : blocksToRemove) {
        const BlockState* blockState = world.getBlockState(blockPos);
        if (airState != nullptr) {
            world.setBlockState(blockPos, airState, 2);
        }
        if (blockState != nullptr) {
            world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, blockPos, static_cast<i32>(blockState->stateId()));
        }
    }

    // 生成铁傀儡实体
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* ironGolemType = registry.getType(entity::EntityTypes::IRON_GOLEM);
    if (ironGolemType != nullptr) {
        std::unique_ptr<Entity> entity = ironGolemType->create(&world);
        if (entity != nullptr) {
            // 位置设置在身体位置中心，Y偏移0.05
            entity->setPosition(static_cast<f32>(bodyPos.x) + 0.5f,
                static_cast<f32>(bodyPos.y) + 0.05f,
                static_cast<f32>(bodyPos.z) + 0.5f);
            entity->setRotation(0.0f, 0.0f);

            // 玩家建造的铁傀儡不攻击玩家
            IronGolemEntity* ironGolem = dynamic_cast<IronGolemEntity*>(entity.get());
            if (ironGolem != nullptr) {
                ironGolem->setPlayerCreated(true);
            }

            auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
            if (mobEntity != nullptr) {
                entity::combat::DifficultyInstance difficultyInstance =
                    entity::combat::DifficultyInstance::at(world, bodyPos);
                mobEntity->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Event);
            }

            world.spawnEntity(std::move(entity));
        }
    }
}

// ============================================================================
// JackOLanternBlock
// ============================================================================

JackOLanternBlock::JackOLanternBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(FACING(), Direction::North));
}

BlockState JackOLanternBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

void JackOLanternBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // 复用 CarvedPumpkinBlock 的傀儡生成逻辑
    CarvedPumpkinBlock::trySpawnGolem(world, pos);
}

// ============================================================================
// MelonStemBlock - 西瓜茎
// ============================================================================

MelonStemBlock::MelonStemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : StemBlock(crop, properties)
{}

u32 MelonStemBlock::getSeedItem() const
{
    if (Items::MELON_SEEDS != nullptr) {
        return Items::MELON_SEEDS->itemId();
    }
    return 0;
}

// ============================================================================
// PumpkinStemBlock - 南瓜茎
// ============================================================================

PumpkinStemBlock::PumpkinStemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : StemBlock(crop, properties)
{}

u32 PumpkinStemBlock::getSeedItem() const
{
    if (Items::PUMPKIN_SEEDS != nullptr) {
        return Items::PUMPKIN_SEEDS->itemId();
    }
    return 0;
}

// ============================================================================
// MelonAttachedStemBlock - 连接西瓜茎
// ============================================================================

MelonAttachedStemBlock::MelonAttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : AttachedStemBlock(crop, properties)
{}

u32 MelonAttachedStemBlock::getSeedItem() const
{
    if (Items::MELON_SEEDS != nullptr) {
        return Items::MELON_SEEDS->itemId();
    }
    return 0;
}

// ============================================================================
// PumpkinAttachedStemBlock - 连接南瓜茎
// ============================================================================

PumpkinAttachedStemBlock::PumpkinAttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : AttachedStemBlock(crop, properties)
{}

u32 PumpkinAttachedStemBlock::getSeedItem() const
{
    if (Items::PUMPKIN_SEEDS != nullptr) {
        return Items::PUMPKIN_SEEDS->itemId();
    }
    return 0;
}

} // namespace blocks
} // namespace mc
