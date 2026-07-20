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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "MobSpawnerBlockEntity.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/biome/Biomes.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::blockentity {

// ============================================================================
// 构造函数
// ============================================================================

MobSpawnerBlockEntity::MobSpawnerBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::MobSpawner, pos)
{}

// ============================================================================
// BlockEntity 接口
// ============================================================================

void MobSpawnerBlockEntity::tick(IWorld& world)
{
    _serverTick(world);
}

bool MobSpawnerBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    if (data.contains("spawn_delay") && data["spawn_delay"].is_number()) {
        m_spawnDelay = data["spawn_delay"].get<i32>();
    }
    if (data.contains("min_spawn_delay") && data["min_spawn_delay"].is_number()) {
        m_minSpawnDelay = data["min_spawn_delay"].get<i32>();
    }
    if (data.contains("max_spawn_delay") && data["max_spawn_delay"].is_number()) {
        m_maxSpawnDelay = data["max_spawn_delay"].get<i32>();
    }
    if (data.contains("spawn_count") && data["spawn_count"].is_number()) {
        m_spawnCount = data["spawn_count"].get<i32>();
    }
    if (data.contains("max_nearby_entities") && data["max_nearby_entities"].is_number()) {
        m_maxNearbyEntities = data["max_nearby_entities"].get<i32>();
    }
    if (data.contains("required_player_range") && data["required_player_range"].is_number()) {
        m_requiredPlayerRange = data["required_player_range"].get<i32>();
    }
    if (data.contains("spawn_range") && data["spawn_range"].is_number()) {
        m_spawnRange = data["spawn_range"].get<i32>();
    }

    // 加载下一个实体 ID
    if (data.contains("next_entity_id") && data["next_entity_id"].is_string()) {
        m_nextEntityId = ResourceLocation(data["next_entity_id"].get<std::string>());
    }

    // 加载生成候选列表
    m_spawnPotentials.clear();
    if (data.contains("spawn_potentials") && data["spawn_potentials"].is_array()) {
        for (const auto& entry : data["spawn_potentials"]) {
            if (entry.contains("entity_id") && entry["entity_id"].is_string()) {
                SpawnEntry se;
                se.entityId = ResourceLocation(entry["entity_id"].get<std::string>());
                se.weight = entry.value("weight", 1);
                m_spawnPotentials.push_back(std::move(se));
            }
        }
    }

    // 加载自定义生成规则
    if (data.contains("custom_spawn_rules") && data["custom_spawn_rules"].is_object()) {
        const auto& rules = data["custom_spawn_rules"];
        CustomSpawnRules csr;
        csr.blockLightMin = rules.value("block_light_min", 0);
        csr.blockLightMax = rules.value("block_light_max", 15);
        csr.skyLightMin = rules.value("sky_light_min", 0);
        csr.skyLightMax = rules.value("sky_light_max", 15);
        m_customSpawnRules = csr;
    }

    return true;
}

void MobSpawnerBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["spawn_delay"] = m_spawnDelay;
    data["min_spawn_delay"] = m_minSpawnDelay;
    data["max_spawn_delay"] = m_maxSpawnDelay;
    data["spawn_count"] = m_spawnCount;
    data["max_nearby_entities"] = m_maxNearbyEntities;
    data["required_player_range"] = m_requiredPlayerRange;
    data["spawn_range"] = m_spawnRange;

    if (!m_nextEntityId.path().empty()) {
        data["next_entity_id"] = m_nextEntityId.toString();
    }

    // 保存生成候选列表
    auto potentials = nlohmann::json::array();
    for (const auto& entry : m_spawnPotentials) {
        potentials.push_back({{"entity_id", entry.entityId.toString()}, {"weight", entry.weight}});
    }
    data["spawn_potentials"] = std::move(potentials);

    // 保存自定义生成规则
    if (m_customSpawnRules.has_value()) {
        const auto& rules = m_customSpawnRules.value();
        data["custom_spawn_rules"] = {{"block_light_min", rules.blockLightMin},
            {"block_light_max", rules.blockLightMax},
            {"sky_light_min", rules.skyLightMin},
            {"sky_light_max", rules.skyLightMax}};
    }
}

