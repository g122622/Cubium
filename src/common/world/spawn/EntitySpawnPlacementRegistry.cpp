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

#include "EntitySpawnPlacementRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "entity/core/EntityRegistry.hpp"
#include "entity/core/EntityType.hpp"
#include "entity/vehicle/DismountHelper.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/math/random/IRandom.hpp"
#include "world/WorldConstants.hpp"
#include "world/biome/BiomeTags.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/ice/SnowBlock.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidTags.hpp"
#include "world/lighting/InternalLightUtils.hpp"
#include "world/spawn/SlimeChunkChecker.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace mc::world::spawn {

// 静态成员定义
std::unordered_map<std::string, EntitySpawnPlacementRegistry::PlacementEntry> EntitySpawnPlacementRegistry::s_registry;
bool EntitySpawnPlacementRegistry::s_initialized = false;

namespace {

bool _fireImmune(const std::string& id)
{
    const auto* type = entity::EntityRegistry::instance().getType(id);
    return type != nullptr && type->immuneToFire();
}

bool _isValidSpawnSurface(const BlockState& state, const std::string& id)
{
    if (state.is(VanillaBlocks::MAGMA)) {
        return _fireImmune(id);
    }
    if (BlockTags::LEAVES().contains(state)) {
        return id == "minecraft:ocelot" || id == "minecraft:parrot";
    }
    if (state.is(VanillaBlocks::ICE) || state.is(VanillaBlocks::FROSTED_ICE)) {
        return id == "minecraft:polar_bear";
    }
    const auto& name = state.blockLocation().path();
    if (name.ends_with("glass") || state.is(VanillaBlocks::BEDROCK) || state.is(VanillaBlocks::BARRIER) ||
        name.ends_with("trapdoor")) {
        return false;
    }
    if (state.is(VanillaBlocks::SNOW)) {
        return state.get(blocks::SnowBlock::LAYERS()) == 8;
    }
    return Block::isFaceFull(state.getBlockSupportShape(), Direction::Up) && state.lightLevel() < 14;
}

bool _isRedstoneConductor(const BlockState& state)
{
    if (state.is(VanillaBlocks::MUD) || state.is(VanillaBlocks::SOUL_SAND)) {
        return true;
    }
    const auto& name = state.blockLocation().path();
    if (name.ends_with("glass") || BlockTags::LEAVES().contains(state) || state.is(VanillaBlocks::ICE) ||
        state.is(VanillaBlocks::FROSTED_ICE) || state.is(VanillaBlocks::GLOWSTONE) ||
        state.is(VanillaBlocks::SEA_LANTERN)) {
        return false;
    }
    return state.getCollisionShape().coversFullBlock();
}

bool _isBlockDangerous(const BlockState& state, const std::string& id)
{
    // 实体的方块免疫先于火焰与通用危险检查。
    if ((id == "minecraft:fox" && state.is(VanillaBlocks::SWEET_BERRY_BUSH)) ||
        ((id == "minecraft:polar_bear" || id == "minecraft:snow_golem" || id == "minecraft:stray") &&
            state.is(VanillaBlocks::POWDER_SNOW)) ||
        ((id == "minecraft:wither" || id == "minecraft:wither_skeleton") && state.is(VanillaBlocks::WITHER_ROSE))) {
        return false;
    }
    return DismountHelper::isBlockDangerous(state, _fireImmune(id));
}

} // namespace

// ============================================================================
// 注册方法
// ============================================================================

void EntitySpawnPlacementRegistry::registerPlacement(const std::string& entityTypeId,
    PlacementType placementType,
    HeightmapType heightmapType,
    PlacementPredicate predicate)
{
    MC_ASSERT_RELEASE(static_cast<bool>(predicate));
    s_registry.insert_or_assign(entityTypeId, PlacementEntry{placementType, heightmapType, std::move(predicate)});
}

// ============================================================================
// 查询方法
// ============================================================================

PlacementType EntitySpawnPlacementRegistry::getPlacementType(const std::string& entityTypeId)
{
    auto it = s_registry.find(entityTypeId);
    if (it != s_registry.end()) {
        return it->second.placementType;
    }
    return PlacementType::NoRestrictions;
}

