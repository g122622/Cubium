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
#include "../../../../entity/core/EntityRegistry.hpp"
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
#include "../../VanillaBlocks.hpp"
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
{
    // 西瓜没有状态属性
}

// ============================================================================
// PumpkinBlock
// ============================================================================

PumpkinBlock::PumpkinBlock(
    const Block* stem, const Block* attachedStem, const Block* carvedPumpkin, const BlockProperties& properties)
    : StemGrownBlock(properties)
    , m_stem(stem)
    , m_attachedStem(attachedStem)
    , m_carvedPumpkin(carvedPumpkin)
{
    // 南瓜没有状态属性
}

// ============================================================================
// 南瓜雕刻功能
// ============================================================================

ActionResultType PumpkinBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(state);

    // 获取玩家手中的物品
    ItemStack& heldItem = player.getHeldItem(hand);

    // 检查是否为剪刀
    const Item* item = heldItem.getItem();
    if (item == nullptr || item != Items::SHEARS) {
        return ActionResultType::Pass;
    }

    // 检查是否有雕刻南瓜方块
    if (m_carvedPumpkin == nullptr) {
        return ActionResultType::Pass;
    }

    // 计算雕刻南瓜的朝向
    // MC 1.16.5: 如果点击的面是垂直方向，则使用玩家的水平朝向的相反方向
    // 否则使用点击面的方向
    Direction facing = hit.face();
    if (facing == Direction::Up || facing == Direction::Down) {
        // 垂直方向点击，使用玩家的水平朝向的相反方向
        // 从玩家 yaw 计算水平朝向
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

    // 如果雕刻南瓜有 FACING 属性，设置朝向
    // CarvedPumpkinBlock 继承自 HorizontalBlock，有 FACING 属性
    std::optional<Direction> currentFacing = carvedState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    if (currentFacing.has_value()) {
        carvedState = &carvedState->with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }

    // 设置方块状态（flag 11: 通知邻居 + 更新客户端）
    world.setBlockState(pos, carvedState, 11);

    // 生成南瓜种子（4个）
    // MC 1.16.5: ItemEntity itementity = new ItemEntity(worldIn, (double)pos.getX() + 0.5D +
    // (double)direction1.getXOffset() * 0.65D, (double)pos.getY() + 0.1D, (double)pos.getZ() + 0.5D +
    // (double)direction1.getZOffset() * 0.65D, new ItemStack(Items.PUMPKIN_SEEDS, 4));
    math::Random rng(static_cast<u64>(pos.x ^ pos.y ^ pos.z));

    // 计算种子生成位置（朝向方向的偏移）
    f64 seedX = static_cast<f64>(pos.x) + 0.5 + static_cast<f64>(Directions::xOffset(facing)) * 0.65;
    f64 seedY = static_cast<f64>(pos.y) + 0.1;
    f64 seedZ = static_cast<f64>(pos.z) + 0.5 + static_cast<f64>(Directions::zOffset(facing)) * 0.65;

    // 创建南瓜种子物品堆（4个）
    ItemStack seedStack(*Items::PUMPKIN_SEEDS, 4);

    // 使用 ItemDropHelper 生成物品实体
    // MC 1.16.5: 种子有轻微的随机速度
    // itementity.setMotion(0.05D * (double)direction1.getXOffset() + worldIn.rand.nextDouble() * 0.02D, 0.05D, 0.05D *
    // (double)direction1.getZOffset() + worldIn.rand.nextDouble() * 0.02D);
    f32 vx = 0.05f * static_cast<f32>(Directions::xOffset(facing)) + static_cast<f32>(rng.nextDouble() * 0.02);
    f32 vy = 0.05f;
    f32 vz = 0.05f * static_cast<f32>(Directions::zOffset(facing)) + static_cast<f32>(rng.nextDouble() * 0.02);

    ItemDropHelper::spawnItemEntity(&world,
        seedStack,
        seedX,
        seedY,
        seedZ,
        vx,
        vy,
        vz,
        ItemEntity::DEFAULT_PICKUP_DELAY,
        "" // 无所有者限制
    );

    // 消耗剪刀耐久度
    // MC 1.16.5: itemstack.damageItem(1, player, (p_220282_1_) -> { p_220282_1_.sendBreakAnimation(handIn); });
    heldItem.attemptDamageItem(1);

    return ActionResultType::Success;
}

