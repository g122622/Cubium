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
#include "../../util/math/random/Random.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/block/Block.hpp"
#include "../../world/block/Material.hpp"
#include <spdlog/spdlog.h>

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
    math::Random& random)
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
        return entry->predicate(world, pos, entityTypeId);
    }

    return true;
}

// ============================================================================
// 放置条件检查
// ============================================================================

bool EntitySpawnPlacementRegistry::checkOnGroundSpawn(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId)
{
    // 参考 MC 1.16.5 WorldEntitySpawner.canSpawnAtBody (ON_GROUND case)

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
    if (!isValidSpawnBlock(world, pos, entityTypeId)) {
        return false;
    }

    // 检查上方位置（对于高度 > 1 的生物）
    const Vector3i posAbove(pos.x, pos.y + 1, pos.z);
    if (!isValidSpawnBlock(world, posAbove, entityTypeId)) {
        return false;
    }

    return true;
}

bool EntitySpawnPlacementRegistry::checkInWaterSpawn(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId)
{
    // 参考 MC 1.16.5 WorldEntitySpawner.canSpawnAtBody (IN_WATER case)

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
    // 参考 MC 1.16.5 WorldEntitySpawner.canSpawnAtBody (IN_LAVA case)

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

bool EntitySpawnPlacementRegistry::isValidSpawnBlock(
    const ISpawnWorldReader& world, const Vector3i& pos, const std::string& /*entityTypeId*/)
{
    // 参考 MC 1.16.5 WorldEntitySpawner.isValidEmptySpawnBlock

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state) {
        return true; // 空气或未加载区域
    }

    // 不能是实心方块
    if (state->isSolid()) {
        return false;
    }

    // 检查方块是否阻止生成
    if (blockPreventsSpawn(state)) {
        return false;
    }

    // 不能是流体
    const Material& material = state->getMaterial();
    if (material.isLiquid()) {
        return false;
    }

    return true;
}

bool EntitySpawnPlacementRegistry::blockPreventsSpawn(const BlockState* state)
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
// 初始化默认规则
// ============================================================================

void EntitySpawnPlacementRegistry::initializeDefaults()
{
    if (s_initialized) {
        return;
    }

    // 参考 MC 1.16.5 EntitySpawnPlacementRegistry 静态初始化块

    // ========== 水生生物 ==========
    registerPlacement("minecraft:cod", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:salmon", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:pufferfish", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:tropical_fish", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:squid", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:dolphin", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:drowned", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:guardian", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:elder_guardian", PlacementType::InWater, HeightmapType::MotionBlockingNoLeaves);

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

    // ========== 怪物 ==========
    registerPlacement("minecraft:zombie", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:skeleton", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:creeper", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:spider", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:cave_spider", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:enderman", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:witch", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:husk", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:stray", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:blaze", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:ghast", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:magma_cube", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:slime", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:silverfish", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:endermite", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:wither", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:wither_skeleton", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:zombie_villager", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:zombified_piglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:piglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:hoglin", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:pillager", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:vindicator", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:evoker", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:illusioner", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:vex", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:ravager", PlacementType::NoRestrictions, HeightmapType::MotionBlockingNoLeaves);
    registerPlacement("minecraft:giant", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);

    // ========== 环境生物 ==========
    registerPlacement("minecraft:bat", PlacementType::OnGround, HeightmapType::MotionBlockingNoLeaves);

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
    spdlog::debug("EntitySpawnPlacementRegistry: Initialized {} entity placements", s_registry.size());
}

bool EntitySpawnPlacementRegistry::isInitialized()
{
    return s_initialized;
}

} // namespace mc::world::spawn
