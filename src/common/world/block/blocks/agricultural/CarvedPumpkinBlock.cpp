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

#include "common/world/block/blocks/agricultural/CarvedPumpkinBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemTypes.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/items/special/HoneycombItem.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/container/CopperChestBlock.hpp"
#include "common/world/block/capability/oxidation/IOxidizableBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/pattern/BlockInWorld.hpp"
#include "common/world/block/pattern/BlockPattern.hpp"
#include "common/world/block/pattern/BlockPatternBuilder.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "common/world/effect/base/LevelEventId.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace blocks {

namespace {

using blockpattern::BlockInWorld;
using blockpattern::BlockPattern;
using blockpattern::BlockPatternBuilder;
using blockpattern::BlockPatternMatch;

/// 傀儡头部谓词：雕刻南瓜或南瓜灯。
bool _isPumpkinHeadState(const BlockState& state)
{
    return state.is(VanillaBlocks::CARVED_PUMPKIN) || state.is(VanillaBlocks::JACK_O_LANTERN);
}

/// 空气谓词：模式中的 '~' 要求该位置为空气。
bool _isAirState(const BlockState& state)
{
    return state.isAir();
}

/// 铜块谓词：COPPER 方块标签。
bool _isCopperBlockState(const BlockState& state)
{
    return BlockTags::COPPER().contains(state);
}

// 六个模式（base/full × 雪/铁/铜）在首次使用时构建；
// 方块注册完成后才会触发（onBlockAdded），因此静态缓存的构建时机是安全的。

const BlockPattern& _snowGolemBasePattern()
{
    static const std::unique_ptr<BlockPattern> pattern =
        BlockPatternBuilder::start()
            .aisle({" ", "#", "#"})
            .where('#', BlockInWorld::hasState([](const BlockState& s) {
                return s.is(VanillaBlocks::SNOW_BLOCK);
            }))
            .build();
    return *pattern;
}

const BlockPattern& _snowGolemFullPattern()
{
    static const std::unique_ptr<BlockPattern> pattern =
        BlockPatternBuilder::start()
            .aisle({"^", "#", "#"})
            .where('^', BlockInWorld::hasState(&_isPumpkinHeadState))
            .where('#', BlockInWorld::hasState([](const BlockState& s) {
                return s.is(VanillaBlocks::SNOW_BLOCK);
            }))
            .build();
    return *pattern;
}

const BlockPattern& _ironGolemBasePattern()
{
    static const std::unique_ptr<BlockPattern> pattern =
        BlockPatternBuilder::start()
            .aisle({"~ ~", "###", "~#~"})
            .where('#', BlockInWorld::hasState([](const BlockState& s) {
                return s.is(VanillaBlocks::IRON_BLOCK);
            }))
            .where('~', BlockInWorld::hasState(&_isAirState))
            .build();
    return *pattern;
}

const BlockPattern& _ironGolemFullPattern()
{
    static const std::unique_ptr<BlockPattern> pattern =
        BlockPatternBuilder::start()
            .aisle({"~^~", "###", "~#~"})
            .where('^', BlockInWorld::hasState(&_isPumpkinHeadState))
            .where('#', BlockInWorld::hasState([](const BlockState& s) {
                return s.is(VanillaBlocks::IRON_BLOCK);
            }))
            .where('~', BlockInWorld::hasState(&_isAirState))
            .build();
    return *pattern;
}

const BlockPattern& _copperGolemBasePattern()
{
    static const std::unique_ptr<BlockPattern> pattern =
        BlockPatternBuilder::start().aisle({" ", "#"}).where('#', BlockInWorld::hasState(&_isCopperBlockState)).build();
    return *pattern;
}

const BlockPattern& _copperGolemFullPattern()
{
    static const std::unique_ptr<BlockPattern> pattern = BlockPatternBuilder::start()
                                                             .aisle({"^", "#"})
                                                             .where('^', BlockInWorld::hasState(&_isPumpkinHeadState))
                                                             .where('#', BlockInWorld::hasState(&_isCopperBlockState))
                                                             .build();
    return *pattern;
}

// 傀儡生成函数：定义在下方，trySpawnGolem 需要先看到声明。
bool _spawnSnowGolem(IWorld& world, const BlockPatternMatch& match);
bool _spawnIronGolem(IWorld& world, const BlockPatternMatch& match);
bool _spawnCopperGolem(IWorld& world,
    const BlockPatternMatch& match,
    const Block* copperBlock,
    Direction facing,
    entity::CopperGolemWeatherState weatherState);

} // anonymous namespace