bool MobSpawnerBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    using namespace mc::entity::serialization::nbt_helper;

    // 读取生成参数（MC Java 使用 short 标签，兼容 int）
    if (auto val = tryGetShort(tag, "Delay")) {
        m_spawnDelay = static_cast<i32>(*val);
    } else if (auto val = tryGetInt(tag, "Delay")) {
        m_spawnDelay = *val;
    }

    if (auto val = tryGetShort(tag, "MinSpawnDelay")) {
        m_minSpawnDelay = static_cast<i32>(*val);
    } else if (auto val = tryGetInt(tag, "MinSpawnDelay")) {
        m_minSpawnDelay = *val;
    }

    if (auto val = tryGetShort(tag, "MaxSpawnDelay")) {
        m_maxSpawnDelay = static_cast<i32>(*val);
    } else if (auto val = tryGetInt(tag, "MaxSpawnDelay")) {
        m_maxSpawnDelay = *val;
    }

    if (auto val = tryGetShort(tag, "SpawnCount")) {
        m_spawnCount = static_cast<i32>(*val);
    } else if (auto val = tryGetInt(tag, "SpawnCount")) {
        m_spawnCount = *val;
    }

    if (auto val = tryGetShort(tag, "MaxNearbyEntities")) {
        m_maxNearbyEntities = static_cast<i32>(*val);
    } else if (auto val = tryGetInt(tag, "MaxNearbyEntities")) {
        m_maxNearbyEntities = *val;
    }

    if (auto val = tryGetShort(tag, "RequiredPlayerRange")) {
        m_requiredPlayerRange = static_cast<i32>(*val);
    } else if (auto val = tryGetInt(tag, "RequiredPlayerRange")) {
        m_requiredPlayerRange = *val;
    }

    if (auto val = tryGetShort(tag, "SpawnRange")) {
        m_spawnRange = static_cast<i32>(*val);
    } else if (auto val = tryGetInt(tag, "SpawnRange")) {
        m_spawnRange = *val;
    }

    // 读取 SpawnData（下一个要生成的实体）
    const auto* spawnDataTag = tryGetCompound(tag, "SpawnData");
    if (spawnDataTag != nullptr) {
        // MC 1.21 格式：SpawnData.entity.id
        const auto* entityTag = tryGetCompound(*spawnDataTag, "entity");
        if (entityTag != nullptr) {
            auto idStr = tryGetString(*entityTag, "id");
            if (idStr.has_value()) {
                m_nextEntityId = ResourceLocation(*idStr);
            }
        } else {
            // 旧版格式：SpawnData.id 直接包含实体 ID
            auto idStr = tryGetString(*spawnDataTag, "id");
            if (idStr.has_value()) {
                m_nextEntityId = ResourceLocation(*idStr);
            }
        }

        // 读取 CustomSpawnRules（MC Java 格式：IntArray [min, max]）
        const auto* customRulesTag = tryGetCompound(*spawnDataTag, "CustomSpawnRules");
        if (customRulesTag != nullptr) {
            CustomSpawnRules csr;

            auto blockLightIt = customRulesTag->value.find("block_light_limit");
            if (blockLightIt != customRulesTag->value.end() && blockLightIt->second->id() == nbt::TagId::IntArray) {
                const auto& arr = dynamic_cast<const nbt::tags::intarray_tag&>(*blockLightIt->second).value;
                if (arr.size() >= 2) {
                    csr.blockLightMin = arr[0];
                    csr.blockLightMax = arr[1];
                }
            }

            auto skyLightIt = customRulesTag->value.find("sky_light_limit");
            if (skyLightIt != customRulesTag->value.end() && skyLightIt->second->id() == nbt::TagId::IntArray) {
                const auto& arr = dynamic_cast<const nbt::tags::intarray_tag&>(*skyLightIt->second).value;
                if (arr.size() >= 2) {
                    csr.skyLightMin = arr[0];
                    csr.skyLightMax = arr[1];
                }
            }

            m_customSpawnRules = csr;
        }
    }

    // 读取 SpawnPotentials
    m_spawnPotentials.clear();
    const auto* potentialsList = tryGetList(tag, "SpawnPotentials");
    if (potentialsList != nullptr && potentialsList->element_id() == nbt::TagId::Compound) {
        const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*potentialsList);
        for (const auto& entryTag : compoundList.value) {
            SpawnEntry entry;
            entry.weight = 1;

            auto weightVal = tryGetInt(entryTag, "weight");
            if (weightVal.has_value()) {
                entry.weight = *weightVal;
            }

            // MC 1.21 格式：data.entity.id
            const auto* dataTag = tryGetCompound(entryTag, "data");
            if (dataTag != nullptr) {
                const auto* entityTag = tryGetCompound(*dataTag, "entity");
                if (entityTag != nullptr) {
                    auto idStr = tryGetString(*entityTag, "id");
                    if (idStr.has_value()) {
                        entry.entityId = ResourceLocation(*idStr);
                    }
                }
            }

            if (!entry.entityId.path().empty()) {
                m_spawnPotentials.push_back(std::move(entry));
            }
        }
    }

    return true;
}

void MobSpawnerBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    tag.put("Delay", static_cast<i16>(m_spawnDelay));
    tag.put("MinSpawnDelay", static_cast<i16>(m_minSpawnDelay));
    tag.put("MaxSpawnDelay", static_cast<i16>(m_maxSpawnDelay));
    tag.put("SpawnCount", static_cast<i16>(m_spawnCount));
    tag.put("MaxNearbyEntities", static_cast<i16>(m_maxNearbyEntities));
    tag.put("RequiredPlayerRange", static_cast<i16>(m_requiredPlayerRange));
    tag.put("SpawnRange", static_cast<i16>(m_spawnRange));

    // 保存 SpawnData
    if (!m_nextEntityId.path().empty()) {
        nbt::tags::compound_tag spawnData;
        nbt::tags::compound_tag entity;
        entity.put("id", m_nextEntityId.toString());
        spawnData.value.emplace("entity", std::make_unique<nbt::tags::compound_tag>(std::move(entity)));

        // 保存 CustomSpawnRules（MC Java 格式：IntArray [min, max]）
        if (m_customSpawnRules.has_value()) {
            const auto& rules = m_customSpawnRules.value();
            nbt::tags::compound_tag customRules;
            customRules.value.emplace("block_light_limit",
                std::make_unique<nbt::tags::intarray_tag>(std::vector<i32>{rules.blockLightMin, rules.blockLightMax}));
            customRules.value.emplace("sky_light_limit",
                std::make_unique<nbt::tags::intarray_tag>(std::vector<i32>{rules.skyLightMin, rules.skyLightMax}));
            spawnData.value.emplace(
                "CustomSpawnRules", std::make_unique<nbt::tags::compound_tag>(std::move(customRules)));
        }

        tag.value.emplace("SpawnData", std::make_unique<nbt::tags::compound_tag>(std::move(spawnData)));
    }

    // 保存 SpawnPotentials
    auto potentialsList = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& entry : m_spawnPotentials) {
        nbt::tags::compound_tag entryTag;
        entryTag.put("weight", entry.weight);
        nbt::tags::compound_tag entity;
        nbt::tags::compound_tag data;
        entity.put("id", entry.entityId.toString());
        data.value.emplace("entity", std::make_unique<nbt::tags::compound_tag>(std::move(entity)));
        entryTag.value.emplace("data", std::make_unique<nbt::tags::compound_tag>(std::move(data)));
        potentialsList->value.push_back(std::move(entryTag));
    }
    tag.value.emplace("SpawnPotentials", std::move(potentialsList));
}

bool MobSpawnerBlockEntity::triggerEvent(i32 id, i32 type)
{
    if (id == 1) {
        // 刷怪笼事件：重置生成延迟，产生视觉"弹出"效果
        // 参考 MC Java: BaseSpawner.onEventTriggered()
        if (m_world != nullptr && m_world->isClientSide()) {
            m_spawnDelay = m_minSpawnDelay;
        }
        return true;
    }
    return false;
}

std::unique_ptr<BlockEntity> MobSpawnerBlockEntity::clone() const
{
    auto copy = std::make_unique<MobSpawnerBlockEntity>(m_pos);
    copy->m_spawnDelay = m_spawnDelay;
    copy->m_spawnPotentials = m_spawnPotentials;
    copy->m_nextEntityId = m_nextEntityId;
    copy->m_customSpawnRules = m_customSpawnRules;
    copy->m_minSpawnDelay = m_minSpawnDelay;
    copy->m_maxSpawnDelay = m_maxSpawnDelay;
    copy->m_spawnCount = m_spawnCount;
    copy->m_maxNearbyEntities = m_maxNearbyEntities;
    copy->m_requiredPlayerRange = m_requiredPlayerRange;
    copy->m_spawnRange = m_spawnRange;
    return copy;
}

