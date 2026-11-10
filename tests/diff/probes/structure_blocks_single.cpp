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

/**
 * @file structure_blocks_single.cpp
 * @brief 差分能力：单个结构在指定区块上的方块产物。
 *
 * `structure_start_single` 只比到「起点由哪些构件组成」，构件的 `generate`（原版
 * `postProcess`）真正往世界里写了什么方块完全没有覆盖。这条能力在起点的同一套随机源上
 * 继续走一步：把结构起点落在的每个区块都跑一遍构件放置，最后按坐标输出该区块内的方块。
 *
 * 世界是一个只记录写入的空世界（读到的方块永远是空气），两侧必须表现得完全一致：
 * 任何「读取地形后改变行为」的差异都会立刻暴露成方块差异，而不是被真实地形掩盖。
 *
 * 每个区块的随机源与生产一致：`setDecorationSeed(seed, chunkMinX, chunkMinZ)` 之后
 * `setFeatureSeed(decorSeed, 0, stageOrdinal)`，再交给该区块里全部相交构件依次消费。
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "probe.hpp"
#include "probe_world.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "structure_probe_common.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using namespace world::gen::structure;

/**
 * @brief 确定性平坦地形。
 *
 * 探针与 oracle 各自按同一份参数构造地形：`surfaceY` 及以下是固体，`waterTopY` 及以下
 * （且高于 `surfaceY`）是水源方块。`enabled=false` 时保持空世界。
 */
struct TerrainConfig {
    bool enabled = false;
    i32 surfaceY = 0;
    i32 waterTopY = 0;
    const BlockState* solid = nullptr;
    const BlockState* fluid = nullptr;
};

/**
 * @brief 按维度选择固定地形参数。
 *
 * 主世界：y<=50 石头、51..62 水；下界：y<=31 下界岩；末地：y<=50 末地石。
 * 数值固定而不用随机高度，是为了让探针与 oracle 完全不共享代码也能对上。
 */
TerrainConfig makeTerrainConfig(std::string_view dimension, bool enabled)
{
    TerrainConfig config;
    config.enabled = enabled;
    if (!enabled) {
        return config;
    }

    if (dimension == "the_nether") {
        config.surfaceY = 31;
        config.waterTopY = 31;
        config.solid = &VanillaBlocks::NETHERRACK->defaultState();
        return config;
    }
    if (dimension == "the_end") {
        config.surfaceY = 50;
        config.waterTopY = 50;
        config.solid = &VanillaBlocks::END_STONE->defaultState();
        return config;
    }

    config.surfaceY = 50;
    config.waterTopY = 62;
    config.solid = &VanillaBlocks::STONE->defaultState();
    config.fluid = &VanillaBlocks::WATER->defaultState();
    return config;
}

/**
 * @brief 只记录写入的假世界（可选平坦地形）。
 *
 * 读取先查写入记录，未命中时返回地形方块（无地形时是空气），写入按坐标去重后保留
 * 最后一次写入。最低高度按维度给出：`fillColumnDown` 会一路填到世界底部，写穿维度
 * 范围会引入与结构无关的差异。
 */