// ============================================================================
// CarvedPumpkinBlock
// ============================================================================

CarvedPumpkinBlock::CarvedPumpkinBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).create(
        [](const Block& block,
            StateValueIndices valueIndices,
            size_t propertyCount,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) {
            return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
        });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(FACING(), Direction::North));
}

BlockState CarvedPumpkinBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

void CarvedPumpkinBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    MC_UNUSED(state);
    trySpawnGolem(world, pos);
}

// ============================================================================
// 傀儡生成静态方法（供 CarvedPumpkinBlock 和 JackOLanternBlock 共用）
// ============================================================================

bool CarvedPumpkinBlock::canSpawnGolem(IWorld& world, const BlockPos& pos)
{
    // 仅检测身体部分（base pattern 的头部位置为任意方块），不检查头部。
    return _snowGolemBasePattern().find(world, pos).has_value() ||
        _ironGolemBasePattern().find(world, pos).has_value() || _copperGolemBasePattern().find(world, pos).has_value();
}

bool CarvedPumpkinBlock::trySpawnGolem(IWorld& world, const BlockPos& pos)
{
    // 优先级：雪傀儡 > 铁傀儡 > 铜傀儡。
    // 每个分支先创建实体，创建成功才清除图案方块（实体创建失败时不修改世界）。
    if (auto match = _snowGolemFullPattern().find(world, pos)) {
        if (_spawnSnowGolem(world, *match)) {
            return true;
        }
    }

    if (auto match = _ironGolemFullPattern().find(world, pos)) {
        if (_spawnIronGolem(world, *match)) {
            return true;
        }
    }

    if (auto match = _copperGolemFullPattern().find(world, pos)) {
        const BlockState* pumpkinState = match->getBlock(0, 0, 0).getState();
        const BlockState* copperState = match->getBlock(0, 1, 0).getState();
        MC_ASSERT_RELEASE(pumpkinState != nullptr);
        MC_ASSERT_RELEASE(copperState != nullptr);

        const Direction facing = pumpkinState->get(BlockStateProperties::HORIZONTAL_FACING());
        // 氧化等级必须在清除方块前计算；Block 指针指向注册表对象，清除后仍然有效。
        const entity::CopperGolemWeatherState weather = getWeatherStateFromCopperBlock(*copperState);
        if (_spawnCopperGolem(world, *match, &copperState->getBlock(), facing, weather)) {
            return true;
        }
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

void CarvedPumpkinBlock::clearPatternBlocks(IWorld& world, const blockpattern::BlockPatternMatch& match)
{
    // 遍历 width × height（depth 固定 0），每格设空气并播放 2001 破坏粒子。
    const BlockState* airState = BlockRegistry::instance().airState();
    MC_ASSERT_RELEASE_MSG(airState != nullptr, "CarvedPumpkinBlock::clearPatternBlocks: air state is not registered");

    for (i32 i = 0; i < match.width(); ++i) {
        for (i32 j = 0; j < match.height(); ++j) {
            const BlockInWorld& blockInWorld = match.getBlock(i, j, 0);
            const BlockState* state = blockInWorld.getState();
            MC_ASSERT_RELEASE_MSG(state != nullptr, "CarvedPumpkinBlock::clearPatternBlocks: cached state is null");

            // 必须在写方块前取 stateId：blockInWorld 的缓存可能指向区块/测试世界的状态对象，
            // setBlockState 之后该对象可能被销毁（测试世界尤为明显）。
            const i32 stateId = static_cast<i32>(state->stateId());
            world.setBlockState(blockInWorld.pos(), airState, world::BlockUpdateFlags::UPDATE_CLIENTS);
            world.playEvent(::mc::world::LevelEventId::PARTICLES_DESTROY_BLOCK, blockInWorld.pos(), stateId);
        }
    }
}

void CarvedPumpkinBlock::updatePatternBlocks(IWorld& world, const blockpattern::BlockPatternMatch& match)
{
    // 对每个模式格子通知邻居。
    MC_ASSERT_RELEASE_MSG(
        VanillaBlocks::AIR != nullptr, "CarvedPumpkinBlock::updatePatternBlocks: AIR is not registered");
    for (i32 i = 0; i < match.width(); ++i) {
        for (i32 j = 0; j < match.height(); ++j) {
            world.updateNeighbors(match.getBlock(i, j, 0).pos(), *VanillaBlocks::AIR);
        }
    }
}

void CarvedPumpkinBlock::triggerSummonedEntity(IWorld& world, Entity& entity, f32 radius)
{
    // 对半径内所有玩家触发 SUMMONED_ENTITY 进度。
    for (Entity* candidate : world.getPlayers()) {
        auto* player = dynamic_cast<Player*>(candidate);
        if (player == nullptr) {
            continue;
        }
        if (player->distanceTo(entity) <= radius) {
            world.onSummonedEntity(player->playerId(), &entity);
        }
    }
}

namespace {

bool _spawnSnowGolem(IWorld& world, const BlockPatternMatch& match)
{
    // 先创建实体，成功后才清除模式方块。
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* snowGolemType = registry.getType(entity::EntityTypeKeys::SNOW_GOLEM);
    if (snowGolemType == nullptr) {
        spdlog::warn("CarvedPumpkinBlock: snow golem entity type is not registered");
        return false;
    }

    auto* ecsRegistry = world.entityRegistry();
    if (ecsRegistry == nullptr) {
        return false;
    }

    std::unique_ptr<Entity> entity = snowGolemType->create(&world, *ecsRegistry);
    if (entity == nullptr) {
        return false;
    }

    // 雪傀儡生成在最底层雪块（pattern 的 (0, 2, 0)）中心。
    const BlockPos spawnPos = match.getBlock(0, 2, 0).pos();
    CarvedPumpkinBlock::clearPatternBlocks(world, match);
    entity->setPosition(
        static_cast<f32>(spawnPos.x) + 0.5f, static_cast<f32>(spawnPos.y) + 0.05f, static_cast<f32>(spawnPos.z) + 0.5f);
    entity->setRotation(0.0f, 0.0f);

    Entity* spawnedEntity = entity.get();
    world.spawnEntity(std::move(entity));
    CarvedPumpkinBlock::triggerSummonedEntity(world, *spawnedEntity, 5.0f);
    CarvedPumpkinBlock::updatePatternBlocks(world, match);
    return true;
}

bool _spawnIronGolem(IWorld& world, const BlockPatternMatch& match)
{
    // 先创建实体，成功后才清除模式方块。
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* ironGolemType = registry.getType(entity::EntityTypeKeys::IRON_GOLEM);
    if (ironGolemType == nullptr) {
        spdlog::warn("CarvedPumpkinBlock: iron golem entity type is not registered");
        return false;
    }

    auto* ecsRegistry = world.entityRegistry();
    if (ecsRegistry == nullptr) {
        return false;
    }

    std::unique_ptr<Entity> entity = ironGolemType->create(&world, *ecsRegistry);
    if (entity == nullptr) {
        return false;
    }

    // 玩家建造的铁傀儡不攻击玩家。
    if (auto* ironGolem = dynamic_cast<IronGolemEntity*>(entity.get())) {
        ironGolem->setPlayerCreated(true);
    }

    // 铁傀儡生成在底层中央铁块（pattern 的 (1, 2, 0)）中心。
    const BlockPos spawnPos = match.getBlock(1, 2, 0).pos();
    CarvedPumpkinBlock::clearPatternBlocks(world, match);
    entity->setPosition(
        static_cast<f32>(spawnPos.x) + 0.5f, static_cast<f32>(spawnPos.y) + 0.05f, static_cast<f32>(spawnPos.z) + 0.5f);
    entity->setRotation(0.0f, 0.0f);

    Entity* spawnedEntity = entity.get();
    world.spawnEntity(std::move(entity));
    CarvedPumpkinBlock::triggerSummonedEntity(world, *spawnedEntity, 5.0f);
    CarvedPumpkinBlock::updatePatternBlocks(world, match);
    return true;
}

bool _spawnCopperGolem(IWorld& world,
    const BlockPatternMatch& match,
    const Block* copperBlock,
    Direction facing,
    entity::CopperGolemWeatherState weatherState)
{
    // 流程：清除图案 → 定位 → 生成实体 → 通知玩家 → 更新邻居 → 替换铜箱子 → 初始化氧化等级。
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* copperGolemType = registry.getType(entity::EntityTypeKeys::COPPER_GOLEM);
    if (copperGolemType == nullptr) {
        spdlog::warn("CarvedPumpkinBlock: copper golem entity type is not registered");
        return false;
    }

    auto* ecsRegistry = world.entityRegistry();
    if (ecsRegistry == nullptr) {
        return false;
    }

    std::unique_ptr<Entity> entity = copperGolemType->create(&world, *ecsRegistry);
    if (entity == nullptr) {
        return false;
    }

    // 铜傀儡生成在南瓜头部（pattern 的 (0, 0, 0)）中心；铜块位置 (0, 1, 0) 会被替换为铜箱子。
    const BlockPos spawnPos = match.getBlock(0, 0, 0).pos();
    const BlockPos copperPos = match.getBlock(0, 1, 0).pos();

    CarvedPumpkinBlock::clearPatternBlocks(world, match);
    entity->setPosition(
        static_cast<f32>(spawnPos.x) + 0.5f, static_cast<f32>(spawnPos.y) + 0.05f, static_cast<f32>(spawnPos.z) + 0.5f);
    entity->setRotation(0.0f, 0.0f);

    Entity* spawnedEntity = entity.get();
    world.spawnEntity(std::move(entity));
    CarvedPumpkinBlock::triggerSummonedEntity(world, *spawnedEntity, 5.0f);
    CarvedPumpkinBlock::updatePatternBlocks(world, match);

    // 把铜块位置替换为对应氧化等级的铜箱子。
    MC_ASSERT_RELEASE_MSG(copperBlock != nullptr, "CarvedPumpkinBlock: copper block is null");
    BlockState chestState = CopperChestBlock::getFromCopperBlock(copperBlock, facing, world, copperPos);
    world.setBlockState(copperPos, &chestState, world::BlockUpdateFlags::UPDATE_CLIENTS);

    // 设置氧化等级并播放生成音效。
    if (auto* copperGolem = dynamic_cast<CopperGolemEntity*>(spawnedEntity)) {
        copperGolem->spawnFromStatue(weatherState);
    }
    return true;
}

} // anonymous namespace

// ============================================================================
// 铜傀儡生成
// ============================================================================

entity::CopperGolemWeatherState CarvedPumpkinBlock::getWeatherStateFromCopperBlock(const BlockState& copperState)
{
    // 氧化等级（UNAFFECTED/EXPOSED/WEATHERED/OXIDIZED）对应
    // CopperGolemWeatherState。转换优先级：
    // 1. 铜块本身实现 IOxidizableBlock（暴露/锈蚀/氧化变种）→ 直接取氧化等级
    // 2. 涂蜡铜块（不实现 IOxidizableBlock）→ 通过 HoneycombItem::getWaxOffMap 查找未涂蜡变种，
    //    再取未涂蜡变种的氧化等级
    // 3. 兜底回退到 Unaffected

    const Block& block = copperState.getBlock();

    // 路径 1：直接实现 IOxidizableBlock
    const auto* oxidizable = dynamic_cast<const blocks::IOxidizableBlock*>(&block);
    if (oxidizable != nullptr) {
        return entity::CopperGolemOxidationUtils::fromBlockOxidation(oxidizable->getOxidationLevel());
    }

    // 路径 2：涂蜡变种，通过除蜡映射表查找未涂蜡变种
    auto& waxOffMap = item::items::HoneycombItem::getWaxOffMap();
    auto it = waxOffMap.find(&block);
    if (it != waxOffMap.end() && it->second != nullptr) {
        const auto* unwaxedOxidizable = dynamic_cast<const blocks::IOxidizableBlock*>(it->second);
        if (unwaxedOxidizable != nullptr) {
            return entity::CopperGolemOxidationUtils::fromBlockOxidation(unwaxedOxidizable->getOxidationLevel());
        }
    }

    // 路径 3：兜底回退到 Unaffected（对应 MC 的 Blocks.COPPER_BLOCK.getAge() = UNAFFECTED）
    return entity::CopperGolemWeatherState::Unaffected;
}

} // namespace blocks
} // namespace mc