// ============================================================================
// 配置接口
// ============================================================================

void MobSpawnerBlockEntity::setEntityId(const ResourceLocation& entityId, math::Random& rng)
{
    m_nextEntityId = entityId;

    // 如果没有生成候选列表，添加一个默认条目
    // 参考 MC Java: BaseSpawner.setEntityId() 只修改 nextSpawnData，
    // spawnPotentials 在 load() 时从 NBT 加载。此处保持候选列表与当前实体一致。
    if (m_spawnPotentials.empty()) {
        m_spawnPotentials.push_back({entityId, 1});
    }

    // 重置延迟（不调用 _delay，因为 _delay 会从 spawnPotentials 中随机选择，
    // 可能覆盖刚设置的 nextEntityId）
    if (m_maxSpawnDelay <= m_minSpawnDelay) {
        m_spawnDelay = m_minSpawnDelay;
    } else {
        m_spawnDelay = m_minSpawnDelay + rng.nextInt(m_maxSpawnDelay - m_minSpawnDelay);
    }

    setChanged();
}

void MobSpawnerBlockEntity::addSpawnPotential(const ResourceLocation& entityId, i32 weight)
{
    m_spawnPotentials.push_back({entityId, weight});
    setChanged();
}

void MobSpawnerBlockEntity::setCustomSpawnRules(const CustomSpawnRules& rules)
{
    m_customSpawnRules = rules;
    setChanged();
}

// ============================================================================
// 生成逻辑
// ============================================================================

void MobSpawnerBlockEntity::_serverTick(IWorld& world)
{
    // 检测附近是否有玩家
    if (!_isNearPlayer(world)) {
        return;
    }

    // 如果没有设置实体类型，不生成
    if (m_nextEntityId.path().empty()) {
        return;
    }

    // 如果延迟为 -1，重置延迟
    if (m_spawnDelay == -1) {
        math::Random rng(
            world.seed() ^ (static_cast<i64>(m_pos.x) * 341873128712ULL + static_cast<i64>(m_pos.z) * 132897987541ULL));
        _delay(rng);
        return;
    }

    // 递减延迟
    if (m_spawnDelay > 0) {
        --m_spawnDelay;
        return;
    }

    // 延迟归零，尝试生成
    bool spawnedAny = _spawnEntities(world);

    // 重置延迟
    math::Random rng(world.seed() ^
        (static_cast<i64>(m_pos.x) * 341873128712ULL + static_cast<i64>(m_pos.z) * 132897987541ULL) +
            world.getGameTime());
    _delay(rng);

    if (spawnedAny) {
        // 通知客户端播放刷怪笼生成粒子效果
        // 参考 MC: BaseSpawner.serverTick() 中成功生成后调用 levelEvent(2004, pos, 0)
        world.playEvent(world::WorldEvents::MOB_SPAWNER_PARTICLES, m_pos, 0);

        // 发送方块事件，使客户端重置生成延迟产生视觉"弹出"效果
        // 参考 MC Java: BaseSpawner.serverTick() 中调用 level.blockEvent(pos, block, 1, 0)
        const BlockState* state = world.getBlockState(m_pos);
        if (state != nullptr) {
            world.blockEvent(m_pos, state->getBlock(), 1, 0);
        }
    }
}

bool MobSpawnerBlockEntity::_isNearPlayer(IWorld& world) const
{
    if (m_requiredPlayerRange <= 0) {
        return true;
    }

    // 查找范围内的玩家
    auto entities = world.getEntitiesInRange(
        Vector3(static_cast<f32>(m_pos.x) + 0.5f, static_cast<f32>(m_pos.y) + 0.5f, static_cast<f32>(m_pos.z) + 0.5f),
        static_cast<f32>(m_requiredPlayerRange));

    for (auto* entity : entities) {
        if (dynamic_cast<Player*>(entity) != nullptr) {
            return true;
        }
    }
    return false;
}

