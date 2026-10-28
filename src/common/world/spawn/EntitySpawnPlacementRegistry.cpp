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
#include <string>
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
    s_registry[entityTypeId] = PlacementEntry(placementType, heightmapType, std::move(predicate));
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

bool EntitySpawnPlacementRegistry::canSpawnEntity(const std::string& entityTypeId,
    ISpawnWorldReader& world,
    SpawnReason reason,
    const Vector3i& pos,
    math::IRandom& random)
{
    // 获取放置条目
    const PlacementEntry* entry = getPlacementEntry(entityTypeId);
    if (!entry) {
        // 未注册的实体类型默认允许生成
        return true;
    }

    // 检查放置类型条件
    if (!canSpawnAtLocation(entry->placementType, world, pos, entityTypeId)) {
        return false;
    }

    // 检查自定义谓词
    if (entry->predicate) {
        return entry->predicate(world, pos, entityTypeId, random, reason);
    }

    return true;
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

// ============================================================================
// 特殊生成谓词
// ============================================================================

namespace {

bool _canTaggedAnimalSpawn(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& id, math::IRandom&, SpawnReason)
{
    // TODO: 谓词差分还需补齐未衰减的原始亮度读取，以及试炼刷怪笼的光照豁免。
    const std::string tag = id == "minecraft:armadillo" ? "minecraft:armadillo_spawnable_on"
        : id == "minecraft:camel"                       ? "minecraft:camels_spawnable_on"
        : id == "minecraft:goat"                        ? "minecraft:goats_spawnable_on"
                                                        : "minecraft:animals_spawnable_on";
    const auto* surface = world.getBlockState(pos.x, pos.y - 1, pos.z);
    const auto* spawnable = BlockTags::getTag(ResourceLocation(tag));
    return surface != nullptr && spawnable != nullptr && spawnable->contains(*surface) &&
        world.getMaxLocalRawBrightness(pos.x, pos.y, pos.z) > 8;
}

bool _canAnyLightMonsterSpawn(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& id, math::IRandom&, SpawnReason reason)
{
    const auto* below = world.getBlockState(pos.x, pos.y - 1, pos.z);
    return world.difficulty() != Difficulty::Peaceful &&
        (isSpawnerReason(reason) || (below != nullptr && _isValidSpawnSurface(*below, id)));
}

/**
 * @brief 青蛙生成条件检查
 *
 * 对应 vanilla Myers 1.21.11 `Frog.checkFrogSpawnRules`：
 *   - 下方方块属于 #minecraft:frogs_spawnable_on 标签；
 *   - 位置亮度足够（`isBrightEnoughToSpawn`：原始亮度 > 8）。
 */
bool _canFrogSpawn(const ISpawnWorldReader& world,
    const Vector3i& pos,
    const std::string& /*entityTypeId*/,
    math::IRandom& /*random*/,
    SpawnReason /*reason*/)
{
    const BlockState* below = world.getBlockState(pos.x, pos.y - 1, pos.z);
    if (below == nullptr || !BlockTags::FROGS_SPAWNABLE_ON().contains(*below)) {
        return false;
    }
    constexpr i32 MIN_SPAWN_BRIGHTNESS = 8;
    return world.getMaxLocalRawBrightness(pos.x, pos.y, pos.z) > MIN_SPAWN_BRIGHTNESS;
}

/**
 * @brief 蝙蝠生成条件检查
 *
 * 蝙蝠只能在光照等级 < 4 的地方生成。
 * 光照检查在 NaturalSpawner 中通过 MonsterEntity::isValidLightLevel() 进行，
 * 这里的谓词仅做基础检查。
 */
bool _canBatSpawn(const ISpawnWorldReader& /*world*/,
    const Vector3i& /*pos*/,
    const std::string& /*entityTypeId*/,
    math::IRandom& /*random*/,
    SpawnReason /*reason*/)
{
    // 光照检查在 NaturalSpawner 中进行，这里返回 true
    return true;
}

/**
 * @brief 鹦鹉螺生成条件检查
 *
 * 对应 MC 1.21.11 AbstractNautilus.checkNautilusSpawnRules：
 * - Y >= seaLevel - 25 且 Y <= seaLevel - 5（海平面下方 5~25 格）
 * - 下方方块为水（fluidState 是 WATER）
 * - 上方方块为 WATER 方块
 */
bool _canNautilusSpawn(const ISpawnWorldReader& world,
    const Vector3i& pos,
    const std::string& /*entityTypeId*/,
    math::IRandom& /*random*/,
    SpawnReason /*reason*/)
{
    // Y 坐标范围：[seaLevel - 25, seaLevel - 5]
    const i32 seaLevel = world::SEA_LEVEL;
    const i32 minY = seaLevel - 25;
    const i32 maxY = seaLevel - 5;
    if (pos.y < minY || pos.y > maxY) {
        return false;
    }

    // 下方方块必须是水（通过 Material::WATER 判断）
    const BlockState* belowState = world.getBlockState(pos.x, pos.y - 1, pos.z);
    if (belowState == nullptr) {
        return false;
    }
    const Material& belowMaterial = belowState->getMaterial();
    if (!belowMaterial.isLiquid() || &belowMaterial != &Material::WATER) {
        return false;
    }

    // 上方方块必须是 WATER 方块（对应 MC Blocks.WATER）
    const BlockState* aboveState = world.getBlockState(pos.x, pos.y + 1, pos.z);
    if (aboveState == nullptr) {
        return false;
    }
    if (!aboveState->is(VanillaBlocks::WATER)) {
        return false;
    }

    return true;
}

/**
 * @brief 怪物生成条件检查（带光照）
 *
 * 完整复刻 vanilla `Monster.isDarkEnoughToSpawn` 的两阶段判定：
 *   1. 原始天空光快速拒绝：`getRawSkyLight(pos) > random(32)` 时直接判定过亮；
 *   2. 有效亮度判定：`getMaxLocalRawBrightness(pos) <= 7`
 *      （该值已含 dayTime 驱动的天空光衰减与方块光取大）。
 *
 * 两阶段都放在谓词内，使谓词成为光照门槛的唯一来源：刷怪笼与自然生成走同一份逻辑，
 * 不再各自补做第一阶段。
 */
bool _canMonsterSpawnInLightPredicate(const ISpawnWorldReader& world,
    const Vector3i& pos,
    const std::string& /*entityTypeId*/,
    math::IRandom& random,
    SpawnReason /*reason*/)
{
    // TODO: 谓词差分需覆盖维度光照参数、难度、生成原因与随机采样顺序。
    if (world.getRawSkyLight(pos.x, pos.y, pos.z) > random.nextInt(32)) {
        return false;
    }
    constexpr i32 MONSTER_SPAWN_LIGHT_LIMIT = 7;
    return world.getMaxLocalRawBrightness(pos.x, pos.y, pos.z) <= MONSTER_SPAWN_LIGHT_LIMIT;
}

/**
 * @brief 史莱姆生成条件检查
 *
 * 复刻 MC 原版 Slime.checkSlimeSpawnRules 逻辑。
 *
 * 三条路径，按优先级：
 * 1. 刷怪笼生成（SpawnReason::Spawner）：跳过史莱姆区块和沼泽条件检查，
 *    直接使用通用怪物生成规则（仅检查非和平难度）。
 * 2. 沼泽地表生成：生物群系标签 ALLOWS_SURFACE_SLIME_SPAWNS + Y∈(50,70) +
 *    月相概率 + 亮度<=random(8)
 * 3. 地下史莱姆区块生成：isSlimeChunk + random(10)==0 + Y<40
 *    门控口径与原始版一致——原始版只要求关卡可作为世界生成关卡（ServerLevel 已实现该
 *    接口），与生成原因无关，故运行时的自然生成同样适用本路径。
 */
bool _canSlimeSpawn(const ISpawnWorldReader& world,
    const Vector3i& pos,
    const std::string& /*entityTypeId*/,
    math::IRandom& random,
    SpawnReason reason)
{
    // 和平难度不生成史莱姆
    if (world.difficulty() == Difficulty::Peaceful) {
        return false;
    }

    // 路径1：刷怪笼生成 — 跳过史莱姆区块和沼泽条件检查，直接允许
    if (isSpawnerReason(reason)) {
        return true;
    }

    // 路径2：地表沼泽史莱姆生成路径
    const BiomeId biome = world.getBiome(pos.x, pos.y, pos.z);
    if (biome::BiomeTags::ALLOWS_SURFACE_SLIME_SPAWNS().contains(biome)) {
        // Y 需要在 (50, 70) 开区间内
        if (pos.y > 50 && pos.y < 70) {
            // 生成概率受月相影响
            const i32 moonPhase = InternalLightUtils::getMoonPhase(world.dayTime());
            const f32 spawnChance = SlimeChunkChecker::getSurfaceSlimeSpawnChance(moonPhase);
            if (random.nextFloat() < spawnChance) {
                // 光照等级 <= 随机值(0-7)
                const i32 brightness = world.getMaxLocalRawBrightness(pos.x, pos.y, pos.z);
                if (brightness <= random.nextInt(8)) {
                    return true;
                }
            }
        }
    }

    // 路径3：地下史莱姆区块生成路径
    //
    // 门控口径：原始版在进入本路径前只检查「关卡可作为世界生成关卡」，不检查生成原因；
    // ServerLevel 实现了该接口，故运行时的自然生成同样走本路径。此前把门控写成
    // 「仅 ChunkGeneration」会使运行时地下史莱姆区块完全失效（史莱姆只剩沼泽一条途径），
    // 与原始版不符，故按同样口径取消生成原因限制。
    // 抽取顺序沿用原始版：先消耗 1/10 概率判定，再依次检查史莱姆区块与 Y 高度。
    if (random.nextInt(10) == 0) {
        const i32 chunkX = pos.x >> world::CHUNK_SHIFT;
        const i32 chunkZ = pos.z >> world::CHUNK_SHIFT;

        // 使用世界种子确定性判断是否为史莱姆区块（10% 概率）
        if (SlimeChunkChecker::isSlimeChunk(world.seed(), chunkX, chunkZ) && pos.y < 40) {
            return true;
        }
    }

    return false;
}

/**
 * @brief 岩浆怪生成条件检查
 */
bool _canMagmaCubeSpawn(const ISpawnWorldReader& /*world*/,
    const Vector3i& /*pos*/,
    const std::string& /*entityTypeId*/,
    math::IRandom& /*random*/,
    SpawnReason /*reason*/)
{
    // 岩浆怪在下界生成，无特殊条件
    return true;
}

/**
 * @brief 恶魂生成条件检查
 *
 * 恶魂需要有足够的生成空间。
 */
bool _canGhastSpawn(const ISpawnWorldReader& world,
    const Vector3i& pos,
    const std::string& /*entityTypeId*/,
    math::IRandom& /*random*/,
    SpawnReason /*reason*/)
{
    // 恶魂需要 4x4x4 的空间
    // 检查周围是否有足够空间
    for (i32 dx = 0; dx < 4; ++dx) {
        for (i32 dy = 0; dy < 4; ++dy) {
            for (i32 dz = 0; dz < 4; ++dz) {
                const BlockState* state = world.getBlockState(pos.x + dx, pos.y + dy, pos.z + dz);
                if (state && state->isSolid()) {
                    return false;
                }
            }
        }
    }
    return true;
}

/**
 * @brief 疣猪兽生成条件检查
 *
 * 疣猪兽不能生成在诡异疣块上方（诡异森林是它的禁区，避免与本生物群系的其他生物重叠）。
 * 原始版的疣猪兽生成规则只有这一条，不含光照判定，因此本谓词一旦注册即代表该实体的
 * 完整生成门槛（谓词存在时不再按分类追加光照门槛）。
 */
bool _canHoglinSpawn(const ISpawnWorldReader& world,
    const Vector3i& pos,
    const std::string& /*entityTypeId*/,
    math::IRandom& /*random*/,
    SpawnReason /*reason*/)
{
    const BlockState* belowState = world.getBlockState(pos.x, pos.y - 1, pos.z);
    if (belowState != nullptr && belowState->is(VanillaBlocks::NETHER_WART_BLOCK)) {
        return false;
    }
    return true;
}

} // anonymous namespace

// ============================================================================
// 初始化默认规则
// ============================================================================

void EntitySpawnPlacementRegistry::initializeDefaults()
{
    if (s_initialized) {
        return;
    }

    // ========== 水生生物 ==========
    registerPlacement("minecraft:cod", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:salmon", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:pufferfish", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:tropical_fish", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:squid", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:glow_squid", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:dolphin", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:axolotl", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:drowned", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:guardian", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:elder_guardian", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    // 鹦鹉螺：带 Y 范围 + 上下方块检查的生成规则（对应 MC AbstractNautilus.checkNautilusSpawnRules）
    registerPlacement(
        "minecraft:nautilus", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, _canNautilusSpawn);

    // ========== 陆生动物 ==========
    registerPlacement("minecraft:pig", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    for (const auto* id : {"minecraft:armadillo", "minecraft:camel", "minecraft:goat", "minecraft:happy_ghast"}) {
        registerPlacement(id, PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _canTaggedAnimalSpawn);
    }
    registerPlacement("minecraft:cow", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:sheep", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:chicken", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:frog", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _canFrogSpawn);
    registerPlacement("minecraft:horse", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:donkey", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:mule", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:llama", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:wolf", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:cat", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:ocelot", PlacementType::OnGround, HeightmapType::MotionBlocking);
    registerPlacement("minecraft:rabbit", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:polar_bear", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:panda", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:fox", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:parrot", PlacementType::OnGround, HeightmapType::MotionBlocking);
    registerPlacement("minecraft:mooshroom", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:turtle", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:trader_llama", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:wandering_trader", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    // 骷髅马是生物分类，按地面 + 明亮光照生成。
    registerPlacement("minecraft:skeleton_horse", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);

    // ========== 怪物（带光照检查）==========
    registerPlacement("minecraft:zombie",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:skeleton",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:creeper",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:cave_spider",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:enderman",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:witch",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:stray",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:giant",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:wither_skeleton",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:zombie_villager",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    // 僵尸马是怪物分类：数据包将其置于 monster 生成列表，须按低光照门槛生成。
    // 若漏掉此光照谓词与 Monster 分类，僵尸马会在白天露天大量生成。
    registerPlacement("minecraft:zombie_horse",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:wither",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:spider",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:evoker",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:illusioner",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:vex",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:vindicator",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:ravager",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:pillager",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);
    for (const auto* id : {"minecraft:bogged", "minecraft:creaking", "minecraft:parched", "minecraft:camel_husk"}) {
        // TODO: parched/camel_husk 还需要与通用怪物谓词一起补齐地表天空门槛。
        registerPlacement(
            id, PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _canMonsterSpawnInLightPredicate);
    }
    for (const auto* id : {"minecraft:breeze", "minecraft:zoglin"}) {
        registerPlacement(id, PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _canAnyLightMonsterSpawn);
    }

    // ========== 特殊怪物 ==========
    // 蝙蝠：需要光照 < 4
    registerPlacement("minecraft:bat", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _canBatSpawn);

    // 史莱姆：史莱姆区块或沼泽
    registerPlacement(
        "minecraft:slime", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _canSlimeSpawn);

    // 岩浆怪：下界无特殊条件
    registerPlacement(
        "minecraft:magma_cube", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _canMagmaCubeSpawn);

    // 恶魂：需要足够空间
    registerPlacement(
        "minecraft:ghast", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _canGhastSpawn);

    // 烈焰人：下界无特殊条件（使用 MonsterEntity::canMonsterSpawn）
    registerPlacement("minecraft:blaze", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);

    // 蠹虫：特殊生成（要塞石头中）
    registerPlacement("minecraft:silverfish", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);

    // 末影螨：无特殊条件
    registerPlacement("minecraft:endermite", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);

    // 尸壳：沙漠僵尸（需要温度检查）
    registerPlacement("minecraft:husk",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        _canMonsterSpawnInLightPredicate);

    // 下界生物
    registerPlacement("minecraft:zombified_piglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:piglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    // 疣猪兽：禁止生成在诡异疣块上方（详见 _canHoglinSpawn 注释）。
    registerPlacement(
        "minecraft:hoglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, _canHoglinSpawn);

    // ========== 环境生物 ==========

    // ========== 岩浆生物 ==========
    registerPlacement("minecraft:strider", PlacementType::InLava, HeightmapType::MotionBlockingNoLeaves);

    // ========== 特殊生物 ==========
    registerPlacement("minecraft:iron_golem", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:snow_golem", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:villager", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:ender_dragon", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:phantom", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:shulker", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);

    s_initialized = true;
}

bool EntitySpawnPlacementRegistry::isInitialized()
{
    return s_initialized;
}

} // namespace mc::world::spawn