// ============================================================================
// CarvedPumpkinBlock
// ============================================================================

CarvedPumpkinBlock::CarvedPumpkinBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    // 创建状态容器，添加 HORIZONTAL_FACING 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).create(
        [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(FACING(), Direction::North));
}

BlockState CarvedPumpkinBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 放置时朝向玩家的反方向
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

void CarvedPumpkinBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // 尝试生成傀儡（雪傀儡或铁傀儡）
    trySpawnGolem(world, pos);
}

// ============================================================================
// 傀儡生成逻辑
// ============================================================================

bool CarvedPumpkinBlock::trySpawnGolem(IWorld& world, const BlockPos& pos)
{
    // MC 1.16.5: 先检测雪傀儡，再检测铁傀儡

    // ===== 1. 检测雪傀儡模式 =====
    // 模式：从上到下依次为南瓜、雪块、雪块（垂直线形）
    if (checkSnowGolemPattern(world, pos)) {
        // 获取需要移除的位置
        BlockPos below1 = pos.down();
        BlockPos below2 = pos.down(2);

        // 移除方块并播放破坏效果
        const BlockState* airState = BlockRegistry::instance().airState();

        // 移除南瓜
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 2);
        }
        world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos, 0);

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

        // 生成雪傀儡
        // 位置：南瓜位置的底部中心（因为雪傀儡高度约1.9格）
        auto& registry = entity::EntityRegistry::instance();
        const entity::EntityType* snowGolemType = registry.getType(entity::EntityTypes::SNOW_GOLEM);
        if (snowGolemType != nullptr) {
            std::unique_ptr<Entity> entity = snowGolemType->create(&world);
            if (entity != nullptr) {
                // MC 1.16.5: setLocationAndAngles(pos + 0.5, pos.y + 0.05, pos + 0.5, 0, 0)
                // 位置设置在南瓜位置（模式顶部）
                entity->setPosition(
                    static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.05f, static_cast<f32>(pos.z) + 0.5f);
                entity->setRotation(0.0f, 0.0f);

                world.spawnEntity(std::move(entity));
            }
        }

        return true;
    }

    // ===== 2. 检测铁傀儡模式 =====
    // 模式：T形铁块结构
    // 顶层：空气、南瓜、空气
    // 中层：铁块、铁块、铁块（手臂）
    // 底层：空气、铁块、空气（身体）
    BlockPos bodyPos;
    if (checkIronGolemPattern(world, pos, bodyPos)) {
        // 需要移除的铁块位置（相对于bodyPos）
        // 中层手臂：bodyPos.up(1) 的东西两侧
        // 底层身体：bodyPos
        // 南瓜：bodyPos.up(2)

        const BlockState* airState = BlockRegistry::instance().airState();

        // 收集所有需要移除的位置
        std::vector<BlockPos> blocksToRemove;

        // 身体（底层中央）
        blocksToRemove.push_back(bodyPos);
        // 手臂（中层：身体上方的东西两侧 + 中央）
        blocksToRemove.push_back(bodyPos.up());
        blocksToRemove.push_back(bodyPos.up().east());
        blocksToRemove.push_back(bodyPos.up().west());
        // 南瓜（顶层中央，即 pos）
        blocksToRemove.push_back(pos);

        // 移除所有方块
        for (const BlockPos& blockPos : blocksToRemove) {
            const BlockState* blockState = world.getBlockState(blockPos);
            if (airState != nullptr) {
                world.setBlockState(blockPos, airState, 2);
            }
            if (blockState != nullptr) {
                world.playEvent(
                    world::WorldEvents::BREAK_BLOCK_EFFECTS, blockPos, static_cast<i32>(blockState->stateId()));
            }
        }

        // 生成铁傀儡
        auto& registry = entity::EntityRegistry::instance();
        const entity::EntityType* ironGolemType = registry.getType(entity::EntityTypes::IRON_GOLEM);
        if (ironGolemType != nullptr) {
            std::unique_ptr<Entity> entity = ironGolemType->create(&world);
            if (entity != nullptr) {
                // MC 1.16.5: setLocationAndAngles(bodyPos + 0.5, bodyPos.y + 2 + 0.05, bodyPos + 0.5, 0, 0)
                // 位置设置在南瓜位置（模式顶部中央）
                entity->setPosition(
                    static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.05f, static_cast<f32>(pos.z) + 0.5f);
                entity->setRotation(0.0f, 0.0f);

                // 设置为玩家创建
                IronGolemEntity* ironGolem = dynamic_cast<IronGolemEntity*>(entity.get());
                if (ironGolem != nullptr) {
                    ironGolem->setPlayerCreated(true);
                }

                world.spawnEntity(std::move(entity));
            }
        }

        return true;
    }

    return false;
}