HeightmapType EntitySpawnPlacementRegistry::getHeightmapType(const std::string& entityTypeId)
{
    auto it = s_registry.find(entityTypeId);
    if (it != s_registry.end()) {
        return it->second.heightmapType;
    }
    return HeightmapType::MotionBlockingNoLeaves;
}

const EntitySpawnPlacementRegistry::PlacementEntry* EntitySpawnPlacementRegistry::getPlacementEntry(
    const std::string& entityTypeId)
{
    auto it = s_registry.find(entityTypeId);
    if (it != s_registry.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// 生成检查方法
// ============================================================================

bool EntitySpawnPlacementRegistry::canSpawnAtLocation(
    PlacementType placementType, const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId)
{
    // 无限制类型直接返回 true
    if (placementType == PlacementType::NoRestrictions) {
        return true;
    }

    // 检查世界边界
    if (!world.isWithinWorldBorder(pos.x, pos.y, pos.z)) {
        return false;
    }

    // 根据放置类型进行检查
    switch (placementType) {
        case PlacementType::OnGround:
            return checkOnGroundSpawn(world, pos, entityTypeId);

        case PlacementType::InWater:
            return checkInWaterSpawn(world, pos, entityTypeId);

        case PlacementType::InLava:
            return checkInLavaSpawn(world, pos, entityTypeId);

        case PlacementType::NoRestrictions:
        default:
            return true;
    }
}

bool EntitySpawnPlacementRegistry::canSpawnEntity(const std::string& id,
    const ISpawnWorldReader& world,
    SpawnReason reason,
    const Vector3i& pos,
    math::IRandom& random)
{
    return canSpawnAtLocation(getPlacementType(id), world, pos, id) && checkSpawnRules(id, world, reason, pos, random);
}

bool EntitySpawnPlacementRegistry::checkSpawnRules(const std::string& id,
    const ISpawnWorldReader& world,
    SpawnReason reason,
    const Vector3i& pos,
    math::IRandom& random)
{
    const auto* entry = getPlacementEntry(id);
    return entry == nullptr || entry->predicate(world, pos, id, random, reason);
}

// ============================================================================
// 放置条件检查
// ============================================================================

bool EntitySpawnPlacementRegistry::checkOnGroundSpawn(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId)
{
    const BlockState* belowState = world.getBlockState(pos.x, pos.y - 1, pos.z);
    if (belowState == nullptr || !_isValidSpawnSurface(*belowState, entityTypeId)) {
        return false;
    }

    // 检查生成位置和上方是否可以通过
    if (!_isValidSpawnBlock(world, pos, entityTypeId)) {
        return false;
    }

    // 检查上方位置（对于高度 > 1 的生物）
    const Vector3i posAbove(pos.x, pos.y + 1, pos.z);
    if (!_isValidSpawnBlock(world, posAbove, entityTypeId)) {
        return false;
    }

    return true;
}

bool EntitySpawnPlacementRegistry::checkInWaterSpawn(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string&)
{
    const BlockState* currentState = world.getBlockState(pos.x, pos.y, pos.z);
    if (currentState == nullptr) {
        return false;
    }
    const auto* fluid = currentState->getFluidState();
    if (fluid == nullptr || !fluid::FluidTags::WATER().contains(fluid->getFluid())) {
        return false;
    }
    const auto* above = world.getBlockState(pos.x, pos.y + 1, pos.z);
    return above != nullptr && !_isRedstoneConductor(*above);
}

bool EntitySpawnPlacementRegistry::checkInLavaSpawn(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string&)
{
    const BlockState* currentState = world.getBlockState(pos.x, pos.y, pos.z);
    const auto* fluid = currentState == nullptr ? nullptr : currentState->getFluidState();
    return fluid != nullptr && fluid::FluidTags::LAVA().contains(fluid->getFluid());
}

bool EntitySpawnPlacementRegistry::_isValidSpawnBlock(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId)
{
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }

    if (state->getCollisionShape().coversFullBlock() || state->getBlock().canProvidePower(*state)) {
        return false;
    }
    const auto* fluid = state->getFluidState();
    if (fluid != nullptr && !fluid->isEmpty()) {
        return false;
    }
    const auto* prevent = BlockTags::getTag(ResourceLocation("minecraft:prevent_mob_spawning_inside"));
    return (prevent == nullptr || !prevent->contains(*state)) && !_isBlockDangerous(*state, entityTypeId);
}

namespace {

struct _check {
    const ISpawnWorldReader& world;
    const Vector3i& pos;
    const std::string& id;
    math::IRandom& random;
    SpawnReason reason;
    SpawnEnvironment env;
};

const BlockState* _block(const _check& c, i32 offset)
{
    return c.world.getBlockState(c.pos.x, c.pos.y + offset, c.pos.z);
}

bool _water(const BlockState* state)
{
    const auto* value = state == nullptr ? nullptr : state->getFluidState();
    return value != nullptr && fluid::FluidTags::WATER().contains(value->getFluid());
}

bool _tag(const _check& c, std::string_view name)
{
    const auto* state = _block(c, -1);
    const auto* tag = BlockTags::getTag(ResourceLocation(std::string(name)));
    return state != nullptr && tag != nullptr && tag->contains(*state);
}

bool _biomeTag(const _check& c, std::string_view name)
{
    const auto* tag = biome::BiomeTags::getTag(ResourceLocation(std::string(name)));
    return tag != nullptr && tag->contains(c.world.getBiome(c.pos.x, c.pos.y, c.pos.z));
}

i32 _raw(const SpawnEnvironment& env, i32 darkening)
{
    return InternalLightUtils::calculateRawBrightness(env.blockLight, env.skyLight, darkening);
}

bool _bright(const _check& c)
{
    return _raw(c.env, 0) > 8;
}

bool _mob(const _check& c)
{
    const auto* below = _block(c, -1);
    return isSpawnerReason(c.reason) || (below != nullptr && _isValidSpawnSurface(*below, c.id));
}

bool _dark(const _check& c)
{
    if (c.env.skyLight > c.random.nextInt(32)) {
        return false;
    }
    if (c.env.monsterBlockLightLimit < 15 && c.env.blockLight > c.env.monsterBlockLightLimit) {
        return false;
    }
    const i32 brightness = _raw(c.env, c.env.thundering ? 10 : c.env.skyDarkening);
    const i32 limit = c.env.monsterLightMinimum == c.env.monsterLightMaximum
        ? c.env.monsterLightMinimum
        : c.env.monsterLightMinimum + c.random.nextInt(c.env.monsterLightMaximum - c.env.monsterLightMinimum + 1);
    return brightness <= limit;
}

bool _monster(const _check& c)
{
    return c.world.difficulty() != Difficulty::Peaceful && (c.reason == SpawnReason::TrialSpawner || _dark(c)) &&
        _mob(c);
}

bool _anyLightMonster(const _check& c)
{
    return c.world.difficulty() != Difficulty::Peaceful && _mob(c);
}

bool _surfaceMonster(const _check& c)
{
    return _monster(c) && (isSpawnerReason(c.reason) || c.env.skyLight >= 15);
}

bool _animal(const _check& c)
{
    const bool bright = c.reason == SpawnReason::TrialSpawner || _bright(c);
    return _tag(c, "minecraft:animals_spawnable_on") && bright;
}

bool _taggedAnimal(const _check& c)
{
    static constexpr std::array<std::pair<std::string_view, std::string_view>, 9> tags{{
        {"minecraft:armadillo", "minecraft:armadillo_spawnable_on"},
        {"minecraft:camel", "minecraft:camels_spawnable_on"},
        {"minecraft:frog", "minecraft:frogs_spawnable_on"},
        {"minecraft:goat", "minecraft:goats_spawnable_on"},
        {"minecraft:mooshroom", "minecraft:mooshrooms_spawnable_on"},
        {"minecraft:parrot", "minecraft:parrots_spawnable_on"},
        {"minecraft:rabbit", "minecraft:rabbits_spawnable_on"},
        {"minecraft:wolf", "minecraft:wolves_spawnable_on"},
        {"minecraft:fox", "minecraft:foxes_spawnable_on"},
    }};
    const auto found = std::find_if(tags.begin(), tags.end(), [&](const auto& entry) {
        return entry.first == c.id;
    });
    MC_ASSERT_RELEASE(found != tags.end());
    return _tag(c, found->second) && _bright(c);
}

bool _axolotl(const _check& c)
{
    return _tag(c, "minecraft:axolotls_spawnable_on");
}

bool _waterSurface(const _check& c)
{
    const auto* above = _block(c, 1);
    return c.pos.y >= c.env.seaLevel - 13 && c.pos.y <= c.env.seaLevel && _water(_block(c, -1)) && above != nullptr &&
        above->is(VanillaBlocks::WATER);
}

bool _nautilus(const _check& c)
{
    const auto* above = _block(c, 1);
    return c.pos.y >= c.env.seaLevel - 25 && c.pos.y <= c.env.seaLevel - 5 && _water(_block(c, -1)) &&
        above != nullptr && above->is(VanillaBlocks::WATER);
}

bool _drowned(const _check& c)
{
    if (!_water(_block(c, -1)) && !isSpawnerReason(c.reason)) {
        return false;
    }
    const bool allowed = c.world.difficulty() != Difficulty::Peaceful &&
        (c.reason == SpawnReason::TrialSpawner || _dark(c)) && (isSpawnerReason(c.reason) || _water(_block(c, 0)));
    if (!allowed || (!isSpawnerReason(c.reason) && c.reason != SpawnReason::Reinforcement)) {
        return _biomeTag(c, "minecraft:more_frequent_drowned_spawns")
            ? c.random.nextInt(15) == 0 && allowed
            : c.random.nextInt(40) == 0 && c.pos.y < c.env.seaLevel - 5 && allowed;
    }
    return true;
}

bool _seeSkyFromBelowWater(const _check& c)
{
    if (c.pos.y >= c.env.seaLevel) {
        return c.env.skyLight >= 15;
    }
    const Vector3i surface(c.pos.x, c.env.seaLevel, c.pos.z);
    if (c.world.environment(surface).skyLight < 15) {
        return false;
    }
    for (i32 y = c.env.seaLevel - 1; y > c.pos.y; --y) {
        const auto* state = c.world.getBlockState(c.pos.x, y, c.pos.z);
        if (state == nullptr) {
            return false;
        }
        if (state->getOpacity() > 0 && !state->isLiquid()) {
            return false;
        }
    }
    return true;
}

bool _guardian(const _check& c)
{
    return (c.random.nextInt(20) == 0 || !_seeSkyFromBelowWater(c)) && c.world.difficulty() != Difficulty::Peaceful &&
        (isSpawnerReason(c.reason) || _water(_block(c, 0))) && _water(_block(c, -1));
}

bool _tropicalFish(const _check& c)
{
    const auto* above = _block(c, 1);
    return _water(_block(c, -1)) && above != nullptr && above->is(VanillaBlocks::WATER) &&
        (_biomeTag(c, "minecraft:allows_tropical_fish_spawns_at_any_height") || _waterSurface(c));
}

bool _bat(const _check& c)
{
    if (c.pos.y >= c.world.getHeight(HeightmapType::WorldSurface, c.pos.x, c.pos.z) || c.random.nextBoolean()) {
        return false;
    }
    return _raw(c.env, c.env.skyDarkening) <= c.random.nextInt(4) && _tag(c, "minecraft:bats_spawnable_on") && _mob(c);
}

bool _smallMonster(const _check& c)
{
    return _anyLightMonster(c) &&
        (isSpawnerReason(c.reason) ||
            !c.world.hasNearbyPlayer(Vector3d(c.pos.x + 0.5, c.pos.y + 0.5, c.pos.z + 0.5), 5.0));
}

bool _ghast(const _check& c)
{
    return c.world.difficulty() != Difficulty::Peaceful && c.random.nextInt(20) == 0 && _mob(c);
}

bool _glowSquid(const _check& c)
{
    const auto* state = _block(c, 0);
    return c.pos.y <= c.env.seaLevel - 33 && _raw(c.env, 0) == 0 && state != nullptr && state->is(VanillaBlocks::WATER);
}

bool _magmaCube(const _check& c)
{
    return c.world.difficulty() != Difficulty::Peaceful;
}

bool _ocelot(const _check& c)
{
    return c.random.nextInt(3) != 0;
}

bool _piglin(const _check& c)
{
    const auto* below = _block(c, -1);
    return below != nullptr && !below->is(VanillaBlocks::NETHER_WART_BLOCK);
}

bool _pillager(const _check& c)
{
    return c.env.blockLight <= 8 && _anyLightMonster(c);
}

bool _polarBear(const _check& c)
{
    return _biomeTag(c, "minecraft:polar_bears_spawn_on_alternate_blocks")
        ? _bright(c) && _tag(c, "minecraft:polar_bears_spawnable_on_alternate")
        : _animal(c);
}

bool _skeletonHorse(const _check& c)
{
    return isSpawnerReason(c.reason) ? c.reason == SpawnReason::TrialSpawner || _bright(c) : _animal(c);
}

bool _slime(const _check& c)
{
    if (c.world.difficulty() == Difficulty::Peaceful) {
        return false;
    }
    if (isSpawnerReason(c.reason)) {
        return _mob(c);
    }
    if (_biomeTag(c, "minecraft:allows_surface_slime_spawns") && c.pos.y > 50 && c.pos.y < 70 &&
        c.random.nextFloat() < c.env.surfaceSlimeSpawnChance &&
        _raw(c.env, c.env.skyDarkening) <= c.random.nextInt(8)) {
        return _mob(c);
    }
    const bool chunk = SlimeChunkChecker::isSlimeChunk(c.world.seed(), c.pos.x >> 4, c.pos.z >> 4);
    return c.random.nextInt(10) == 0 && chunk && c.pos.y < 40 && _mob(c);
}

bool _stray(const _check& c)
{
    i32 y = c.pos.y;
    const BlockState* state;
    do {
        state = c.world.getBlockState(c.pos.x, ++y, c.pos.z);
    } while (state != nullptr && state->is(VanillaBlocks::POWDER_SNOW));
    return _monster(c) &&
        (isSpawnerReason(c.reason) || c.world.environment(Vector3i(c.pos.x, y - 1, c.pos.z)).skyLight >= 15);
}

bool _strider(const _check& c)
{
    i32 y = c.pos.y;
    const BlockState* state;
    const fluid::FluidState* fluid;
    do {
        state = c.world.getBlockState(c.pos.x, ++y, c.pos.z);
        fluid = state == nullptr ? nullptr : state->getFluidState();
    } while (fluid != nullptr && fluid::FluidTags::LAVA().contains(fluid->getFluid()));
    return state != nullptr && state->isAir();
}

bool _turtle(const _check& c)
{
    return c.pos.y < c.env.seaLevel + 4 && _tag(c, "minecraft:sand") && _bright(c);
}

bool _zombifiedPiglin(const _check& c)
{
    return c.world.difficulty() != Difficulty::Peaceful && _piglin(c);
}

EntitySpawnPlacementRegistry::PlacementPredicate _predicate(bool (*function)(const _check&))
{
    return [function](const ISpawnWorldReader& world,
               const Vector3i& pos,
               const std::string& id,
               math::IRandom& random,
               SpawnReason reason) {
        return function(_check{world, pos, id, random, reason, world.environment(pos)});
    };
}

} // namespace

void EntitySpawnPlacementRegistry::initializeDefaults()
{
    if (s_initialized) {
        return;
    }

    registerPlacement(
        "minecraft:axolotl", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _predicate(_axolotl));
    registerPlacement(
        "minecraft:cod", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _predicate(_waterSurface));
    registerPlacement(
        "minecraft:dolphin", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _predicate(_waterSurface));
    registerPlacement(
        "minecraft:drowned", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _predicate(_drowned));
    registerPlacement(
        "minecraft:guardian", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _predicate(_guardian));
    registerPlacement("minecraft:pufferfish",
        PlacementType::InWater,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_waterSurface));
    registerPlacement(
        "minecraft:salmon", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _predicate(_waterSurface));
    registerPlacement(
        "minecraft:squid", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _predicate(_waterSurface));
    registerPlacement("minecraft:tropical_fish",
        PlacementType::InWater,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_tropicalFish));
    registerPlacement("minecraft:armadillo",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_taggedAnimal));
    registerPlacement(
        "minecraft:bat", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_bat));
    registerPlacement("minecraft:blaze",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_anyLightMonster));
    registerPlacement(
        "minecraft:bogged", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement("minecraft:breeze",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_anyLightMonster));
    registerPlacement(
        "minecraft:camel", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_taggedAnimal));
    registerPlacement("minecraft:camel_husk",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_surfaceMonster));
    registerPlacement(
        "minecraft:cave_spider", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement(
        "minecraft:chicken", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement(
        "minecraft:cow", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement(
        "minecraft:creeper", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement(
        "minecraft:donkey", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement(
        "minecraft:enderman", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement("minecraft:endermite",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_smallMonster));
    registerPlacement(
        "minecraft:ender_dragon", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_mob));
    registerPlacement(
        "minecraft:frog", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_taggedAnimal));
    registerPlacement(
        "minecraft:ghast", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_ghast));
    registerPlacement(
        "minecraft:happy_ghast", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement(
        "minecraft:giant", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement(
        "minecraft:glow_squid", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _predicate(_glowSquid));
    registerPlacement(
        "minecraft:goat", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_taggedAnimal));
    registerPlacement(
        "minecraft:horse", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement(
        "minecraft:husk", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_surfaceMonster));
    registerPlacement(
        "minecraft:iron_golem", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_mob));
    registerPlacement(
        "minecraft:llama", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement(
        "minecraft:magma_cube", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_magmaCube));
    registerPlacement("minecraft:mooshroom",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_taggedAnimal));
    registerPlacement(
        "minecraft:mule", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement(
        "minecraft:nautilus", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _predicate(_nautilus));
    registerPlacement("minecraft:ocelot", PlacementType::OnGround, HeightmapType::MotionBlocking, _predicate(_ocelot));
    registerPlacement(
        "minecraft:parrot", PlacementType::OnGround, HeightmapType::MotionBlocking, _predicate(_taggedAnimal));
    registerPlacement(
        "minecraft:pig", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement(
        "minecraft:hoglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_piglin));
    registerPlacement(
        "minecraft:piglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_piglin));
    registerPlacement(
        "minecraft:pillager", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_pillager));
    registerPlacement(
        "minecraft:polar_bear", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_polarBear));
    registerPlacement(
        "minecraft:rabbit", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_taggedAnimal));
    registerPlacement(
        "minecraft:sheep", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement("minecraft:silverfish",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_smallMonster));
    registerPlacement(
        "minecraft:skeleton", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement("minecraft:skeleton_horse",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_skeletonHorse));
    registerPlacement(
        "minecraft:slime", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_slime));
    registerPlacement(
        "minecraft:snow_golem", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_mob));
    registerPlacement(
        "minecraft:spider", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement(
        "minecraft:stray", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_stray));
    registerPlacement("minecraft:parched",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_surfaceMonster));
    registerPlacement(
        "minecraft:strider", PlacementType::InLava, HeightmapType::MotionBlockingNoLeaves, _predicate(_strider));
    registerPlacement(
        "minecraft:turtle", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_turtle));
    registerPlacement(
        "minecraft:villager", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_mob));
    registerPlacement(
        "minecraft:witch", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement(
        "minecraft:wither", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement("minecraft:wither_skeleton",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_monster));
    registerPlacement(
        "minecraft:wolf", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_taggedAnimal));
    registerPlacement("minecraft:zoglin",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_anyLightMonster));
    registerPlacement(
        "minecraft:creaking", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement(
        "minecraft:zombie", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement(
        "minecraft:zombie_horse", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement("minecraft:zombified_piglin",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_zombifiedPiglin));
    registerPlacement("minecraft:zombie_villager",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_monster));
    registerPlacement(
        "minecraft:cat", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement("minecraft:elder_guardian",
        PlacementType::InWater,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_guardian));
    registerPlacement(
        "minecraft:evoker", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement("minecraft:fox",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_taggedAnimal));
    registerPlacement("minecraft:illusioner",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_monster));
    registerPlacement(
        "minecraft:panda", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves, _predicate(_animal));
    registerPlacement(
        "minecraft:phantom", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves, _predicate(_mob));
    registerPlacement(
        "minecraft:ravager", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement(
        "minecraft:shulker", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves, _predicate(_mob));
    registerPlacement("minecraft:trader_llama",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_animal));
    registerPlacement(
        "minecraft:vex", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    registerPlacement("minecraft:vindicator",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        _predicate(_monster));
    registerPlacement(
        "minecraft:wandering_trader", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _predicate(_mob));
    registerPlacement(
        "minecraft:warden", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves, _predicate(_monster));
    s_initialized = true;
}

bool EntitySpawnPlacementRegistry::isInitialized()
{
    return s_initialized;
}

} // namespace mc::world::spawn