void MobSpawnerBlockEntity::_delay(math::Random& rng)
{
    if (m_maxSpawnDelay <= m_minSpawnDelay) {
        m_spawnDelay = m_minSpawnDelay;
    } else {
        m_spawnDelay = m_minSpawnDelay + rng.nextInt(m_maxSpawnDelay - m_minSpawnDelay);
    }

    // 从 spawnPotentials 中随机选择下一个实体类型
    if (!m_spawnPotentials.empty()) {
        _selectNextEntity(rng);
    }
}

bool MobSpawnerBlockEntity::_spawnEntities(IWorld& world)
{
    // 获取实体类型
    const entity::EntityType* entityType = entity::EntityRegistry::instance().getType(m_nextEntityId.toString());
    if (entityType == nullptr || !entityType->canSummon()) {
        return false;
    }

    // 当 CustomSpawnRules 存在时，非和平生物（Monster 分类）在和平难度下不生成
    if (m_customSpawnRules.has_value()) {
        if (!entity::isPeaceful(entityType->classification()) &&
            !entity::combat::DifficultyHelper::allowsMobSpawning(world.difficulty())) {
            return false;
        }
    }

    bool spawnedAny = false;
    math::Random rng(world.seed() ^ world.getGameTime());

    for (i32 i = 0; i < m_spawnCount; ++i) {
        // 在 spawnRange 范围内计算随机生成位置
        f32 xOffset = static_cast<f32>(rng.nextInt(2 * m_spawnRange + 1) - m_spawnRange);
        f32 yOffset = static_cast<f32>(rng.nextInt(3) - 1);
        f32 zOffset = static_cast<f32>(rng.nextInt(2 * m_spawnRange + 1) - m_spawnRange);

        Vector3 spawnPos(static_cast<f32>(m_pos.x) + 0.5f + xOffset,
            static_cast<f32>(m_pos.y) + yOffset,
            static_cast<f32>(m_pos.z) + 0.5f + zOffset);

        BlockPos spawnBlockPos(static_cast<i32>(std::floor(spawnPos.x)),
            static_cast<i32>(std::floor(spawnPos.y)),
            static_cast<i32>(std::floor(spawnPos.z)));

        // 检查生成位置是否满足光照和生成规则
        if (!_isValidSpawnPosition(world, spawnBlockPos, *entityType)) {
            continue;
        }

        // 检查附近同类型实体数量
        i32 nearbyCount = _countNearbyEntities(world, m_nextEntityId);
        if (nearbyCount >= m_maxNearbyEntities) {
            return spawnedAny;
        }

        // 创建实体
        auto entity = entityType->create(&world);
        if (entity == nullptr) {
            continue;
        }

        entity->setPosition(spawnPos);
        entity->setRotation(rng.nextFloat() * 360.0f, 0.0f);

        // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化（使用位置感知的区域难度）
        auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
        if (mobEntity != nullptr) {
            entity::combat::DifficultyInstance difficulty = entity::combat::DifficultyInstance::at(world,
                BlockPos(static_cast<i32>(std::floor(spawnPos.x)),
                    static_cast<i32>(spawnPos.y),
                    static_cast<i32>(std::floor(spawnPos.z))));
            mobEntity->finalizeSpawn(world, difficulty, world::spawn::SpawnReason::Spawner);
        }

        // 尝试添加到世界
        EntityInstanceId id = world.spawnEntity(std::move(entity));
        if (id != EntityInstanceId(0)) {
            spawnedAny = true;
        }
    }

    return spawnedAny;
}