bool CarvedPumpkinBlock::checkSnowGolemPattern(IWorld& world, const BlockPos& pos) const
{
    // MC 1.16.5: 雪傀儡模式
    // 从上到下：南瓜、雪块、雪块
    // 模式匹配：translateOffset(0, 0, 0) = 南瓜，(0, 1, 0) = 雪块，(0, 2, 0) = 雪块

    // 检查南瓜下方第一个雪块
    const BlockState* below1 = world.getBlockState(pos.down());
    if (below1 == nullptr || !below1->is(VanillaBlocks::SNOW_BLOCK)) {
        return false;
    }

    // 检查南瓜下方第二个雪块
    const BlockState* below2 = world.getBlockState(pos.down(2));
    if (below2 == nullptr || !below2->is(VanillaBlocks::SNOW_BLOCK)) {
        return false;
    }

    return true;
}

bool CarvedPumpkinBlock::checkIronGolemPattern(IWorld& world, const BlockPos& pos, BlockPos& outBodyPos) const
{

    // MC 1.16.5: 铁傀儡模式 - T形结构
    // 顶层：空气、南瓜、空气  (~^~)
    // 中层：铁块、铁块、铁块 (###) - 手臂
    // 底层：空气、铁块、空气 (~#~) - 身体
    //
    // 南瓜位置相对于身体(bodyPos)的偏移：
    // 如果南瓜在手臂中央上方，则 bodyPos = pos.down(2)
    // 需要检查四个方向（南瓜可以朝向不同）

    // 南瓜的正下方应该是铁块（手臂中央）
    const BlockState* below = world.getBlockState(pos.down());
    if (below == nullptr || !below->is(VanillaBlocks::IRON_BLOCK)) {
        return false;
    }

    // 再下方应该是铁块（身体）
    const BlockState* below2 = world.getBlockState(pos.down(2));
    if (below2 == nullptr || !below2->is(VanillaBlocks::IRON_BLOCK)) {
        return false;
    }

    // 检查手臂（中层的两侧是否为铁块）
    // 需要检查四个方向，确定手臂的方向
    const BlockPos armCenter = pos.down(); // 手臂中央（南瓜正下方）

    // 检查四个方向：南北或东西
    // 方向组合：North-South 或 East-West

    // 尝试东西方向
    const BlockState* armEast = world.getBlockState(armCenter.east());
    const BlockState* armWest = world.getBlockState(armCenter.west());
    bool eastWestValid = (armEast != nullptr && armEast->is(VanillaBlocks::IRON_BLOCK)) &&
        (armWest != nullptr && armWest->is(VanillaBlocks::IRON_BLOCK));

    // 检查东西方向时，两侧应该是空气
    if (eastWestValid) {
        // 检查顶层的空气
        const BlockState* topEast = world.getBlockState(pos.east());
        const BlockState* topWest = world.getBlockState(pos.west());
        if (!isAir(topEast) || !isAir(topWest)) {
            eastWestValid = false;
        }

        // 检查底层的空气
        const BlockState* bottomEast = world.getBlockState(armCenter.east().down());
        const BlockState* bottomWest = world.getBlockState(armCenter.west().down());
        if (!isAir(bottomEast) || !isAir(bottomWest)) {
            eastWestValid = false;
        }
    }

    if (eastWestValid) {
        outBodyPos = pos.down(2); // 身体位置
        return true;
    }

    // 尝试南北方向
    const BlockState* armNorth = world.getBlockState(armCenter.north());
    const BlockState* armSouth = world.getBlockState(armCenter.south());
    bool northSouthValid = (armNorth != nullptr && armNorth->is(VanillaBlocks::IRON_BLOCK)) &&
        (armSouth != nullptr && armSouth->is(VanillaBlocks::IRON_BLOCK));

    if (northSouthValid) {
        // 检查顶层的空气
        const BlockState* topNorth = world.getBlockState(pos.north());
        const BlockState* topSouth = world.getBlockState(pos.south());
        if (!isAir(topNorth) || !isAir(topSouth)) {
            northSouthValid = false;
        }

        // 检查底层的空气
        const BlockState* bottomNorth = world.getBlockState(armCenter.north().down());
        const BlockState* bottomSouth = world.getBlockState(armCenter.south().down());
        if (!isAir(bottomNorth) || !isAir(bottomSouth)) {
            northSouthValid = false;
        }
    }

    if (northSouthValid) {
        outBodyPos = pos.down(2); // 身体位置
        return true;
    }

    return false;
}

