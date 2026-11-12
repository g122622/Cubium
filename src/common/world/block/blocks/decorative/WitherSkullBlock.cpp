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

#include "common/world/block/blocks/decorative/WitherSkullBlock.hpp"

#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/boss/WitherEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/agricultural/CarvedPumpkinBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/pattern/BlockInWorld.hpp"
#include "common/world/block/pattern/BlockPattern.hpp"
#include "common/world/block/pattern/BlockPatternBuilder.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/SkullBlockEntity.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

namespace {

using blockpattern::BlockInWorld;
using blockpattern::BlockPattern;
using blockpattern::BlockPatternBuilder;

/**
 * @brief 把物品上的 `minecraft:note_block_sound` 组件写入头颅方块实体
 *
 * 该组件对任意头颅都有效（玩家头、生物头皆可），头颅放在音符盒上方时音符盒改用这个
 * 声音。原版在 `SkullBlock` 的放置路径统一搬运该组件，这里同样做成共享助手，
 * 避免四个 `onBlockPlacedBy` 重写里各写一遍。
 */
void _applyNoteBlockSoundToSkull(IWorld& world, const BlockPos& pos, const ItemStack& stack)
{
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr) {
        return;
    }
    auto* skull = dynamic_cast<blockentity::SkullBlockEntity*>(blockEntity);
    if (skull == nullptr) {
        return;
    }

    // 组件缺失时不动方块实体（保持其原有值），而不是清空——
    // 结构模板放置等场景可能已经写好了值。
    const std::string* sound = stack.getNoteBlockSound();
    if (sound != nullptr) {
        skull->setNoteBlockSound(*sound);
    }
}

/// 凋灵召唤完整图案：
/// 顶层 3 个凋灵骷髅头，中层 3 个基座，底层 空气-基座-空气。
const BlockPattern& _witherFullPattern()
{
    static const std::unique_ptr<BlockPattern> pattern =
        BlockPatternBuilder::start()
            .aisle({"^^^", "###", "~#~"})
            .where('^', BlockInWorld::hasState([](const BlockState& state) {
                return state.is(VanillaBlocks::WITHER_SKELETON_SKULL) ||
                    state.is(VanillaBlocks::WITHER_SKELETON_WALL_SKULL);
            }))
            .where('#', BlockInWorld::hasState([](const BlockState& state) {
                return BlockTags::WITHER_SUMMON_BASE_BLOCKS().contains(state);
            }))
            .where('~', BlockInWorld::hasState([](const BlockState& state) {
                return state.isAir();
            }))
            .build();
    return *pattern;
}

/// 凋灵召唤的「基座」图案：顶层不约束（空格匹配任意方块），中层 3 个基座，底层 空气-基座-空气。
/// 对应原版 `WitherSkullBlock.getOrCreateWitherBase`，用于「头颅还没放下，先看基座在不在」。
const BlockPattern& _witherBasePattern()
{
    static const std::unique_ptr<BlockPattern> pattern =
        BlockPatternBuilder::start()
            .aisle({"   ", "###", "~#~"})
            .where('#', BlockInWorld::hasState([](const BlockState& state) {
                return BlockTags::WITHER_SUMMON_BASE_BLOCKS().contains(state);
            }))
            .where('~', BlockInWorld::hasState([](const BlockState& state) {
                return state.isAir();
            }))
            .build();
    return *pattern;
}

// 召唤判定（图案匹配、清图案、生成凋灵、无敌阶段、进度触发）已完整落在 checkSpawn 内，
// 玩家放置站立/墙挂凋灵骷髅头的两条路径都会调用它。
// 覆盖见 `tests/unit/common/world/block/blocks/WitherSkullBlockTest.cpp`（正/负路径与无敌阶段）。
// 发射器放置头颅走的是专用发射行为 `WitherSkeletonSkullDispenseBehavior`
// （`common/world/block/dispense/`）：它先用 `WitherSkullBlock::canSpawnMob` 判定基座图案，
// 命中才写入头颅方块并调用 checkSpawn，不命中则失败且不消耗物品——与原版
// `DispenserBlock` 静态块里 `Items.WITHER_SKELETON_SKULL` 的匿名行为一致。

} // namespace

// ============================================================================
// WitherSkullBlock / WitherWallSkullBlock
// ============================================================================

WitherSkullBlock::WitherSkullBlock(const BlockProperties& properties)
    : SkullBlock(SkullType::WitherSkeleton, properties)
{}

void WitherSkullBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* placer, const ItemStack& stack)
{
    MC_UNUSED(state);
    MC_UNUSED(placer);
    _applyNoteBlockSoundToSkull(world, pos, stack);
    checkSpawn(world, pos);
}

void WitherSkullBlock::checkSpawn(IWorld& world, const BlockPos& pos)
{
    // 召唤流程：
    //   仅服务端判定；头颅方块须为凋灵骷髅头颅（站立或墙挂）；
    //   y >= 世界最低高度且难度非和平；匹配凋灵 T 形图案后清除图案方块并生成凋灵。
    if (world.isClientSide()) {
        return;
    }

    const BlockState* skullState = world.getBlockState(pos);
    if (skullState == nullptr) {
        return;
    }

    const bool isWitherSkull = skullState->is(VanillaBlocks::WITHER_SKELETON_SKULL) ||
        skullState->is(VanillaBlocks::WITHER_SKELETON_WALL_SKULL);
    if (!isWitherSkull) {
        return;
    }

    if (pos.y < world::MIN_BUILD_HEIGHT || world.difficulty() == Difficulty::Peaceful) {
        return;
    }

    // 凋灵图案（含头颅层）：顶层 3 个头颅、中层 3 个基座、底层 空气-基座-空气。
    // find() 枚举候选起点与 24 种朝向，因此 3 个头颅中任意一个作为触发点都能命中。
    auto match = _witherFullPattern().find(world, pos);
    if (!match.has_value()) {
        return;
    }

    // 创建凋灵实体。
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* witherType = registry.getType(entity::EntityTypeKeys::WITHER);
    if (witherType == nullptr) {
        return;
    }
    auto* ecsRegistry = world.entityRegistry();
    if (ecsRegistry == nullptr) {
        return;
    }

    std::unique_ptr<Entity> entity = witherType->create(&world, *ecsRegistry);
    if (entity == nullptr) {
        return;
    }

    // 清除图案方块（遍历全部 3×3 格子，含空气格）后于顶点生成凋灵。
    CarvedPumpkinBlock::clearPatternBlocks(world, *match);

    // 凋灵生成于图案顶点头颅位置，y + 0.55。
    const BlockPos spawnPos = match->getBlock(1, 2, 0).pos();
    entity->setPosition(
        static_cast<f32>(spawnPos.x) + 0.5f, static_cast<f32>(spawnPos.y) + 0.55f, static_cast<f32>(spawnPos.z) + 0.5f);

    // 朝向按图案前向轴决定（X 轴 0 度、Z 轴 90 度），并同步身体/头部朝向。
    const f32 yaw = Directions::getAxis(match->forwards()) == Axis::X ? 0.0f : 90.0f;
    entity->setRotation(yaw, 0.0f);
    entity->setYBodyRot(yaw);
    entity->setYHeadRot(yaw);

    // 凋灵生成后进入 220 tick 生成无敌阶段。
    if (auto* wither = dynamic_cast<entity::WitherEntity*>(entity.get())) {
        wither->makeInvulnerable();
    }

    Entity* spawnedEntity = entity.get();
    world.spawnEntity(std::move(entity));
    // 对 50 格内玩家触发 SUMMONED_ENTITY 进度。
    CarvedPumpkinBlock::triggerSummonedEntity(world, *spawnedEntity, 50.0f);
    CarvedPumpkinBlock::updatePatternBlocks(world, *match);
}

bool WitherSkullBlock::canSpawnMob(IWorld& world, const BlockPos& pos, const ItemStack& stack)
{
    // 与 checkSpawn 的前置条件一致，只是把「头颅层」换成不约束（头颅还没放下）：
    // 物品必须是凋灵骷髅头颅，位置至少要留出 3 层图案（故 +2），难度非和平，且必须在服务端判定。
    if (stack.getItem() != Items::WITHER_SKELETON_SKULL) {
        return false;
    }
    if (pos.y < world.getMinBuildHeight() + 2) {
        return false;
    }
    if (world.difficulty() == Difficulty::Peaceful || world.isClientSide()) {
        return false;
    }
    return _witherBasePattern().find(world, pos).has_value();
}

WitherWallSkullBlock::WitherWallSkullBlock(const BlockProperties& properties)
    : WallSkullBlock(SkullType::WitherSkeleton, properties)
{}

void WitherWallSkullBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* placer, const ItemStack& stack)
{
    MC_UNUSED(state);
    MC_UNUSED(placer);
    _applyNoteBlockSoundToSkull(world, pos, stack);
    WitherSkullBlock::checkSpawn(world, pos);
}

} // namespace blocks
} // namespace mc