class RecordingWorld final : public probe::NullWorld {
public:
    RecordingWorld(i32 minBuildHeight, TerrainConfig terrain, u64 seed)
        : m_minBuildHeight(minBuildHeight)
        , m_terrain(terrain)
        , m_seed(seed)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto found = m_blocks.find(BlockPos(x, y, z).asLong());
        if (found != m_blocks.end()) {
            return &found->second;
        }
        // 空位必须返回**空气状态**而不是 nullptr：规则处理器把 nullptr 当作「读不到
        // 方块」并直接判定位置谓词失败，导致所有带位置谓词的替换规则在空世界里静默
        // 失效（原版空中位置读到的是 Blocks.AIR.defaultBlockState()）。
        return _terrainStateAt(y);
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        // 按值保存：结构代码可以把「默认状态 + with(...)」的 BlockState 放在局部变量里
        // 传进来（生产世界在 setBlockState 中立即拷贝进区块，没有生命周期问题），
        // 记录指针则会在 generate 返回后悬空。nullptr 表示清空，按空气记录。
        m_blocks.insert_or_assign(
            BlockPos(x, y, z).asLong(), state != nullptr ? *state : VanillaBlocks::AIR->defaultState());
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        // 与 oracle 的 FakeLevel 一致：流体状态由当前读到的方块推导。这样多个构件
        // 叠放时，后放的构件能读到前一个构件经数据标记写入的水，从而正确补上水浸。
        return getBlockState(x, y, z)->getFluidState();
    }

    [[nodiscard]] i32 getMinBuildHeight() const override
    {
        return m_minBuildHeight;
    }

    /// 世界种子：`CappedStructureProcessor` 等按「世界种子 + 模板原点」派生位置随机源，
    /// 必须与 oracle 的 `FakeLevel.getSeed()` 一致，否则替换落点整体漂移。
    [[nodiscard]] u64 seed() const override
    {
        return m_seed;
    }

    /// 返回最高方块 Y（Cubium 语义）。无地形即没有方块，返回 minY - 1；
    /// 原版 oracle 的空世界桩返回首个可放置 Y（minY），两者正好相差这一格。
    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        if (!m_terrain.enabled) {
            return m_minBuildHeight - 1;
        }
        return m_terrain.fluid != nullptr ? m_terrain.waterTopY : m_terrain.surfaceY;
    }

    /**
     * @brief 按高度图类型返回最高方块 Y。
     *
     * 这是 Cubium IWorld 的契约（返回方块本身的 Y，不是原版 LevelReader 的「首个可放置
     * Y」）。oracle 侧对应返回原版语义（+1），两侧的差值正是这条能力要暴露的差异。
     */
    [[nodiscard]] i32 getHeight(i32, i32, world::chunk::HeightmapType type) const override
    {
        if (!m_terrain.enabled) {
            return m_minBuildHeight - 1;
        }
        if (type == world::chunk::HeightmapType::OceanFloor || type == world::chunk::HeightmapType::OceanFloorWG) {
            return m_terrain.surfaceY;
        }
        return m_terrain.fluid != nullptr ? m_terrain.waterTopY : m_terrain.surfaceY;
    }

    [[nodiscard]] const std::unordered_map<i64, BlockState>& blocks() const
    {
        return m_blocks;
    }

    /**
     * @brief 模板数据标记生成的实体（结构放置的实体产物）。
     *
     * 与 oracle 一致，只保留**类型与位置**：模板 NBT 里的 UUID 每侧随机，生物初始化
     * 也算不上结构放置的产物；偏航角对部分生物（潜影贝）由实例随机源初始化，原版两次
     * 运行之间都不一样，不能作为确定性比较项。坐标以千分之一为单位取整输出，避免
     * f32/f64 的十进制表示差异被当成真实差异。
     */
    struct SpawnedEntity {
        std::string type;
        i32 xMilli;
        i32 yMilli;
        i32 zMilli;
    };

    [[nodiscard]] const std::vector<SpawnedEntity>& spawnedEntities() const
    {
        return m_entities;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return 0;
        }
        // 携带乘客的实体（坐骑）来自生物初始化（如溺尸的僵尸鹦鹉螺），不是数据标记直接
        // 产生的结构产物；它们依赖世界随机与生物初始化，与结构放置无关，不参与比较。
        if (entity->hasPassengers()) {
            return 0;
        }
        const entity::EntityType* type = entity->entityType();
        const Vector3 pos = entity->position();
        m_entities.push_back(SpawnedEntity{type != nullptr ? type->name() : std::string(),
            static_cast<i32>(std::lround(static_cast<f64>(pos.x) * 1000.0)),
            static_cast<i32>(std::lround(static_cast<f64>(pos.y) * 1000.0)),
            static_cast<i32>(std::lround(static_cast<f64>(pos.z) * 1000.0))});
        return static_cast<EntityInstanceId>(m_entities.size());
    }