bool CarvedPumpkinBlock::isPumpkin(const BlockState* state)
{
    // MC 1.16.5: IS_PUMPKIN 谓词
    // 检查是否为雕刻南瓜或南瓜灯
    if (state == nullptr) {
        return false;
    }
    const Block& block = state->getBlock();
    return &block == VanillaBlocks::JACK_O_LANTERN ||
        // 注意：CarvedPumpkinBlock 在当前项目中可能未单独注册
        // 如果 CARVED_PUMPKIN 方块单独存在，需要添加检查
        // 目前暂时只检查 JACK_O_LANTERN
        false; // 占位，后续可添加 CARVED_PUMPKIN 检查
}

bool CarvedPumpkinBlock::isAir(const BlockState* state)
{
    // MC 1.16.5: 检查是否为空气
    if (state == nullptr) {
        return true; // 超出世界边界视为空气
    }
    return state->isAir();
}

// ============================================================================
// JackOLanternBlock
// ============================================================================

JackOLanternBlock::JackOLanternBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    // 创建状态容器，添加 HORIZONTAL_FACING 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).create(
        [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(FACING(), Direction::North));
}

BlockState JackOLanternBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 放置时朝向玩家的反方向
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

void JackOLanternBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // MC 1.16.5: 南瓜灯也能触发傀儡生成
    // JackOLanternBlock 继承的 IS_PUMPKIN 谓词包含 JACK_O_LANTERN
    trySpawnGolem(world, pos);
}

