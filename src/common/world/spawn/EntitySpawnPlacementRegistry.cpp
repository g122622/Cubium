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
#include "util/assert/AssertAll.hpp"
#include "util/math/random/IRandom.hpp"
#include "world/IWorld.hpp"
#include "world/WorldConstants.hpp"
#include "world/biome/BiomeTags.hpp"
#include "world/block/Block.hpp"
#include "world/block/Material.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include "world/lighting/InternalLightUtils.hpp"
#include "world/spawn/SlimeChunkChecker.hpp"

namespace mc::world::spawn {

// 静态成员定义
std::unordered_map<std::string, EntitySpawnPlacementRegistry::PlacementEntry> EntitySpawnPlacementRegistry::s_registry;
bool EntitySpawnPlacementRegistry::s_initialized = false;

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
    if (!world.isInWorldBounds(pos.x, pos.y, pos.z)) {
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
    // 检查脚下方块是否允许生成
    const Vector3i posBelow(pos.x, pos.y - 1, pos.z);
    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(posBelow.x, posBelow.y, posBelow.z);
    if (!belowState) {
        return false;
    }

    bool hasSurfaceSupport = belowState->isSolid();

    if (const IWorld* worldReader = dynamic_cast<const IWorld*>(&world)) {
        hasSurfaceSupport = belowState->isSolidSide(const_cast<IWorld&>(*worldReader), belowPos, Direction::Up);
    }

    if (!hasSurfaceSupport) {
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
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId)
{
    // 当前位置必须是水
    const BlockState* currentState = world.getBlockState(pos.x, pos.y, pos.z);
    if (!currentState) {
        return false;
    }

    const Material& material = currentState->getMaterial();
    if (!material.isLiquid()) {
        return false;
    }

    // 检查是否是水材质（通过与 WATER 静态实例比较）
    if (&material != &Material::WATER) {
        return false;
    }

    // 下方位置也必须是水（确保足够深度）
    const Vector3i posBelow(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(posBelow.x, posBelow.y, posBelow.z);
    if (!belowState) {
        return false;
    }

    const Material& belowMaterial = belowState->getMaterial();
    if (!belowMaterial.isLiquid() || &belowMaterial != &Material::WATER) {
        return false;
    }

    // 上方不能是实心方块
    const Vector3i posAbove(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(posAbove.x, posAbove.y, posAbove.z);
    if (aboveState && aboveState->isSolid()) {
        return false;
    }

    return true;
}

bool EntitySpawnPlacementRegistry::checkInLavaSpawn(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId)
{
    // 当前位置必须是岩浆
    const BlockState* currentState = world.getBlockState(pos.x, pos.y, pos.z);
    if (!currentState) {
        return false;
    }

    const Material& material = currentState->getMaterial();
    if (!material.isLiquid()) {
        return false;
    }

    // 检查是否是岩浆材质（通过与 LAVA 静态实例比较）
    if (&material != &Material::LAVA) {
        return false;
    }

    return true;
}

bool EntitySpawnPlacementRegistry::_isValidSpawnBlock(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& /*entityTypeId*/)
{
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state) {
        return true; // 空气或未加载区域
    }

    // 不能是实心方块
    if (state->isSolid()) {
        return false;
    }

    // 检查方块是否阻止生成
    if (_blockPreventsSpawn(state)) {
        return false;
    }

    // 不能是流体
    const Material& material = state->getMaterial();
    if (material.isLiquid()) {
        return false;
    }

    return true;
}

bool EntitySpawnPlacementRegistry::_blockPreventsSpawn(const BlockState* state)
{
    if (!state) {
        return false;
    }

    if (state->isSolid()) {
        return false;
    }

    return state->getShape().isFullBlock() || state->getCollisionShape().isFullBlock();
}

// ============================================================================
// 特殊生成谓词
// ============================================================================

namespace {

/**
 * @brief 蝙蝠生成条件检查
 *
 * 蝙蝠只能在光照等级 < 4 的地方生成。
 * 光照检查在 NaturalSpawner 中通过 MonsterEntity::isValidLightLevel() 进行，
 * 这里的谓词仅做基础检查。
 */
bool canBatSpawn(const ISpawnWorldReader& /*world*/,
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
bool canNautilusSpawn(const ISpawnWorldReader& world,
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
 * 怪物需要光照等级满足 isValidLightLevel() 条件。
 * 光照检查在 NaturalSpawner 中进行。
 */
bool canMonsterSpawnInLightPredicate(const ISpawnWorldReader& /*world*/,
    const Vector3i& /*pos*/,
    const std::string& /*entityTypeId*/,
    math::IRandom& /*random*/,
    SpawnReason /*reason*/)
{
    // 光照检查在 NaturalSpawner 中进行，这里返回 true
    return true;
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
 * 3. 地下史莱姆区块生成：仅限 ChunkGeneration 阶段 + isSlimeChunk + random(10)==0 + Y<40
 */
bool canSlimeSpawn(const ISpawnWorldReader& world,
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

    // 路径3：地下史莱姆区块生成路径（仅限区块生成阶段）
    if (reason == SpawnReason::ChunkGeneration) {
        if (pos.y < 40) {
            const i32 chunkX = pos.x >> world::CHUNK_SHIFT;
            const i32 chunkZ = pos.z >> world::CHUNK_SHIFT;

            // 使用世界种子确定性判断是否为史莱姆区块（10% 概率）
            if (SlimeChunkChecker::isSlimeChunk(world.seed(), chunkX, chunkZ)) {
                // 额外 10% 随机概率通过
                if (random.nextInt(10) == 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * @brief 岩浆怪生成条件检查
 */
bool canMagmaCubeSpawn(const ISpawnWorldReader& /*world*/,
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
bool canGhastSpawn(const ISpawnWorldReader& world,
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
        "minecraft:nautilus", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves, canNautilusSpawn);
    // 僵尸鹦鹉螺：MC 1.21.11 未在 SpawnPlacements 注册，仅作为溺尸骑乘者生成
    // 这里注册为 InWater 仅用于刷怪蛋/命令生成场景
    registerPlacement("minecraft:zombie_nautilus", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);

    // ========== 陆生动物 ==========
    registerPlacement("minecraft:pig", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:cow", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:sheep", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:chicken", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
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
    registerPlacement(
        "minecraft:wandering_trader", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:zombie_horse", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:skeleton_horse", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);

    // ========== 怪物（带光照检查）==========
    registerPlacement("minecraft:zombie",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:skeleton",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:creeper",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:cave_spider",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:enderman",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:witch",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:stray",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:giant",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:wither_skeleton",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:zombie_villager",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:wither",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:spider",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:evoker",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:illusioner",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:vex",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:vindicator",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:ravager",
        PlacementType::NoRestrictions,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);
    registerPlacement("minecraft:pillager",
        PlacementType::OnGround,
        HeightmapType::MotionBlockingNoLeaves,
        canMonsterSpawnInLightPredicate);

    // ========== 特殊怪物 ==========
    // 蝙蝠：需要光照 < 4
    registerPlacement("minecraft:bat", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, canBatSpawn);

    // 史莱姆：史莱姆区块或沼泽
    registerPlacement("minecraft:slime", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, canSlimeSpawn);

    // 岩浆怪：下界无特殊条件
    registerPlacement(
        "minecraft:magma_cube", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, canMagmaCubeSpawn);

    // 恶魂：需要足够空间
    registerPlacement("minecraft:ghast", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves, canGhastSpawn);

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
        canMonsterSpawnInLightPredicate);

    // 下界生物
    registerPlacement("minecraft:zombified_piglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:piglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:hoglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);

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