bool MobSpawnerBlockEntity::_isValidSpawnPosition(
    IWorld& world, const BlockPos& spawnPos, const entity::EntityType& entityType) const
{
    if (m_customSpawnRules.has_value()) {
        // 当 CustomSpawnRules 存在时，检查方块光照和天空光照是否在指定范围内。
        // 对应 MC Java 的 SpawnData.CustomSpawnRules.isValidPosition()。
        // 注意：CustomSpawnRules 使用原始光照值，不应用天空变暗（skyDarkening）。
        const u8 blockLight = world.getBlockLight(spawnPos);
        const u8 skyLight = world.getSkyLight(spawnPos);
        return m_customSpawnRules->isValidPosition(blockLight, skyLight);
    }

    // 当 CustomSpawnRules 不存在时，使用默认的生成放置规则检查。
    // 对应 MC Java 的 SpawnPlacements.checkSpawnRules()。
    // EntitySpawnPlacementRegistry::canSpawnEntity() 会根据实体类型注册的
    // PlacementType（OnGround/InWater/InLava/NoRestrictions）和自定义谓词进行检查。
    // 需要创建一个适配器将 IWorld 转换为 ISpawnWorldReader。
    class SpawnerWorldAdapter final : public world::spawn::ISpawnWorldReader {
    public:
        explicit SpawnerWorldAdapter(IWorld& world)
            : m_world(world)
        {}

        [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
        {
            return m_world.getBlockState(BlockPos(x, y, z));
        }

        [[nodiscard]] bool isInWorldBounds(i32 x, i32 y, i32 z) const override
        {
            return m_world.isWithinWorldBounds(x, y, z);
        }

        [[nodiscard]] i32 getHeight(world::chunk::HeightmapType type, i32 x, i32 z) const override
        {
            const ChunkCoord chunkX = world::toChunkCoord(x);
            const ChunkCoord chunkZ = world::toChunkCoord(z);
            const ChunkData* chunk = m_world.getChunk(chunkX, chunkZ);
            if (chunk == nullptr) {
                return m_world.getHeight(x, z);
            }
            const i32 localX = world::toLocalCoord(x);
            const i32 localZ = world::toLocalCoord(z);
            return chunk->getTopBlockY(type, localX, localZ);
        }

        [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override
        {
            const ChunkCoord chunkX = world::toChunkCoord(x);
            const ChunkCoord chunkZ = world::toChunkCoord(z);
            const ChunkData* chunk = m_world.getChunk(chunkX, chunkZ);
            if (chunk == nullptr) {
                return Biomes::Plains;
            }
            const i32 localX = world::toLocalCoord(x);
            const i32 localZ = world::toLocalCoord(z);
            return chunk->getBiomeAtBlock(localX, y, localZ);
        }

        [[nodiscard]] u64 seed() const override { return m_world.seed(); }

        [[nodiscard]] Difficulty difficulty() const override { return m_world.difficulty(); }

        [[nodiscard]] i64 dayTime() const override { return m_world.dayTime(); }

        [[nodiscard]] i32 getMaxLocalRawBrightness(i32 x, i32 y, i32 z) const override
        {
            return m_world.getMaxLocalRawBrightness(BlockPos(x, y, z));
        }

    private:
        IWorld& m_world;
    };

    SpawnerWorldAdapter adapter(world);
    // 刷怪笼生成使用 SpawnReason::Spawner
    math::Random rng(world.seed() ^ world.getGameTime());
    return world::spawn::EntitySpawnPlacementRegistry::canSpawnEntity(entityType.name(),
        adapter,
        world::spawn::SpawnReason::Spawner,
        Vector3i(spawnPos.x, spawnPos.y, spawnPos.z),
        rng);
}

i32 MobSpawnerBlockEntity::_countNearbyEntities(IWorld& world, const ResourceLocation& entityId) const
{
    // 在膨胀区域内查找同类型实体
    constexpr f32 NEARBY_ENTITY_RANGE = 16.0f;
    auto entities = world.getEntitiesInRange(
        Vector3(static_cast<f32>(m_pos.x) + 0.5f, static_cast<f32>(m_pos.y) + 0.5f, static_cast<f32>(m_pos.z) + 0.5f),
        NEARBY_ENTITY_RANGE);

    i32 count = 0;
    for (auto* entity : entities) {
        if (entity->getTypeId() == entityId.toString()) {
            ++count;
        }
    }
    return count;
}

void MobSpawnerBlockEntity::_selectNextEntity(math::Random& rng)
{
    if (m_spawnPotentials.empty()) {
        return;
    }

    // 加权随机选择
    i32 totalWeight = 0;
    for (const auto& entry : m_spawnPotentials) {
        totalWeight += entry.weight;
    }

    if (totalWeight <= 0) {
        m_nextEntityId = m_spawnPotentials[0].entityId;
        return;
    }

    i32 roll = rng.nextInt(totalWeight);
    i32 accumulated = 0;
    for (const auto& entry : m_spawnPotentials) {
        accumulated += entry.weight;
        if (roll < accumulated) {
            m_nextEntityId = entry.entityId;
            return;
        }
    }

    // 回退到最后一项
    m_nextEntityId = m_spawnPotentials.back().entityId;
}

} // namespace mc::blockentity