bool JackOLanternBlock::trySpawnGolem(IWorld& world, const BlockPos& pos)
{
    // 复用 CarvedPumpkinBlock 的检测逻辑
    // 由于检测逻辑相同，这里直接使用类似的实现

    // ===== 1. 检测雪傀儡模式 =====
    const BlockState* below1 = world.getBlockState(pos.down());
    const BlockState* below2 = world.getBlockState(pos.down(2));

    if (below1 != nullptr && below1->is(VanillaBlocks::SNOW_BLOCK) && below2 != nullptr &&
        below2->is(VanillaBlocks::SNOW_BLOCK)) {
        // 匹配雪傀儡模式
        const BlockState* airState = BlockRegistry::instance().airState();

        // 移除南瓜灯
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 2);
        }
        world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos, 0);

        // 移除雪块
        if (airState != nullptr) {
            world.setBlockState(pos.down(), airState, 2);
        }
        if (below1 != nullptr) {
            world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos.down(), static_cast<i32>(below1->stateId()));
        }

        if (airState != nullptr) {
            world.setBlockState(pos.down(2), airState, 2);
        }
        if (below2 != nullptr) {
            world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos.down(2), static_cast<i32>(below2->stateId()));
        }

        // 生成雪傀儡
        auto& registry = entity::EntityRegistry::instance();
        const entity::EntityType* snowGolemType = registry.getType(entity::EntityTypes::SNOW_GOLEM);
        if (snowGolemType != nullptr) {
            std::unique_ptr<Entity> entity = snowGolemType->create(&world);
            if (entity != nullptr) {
                entity->setPosition(
                    static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.05f, static_cast<f32>(pos.z) + 0.5f);
                entity->setRotation(0.0f, 0.0f);
                world.spawnEntity(std::move(entity));
            }
        }
        return true;
    }

    // ===== 2. 检测铁傀儡模式 =====
    const BlockState* armCenter = world.getBlockState(pos.down());
    const BlockState* body = world.getBlockState(pos.down(2));

    if (armCenter != nullptr && armCenter->is(VanillaBlocks::IRON_BLOCK) && body != nullptr &&
        body->is(VanillaBlocks::IRON_BLOCK)) {

        const BlockPos armPos = pos.down();

        // 检查东西方向
        const BlockState* armEast = world.getBlockState(armPos.east());
        const BlockState* armWest = world.getBlockState(armPos.west());
        bool eastWestValid = armEast != nullptr && armEast->is(VanillaBlocks::IRON_BLOCK) && armWest != nullptr &&
            armWest->is(VanillaBlocks::IRON_BLOCK);

        if (eastWestValid) {
            const BlockState* topEast = world.getBlockState(pos.east());
            const BlockState* topWest = world.getBlockState(pos.west());
            const BlockState* bottomEast = world.getBlockState(armPos.east().down());
            const BlockState* bottomWest = world.getBlockState(armPos.west().down());

            if ((topEast == nullptr || topEast->isAir()) && (topWest == nullptr || topWest->isAir()) &&
                (bottomEast == nullptr || bottomEast->isAir()) && (bottomWest == nullptr || bottomWest->isAir())) {

                // 匹配铁傀儡模式，移除方块并生成实体
                const BlockState* airState = BlockRegistry::instance().airState();
                std::vector<BlockPos> blocksToRemove = {
                    pos.down(2),       // 身体
                    pos.down(),        // 手臂中央
                    pos.down().east(), // 手臂东
                    pos.down().west(), // 手臂西
                    pos                // 南瓜灯
                };

                for (const BlockPos& blockPos : blocksToRemove) {
                    const BlockState* blockState = world.getBlockState(blockPos);
                    if (airState != nullptr) {
                        world.setBlockState(blockPos, airState, 2);
                    }
                    if (blockState != nullptr) {
                        world.playEvent(
                            world::WorldEvents::BREAK_BLOCK_EFFECTS, blockPos, static_cast<i32>(blockState->stateId()));
                    }
                }

                // 生成铁傀儡
                auto& registry = entity::EntityRegistry::instance();
                const entity::EntityType* ironGolemType = registry.getType(entity::EntityTypes::IRON_GOLEM);
                if (ironGolemType != nullptr) {
                    std::unique_ptr<Entity> entity = ironGolemType->create(&world);
                    if (entity != nullptr) {
                        entity->setPosition(static_cast<f32>(pos.x) + 0.5f,
                            static_cast<f32>(pos.y) + 0.05f,
                            static_cast<f32>(pos.z) + 0.5f);
                        entity->setRotation(0.0f, 0.0f);

                        IronGolemEntity* ironGolem = dynamic_cast<IronGolemEntity*>(entity.get());
                        if (ironGolem != nullptr) {
                            ironGolem->setPlayerCreated(true);
                        }

                        world.spawnEntity(std::move(entity));
                    }
                }
                return true;
            }
        }

        // 检查南北方向
        const BlockState* armNorth = world.getBlockState(armPos.north());
        const BlockState* armSouth = world.getBlockState(armPos.south());
        bool northSouthValid = armNorth != nullptr && armNorth->is(VanillaBlocks::IRON_BLOCK) && armSouth != nullptr &&
            armSouth->is(VanillaBlocks::IRON_BLOCK);

        if (northSouthValid) {
            const BlockState* topNorth = world.getBlockState(pos.north());
            const BlockState* topSouth = world.getBlockState(pos.south());
            const BlockState* bottomNorth = world.getBlockState(armPos.north().down());
            const BlockState* bottomSouth = world.getBlockState(armPos.south().down());

            if ((topNorth == nullptr || topNorth->isAir()) && (topSouth == nullptr || topSouth->isAir()) &&
                (bottomNorth == nullptr || bottomNorth->isAir()) && (bottomSouth == nullptr || bottomSouth->isAir())) {

                // 匹配铁傀儡模式，移除方块并生成实体
                const BlockState* airState = BlockRegistry::instance().airState();
                std::vector<BlockPos> blocksToRemove = {
                    pos.down(2),        // 身体
                    pos.down(),         // 手臂中央
                    pos.down().north(), // 手臂北
                    pos.down().south(), // 手臂南
                    pos                 // 南瓜灯
                };

                for (const BlockPos& blockPos : blocksToRemove) {
                    const BlockState* blockState = world.getBlockState(blockPos);
                    if (airState != nullptr) {
                        world.setBlockState(blockPos, airState, 2);
                    }
                    if (blockState != nullptr) {
                        world.playEvent(
                            world::WorldEvents::BREAK_BLOCK_EFFECTS, blockPos, static_cast<i32>(blockState->stateId()));
                    }
                }

                // 生成铁傀儡
                auto& registry = entity::EntityRegistry::instance();
                const entity::EntityType* ironGolemType = registry.getType(entity::EntityTypes::IRON_GOLEM);
                if (ironGolemType != nullptr) {
                    std::unique_ptr<Entity> entity = ironGolemType->create(&world);
                    if (entity != nullptr) {
                        entity->setPosition(static_cast<f32>(pos.x) + 0.5f,
                            static_cast<f32>(pos.y) + 0.05f,
                            static_cast<f32>(pos.z) + 0.5f);
                        entity->setRotation(0.0f, 0.0f);

                        IronGolemEntity* ironGolem = dynamic_cast<IronGolemEntity*>(entity.get());
                        if (ironGolem != nullptr) {
                            ironGolem->setPlayerCreated(true);
                        }

                        world.spawnEntity(std::move(entity));
                    }
                }
                return true;
            }
        }
    }

    return false;
}

// ============================================================================
// MelonStemBlock - 西瓜茎
// ============================================================================

MelonStemBlock::MelonStemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : StemBlock(crop, properties)
{}

u32 MelonStemBlock::getSeedItem() const
{
    // 返回西瓜种子物品ID
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
    // 返回南瓜种子物品ID
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
    // 返回西瓜种子物品ID
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
    // 返回南瓜种子物品ID
    if (Items::PUMPKIN_SEEDS != nullptr) {
        return Items::PUMPKIN_SEEDS->itemId();
    }
    return 0;
}

} // namespace blocks
} // namespace mc