private:
    [[nodiscard]] const BlockState* _terrainStateAt(i32 y) const
    {
        if (!m_terrain.enabled) {
            return &VanillaBlocks::AIR->defaultState();
        }
        if (y <= m_terrain.surfaceY) {
            return m_terrain.solid;
        }
        if (m_terrain.fluid != nullptr && y <= m_terrain.waterTopY) {
            return m_terrain.fluid;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    i32 m_minBuildHeight;
    TerrainConfig m_terrain;
    u64 m_seed;
    std::unordered_map<i64, BlockState> m_blocks;
    std::vector<SpawnedEntity> m_entities;
};

const Structure& findStructure(const ResourceLocation& id)
{
    const Structure* structure = StructureRegistry::get(id);
    MC_ASSERT_RELEASE_MSG(structure != nullptr, "structure_blocks_single: unknown structure");
    return *structure;
}

/// 状态的规范字符串：`minecraft:stone` 或 `minecraft:oak_log[axis=x]`（属性按名字排序）。
std::string stateToString(const BlockState& state)
{
    std::string out = state.getBlock().blockLocation().toString();
    std::vector<std::pair<std::string, std::string>> pairs;
    for (const auto& entry : state.values()) {
        pairs.emplace_back(entry.property->name(), entry.property->valueToString(entry.valueIndex));
    }
    if (pairs.empty()) {
        return out;
    }
    std::sort(pairs.begin(), pairs.end());
    out.push_back('[');
    for (std::size_t i = 0; i < pairs.size(); ++i) {
        if (i != 0) {
            out.push_back(',');
        }
        out += pairs[i].first;
        out.push_back('=');
        out += pairs[i].second;
    }
    out.push_back(']');
    return out;
}

std::string runLine(std::string_view inputLine)
{
    initializeStructureRegistries();

    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const std::string dimensionName = input.at("dimension").get<std::string>();
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const i32 chunkX = input.at("chunkX").get<i32>();
    const i32 chunkZ = input.at("chunkZ").get<i32>();
    const ResourceLocation structureId(input.at("structure").get<std::string>());

    WorldSetup setup = makeWorldSetup(dimensionName, seed);
    auto biomeSource = makeBiomeSource(dimensionName, *setup.randomState);
    NoiseChunkGenerator generator(std::move(setup.settings), std::move(biomeSource), setup.randomState);

    // 起点生成：与 structure_start_single 完全同源（每格从同一个初始状态起算）。
    math::WorldgenRandom structureRng(std::make_unique<math::JavaLegacyRandom>(0ULL));
    structureRng.setLargeFeatureSeed(static_cast<i64>(seed), chunkX, chunkZ);

    std::vector<std::unique_ptr<::mc::world::chunk::ChunkPrimer>> regionOwners;
    std::vector<IChunk*> regionChunks;
    for (i32 dz = -1; dz <= 1; ++dz) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            regionOwners.push_back(std::make_unique<::mc::world::chunk::ChunkPrimer>(chunkX + dx, chunkZ + dz));
            regionChunks.push_back(regionOwners.back().get());
        }
    }
    ::mc::WorldGenRegion region(chunkX, chunkZ, 1, std::move(regionChunks));

    const Structure& structure = findStructure(structureId);
    auto start = structure.generate(generator, &region, structureRng, chunkX, chunkZ);
    if (start == nullptr || !start->isValid()) {
        return json{
            {"unit", input.value("unit", "")}, {"valid", false}, {"blocks", json::array()}, {"entities", json::array()}}
            .dump();
    }

    // 起点覆盖的区块范围：原版对每个区块各跑一次 `StructureStart.placeInChunk`。
    const auto& startBox = start->getBoundingBox();
    const i32 minChunkX = startBox.minX() >> world::CHUNK_SHIFT;
    const i32 maxChunkX = startBox.maxX() >> world::CHUNK_SHIFT;
    const i32 minChunkZ = startBox.minZ() >> world::CHUNK_SHIFT;
    const i32 maxChunkZ = startBox.maxZ() >> world::CHUNK_SHIFT;

    RecordingWorld world(
        generator.getMinY(), makeTerrainConfig(dimensionName, input.value("terrain", false)), seed);
    ::mc::world::chunk::ChunkPrimer chunk(chunkX, chunkZ);

    const i32 minY = generator.getMinY();
    const i32 maxY = minY + generator.getGenDepth() - 1;
    const i32 stageOrdinal = static_cast<i32>(structure.decorationStage());

    for (i32 cz = minChunkZ; cz <= maxChunkZ; ++cz) {
        for (i32 cx = minChunkX; cx <= maxChunkX; ++cx) {
            // 原版 `ChunkGenerator.applyBiomeDecoration`：每个区块一份随机源，
            // 装饰种子按该区块最小角派生，特征种子按结构在本阶段的位置派生。
            math::WorldgenRandom rng(std::make_unique<math::Random>(0ULL));
            const u64 decorSeed = rng.setDecorationSeed(seed, cx * world::CHUNK_WIDTH, cz * world::CHUNK_WIDTH);
            rng.setFeatureSeed(decorSeed, 0, stageOrdinal);

            // 原版 `getWritableArea`：y ∈ [minY + 1, maxY]，水平方向整个区块。
            const StructureBoundingBox writable(cx * world::CHUNK_WIDTH,
                minY + 1,
                cz * world::CHUNK_WIDTH,
                cx * world::CHUNK_WIDTH + world::CHUNK_WIDTH - 1,
                maxY,
                cz * world::CHUNK_WIDTH + world::CHUNK_WIDTH - 1);

            for (const auto& piece : start->pieces()) {
                const auto box = piece->getBoundingBox();
                if (!box.intersects(writable)) {
                    continue;
                }
                piece->generate(world, rng, cx, cz, writable, &chunk, &generator);
            }
        }
    }

    // 实体产物模式：只输出模板数据标记生成的实体，与方块产物共用同一套放置流程。
    if (input.value("output", std::string("blocks")) == "entities") {
        auto entities = world.spawnedEntities();
        std::sort(entities.begin(), entities.end(), [](const auto& left, const auto& right) {
            return std::tie(left.type, left.xMilli, left.yMilli, left.zMilli) <
                std::tie(right.type, right.xMilli, right.yMilli, right.zMilli);
        });

        json out = json::array();
        for (const auto& entity : entities) {
            out.push_back(
                json{{"type", entity.type}, {"x", entity.xMilli}, {"y", entity.yMilli}, {"z", entity.zMilli}});
        }
        return json{{"unit", input.value("unit", "")}, {"valid", true}, {"entities", std::move(out)}}.dump();
    }

    std::vector<std::pair<BlockPos, const BlockState*>> sorted;
    sorted.reserve(world.blocks().size());
    for (const auto& [key, state] : world.blocks()) {
        sorted.emplace_back(BlockPos::fromLong(key), &state);
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& left, const auto& right) {
        const auto& a = left.first;
        const auto& b = right.first;
        if (a.x != b.x) {
            return a.x < b.x;
        }
        if (a.y != b.y) {
            return a.y < b.y;
        }
        return a.z < b.z;
    });

    json blocks = json::array();
    for (const auto& [pos, state] : sorted) {
        blocks.push_back(json{pos.x, pos.y, pos.z, stateToString(*state)});
    }

    return json{{"unit", input.value("unit", "")}, {"valid", true}, {"blocks", std::move(blocks)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(structure_blocks_single, nullptr, &runLine)

} // namespace mc::diff
