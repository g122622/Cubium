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

#include "JigsawStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/jigsaw/AssemblyTypes.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/jigsaw/JigsawJunction.hpp"
#include "server/world/gen/jigsaw/JigsawPlacer.hpp"
#include "server/world/gen/jigsaw/JigsawTypes.hpp"
#include "server/world/gen/jigsaw/PoolAliasBinding.hpp"
#include "server/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/valueprovider/HeightProvider.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

using mc::i32;

/// Jigsaw 结构生成所需的最小地形高度
constexpr i32 MIN_TERRAIN_HEIGHT = 20;

/// Jigsaw 结构生成所需的最大地形高度
constexpr i32 MAX_TERRAIN_HEIGHT = 220;

/// 默认起始高度（接近海平面）
constexpr i32 DEFAULT_START_Y = 64;

/**
 * @brief Jigsaw 结构片段适配器
 *
 * 将 PlacedPiece 适配为 StructurePiece，用于存储到 StructureStart。
 * 存储 JigsawJunction 用于地形平滑计算。
 */
class JigsawPlacedPieceAdapter final : public mc::world::gen::structure::StructurePiece {
public:
    explicit JigsawPlacedPieceAdapter(mc::world::gen::jigsaw::PlacedPiece placed) noexcept
        : StructurePiece(90,
              placed.boundingBox.minX(),
              placed.boundingBox.minY(),
              placed.boundingBox.minZ(),
              placed.boundingBox.maxX(),
              placed.boundingBox.maxY(),
              placed.boundingBox.maxZ())
        , m_placed(std::move(placed))
        , m_groundLevelDelta(m_placed.groundLevelDelta)
        , m_junctions(m_placed.junctions)
    {
        // 构件的旋转/镜像必须写回 StructurePiece 基类字段：装配信息只存在于 m_placed 里时，
        // 经 StructurePiece 接口读到的旋转恒为 None、镜像恒为 None，诊断与 parity 校验都会失真。
        setRotation(m_placed.rotation);
        setMirror(m_placed.mirror);
    }

    void generate(mc::IWorldWriter& world,
        mc::math::IRandom& rng,
        mc::i32 chunkX,
        mc::i32 chunkZ,
        const mc::world::gen::structure::StructureBoundingBox& chunkBounds,
        mc::ChunkPrimer* chunk,
        mc::IChunkGenerator* generator) override
    {
        // MC 1.21.11: 方块放置在 FEATURES 阶段通过 placeInChunk → generate 调用
        if (m_placed.piece && !m_placed.piece->isEmpty()) {
            mc::world::gen::jigsaw::JigsawPlacer::placePiece(world, m_placed, rng, &chunkBounds, chunk, generator);
        }
    }

    [[nodiscard]] mc::i32 getGroundLevelDelta() const noexcept override
    {
        return m_groundLevelDelta;
    }

    [[nodiscard]] const std::vector<mc::world::gen::jigsaw::JigsawJunction>& getJunctions() const noexcept override
    {
        return m_junctions;
    }

    [[nodiscard]] bool isJigsawPiece() const noexcept override
    {
        return true;
    }

    /**
     * @brief 片段来源的模板（或地物）标识
     *
     * 仅用于诊断与差分：只比包围盒无法区分"装配选错了模板"与"摆放位置有偏差"。
     * 地物池元素没有模板文件，其标识取被放置的地物 id。
     */
    [[nodiscard]] std::string_view templateLocation() const noexcept override
    {
        if (!m_placed.piece) {
            return {};
        }
        // 列表/地物/空池元素没有模板文件，退回到池元素类型名（与 oracle 侧的
        // `structure_pool_element` 注册名同口径），否则这些构件在差分里只能是空串。
        const std::string& name = m_placed.piece->getName();
        return name.empty() ? std::string_view(m_placed.piece->getTypeName()) : std::string_view(name);
    }

    [[nodiscard]] mc::StructurePieceProjection getProjection() const noexcept override
    {
        return (m_placed.projection == mc::world::gen::jigsaw::JigsawPlacementBehaviour::TerrainMatching)
            ? mc::StructurePieceProjection::TerrainMatching
            : mc::StructurePieceProjection::Rigid;
    }

private:
    mc::world::gen::jigsaw::PlacedPiece m_placed;
    mc::i32 m_groundLevelDelta;
    std::vector<mc::world::gen::jigsaw::JigsawJunction> m_junctions;
};

} // namespace

namespace mc {
namespace world {
namespace gen {
namespace structure {

const std::string JigsawStructure::m_name = "jigsaw";

JigsawStructure::JigsawStructure(ResourceLocation id,
    JigsawConfig config,
    i32 startY,
    bool nearTerrain,
    bool adjustForTerrain,
    TerrainAdaptation terrainAdaptation)
    : Structure(std::move(id))
    , m_config(std::move(config))
    , m_startY(startY)
    , m_nearTerrain(nearTerrain)
    , m_adjustForTerrain(adjustForTerrain)
{
    // 写入基类 m_terrainAdaptation：非 None 时由基类非虚 terrainAdaptation() 直接返回；
    // None 时回退 defaultTerrainAdaptation()（同样返回基类成员，为 None）。
    m_terrainAdaptation = terrainAdaptation;
}

std::unique_ptr<StructureStart> JigsawStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = makeStructureStart(chunkX, chunkZ);

    // 获取起始模板池
    auto& patternRegistry = jigsaw::TemplatePoolRegistry::instance();
    const jigsaw::TemplatePool* startPool = patternRegistry.getPool(m_config.startPool);

    if (!startPool || startPool->isEmpty()) {
        return start;
    }

    // 起始高度**只**来自 start_height 高度提供者（原版 findGenerationPoint 同样只采样它）。
    // 通用拼图结构在地表以下（试炼密室/要塞/下界堡垒等）或高空生成，任何"地形太低就回退到
    // 某个默认 Y"的兜底都会把它们整批搬到地表附近——这是曾经让整类结构在差分里系统性错位的根因。
    i32 startY = m_startY;
    if (m_config.startHeight) {
        valueprovider::WorldGenerationContext context(MIN_BUILD_HEIGHT, CHUNK_HEIGHT);
        startY = m_config.startHeight->sample(rng, context);
    }
    // 候选生成点：区块最小角 + start_height 给出的 Y（原版 JigsawStructure.findGenerationPoint
    // 用 chunkPos.getMinBlockX()/getMinBlockZ()，**不含任何随机偏移**——随机偏移早在结构集
    // 选点时由 RandomSpreadStructurePlacement 消耗完毕）。
    const BlockPos startPos(chunkX * CHUNK_WIDTH, startY, chunkZ * CHUNK_WIDTH);

    // 预解析池别名绑定（试炼密室等结构的池随机化）。
    // 无别名时传 nullptr，JigsawAssembler 使用空查找表（恒等映射）。
    const jigsaw::PoolAliasBindings* aliases = m_config.poolAliases.empty() ? nullptr : &m_config.poolAliases;

    // maxDistanceFromCenter：装配期初始化 freeShape 的可放置空间（缺省时 JigsawAssembler
    // 内部使用 MC 默认值 MaxDistance(80)）。
    const structure::MaxDistance* maxDistance =
        m_config.maxDistanceFromCenter.has_value() ? &(*m_config.maxDistanceFromCenter) : nullptr;

    // ===== 阶段一：选起始块 + 解析起始摆放（对应原版 Structure.findGenerationPoint 起始段）=====
    // 只消耗「起始旋转 / 起始块 / 起始块连接点洗牌」的随机数，不做任何子构件装配。
    auto resolved = jigsaw::JigsawAssembler::resolveStart(*startPool,
        BlockPos(chunkX * CHUNK_WIDTH, 0, chunkZ * CHUNK_WIDTH),
        startY,
        m_config.projectStartToHeightmap,
        rng,
        generator,
        aliases,
        m_config.startJigsawName,
        &m_config.dimensionPadding,
        static_cast<i64>(generator.seed()));
    if (!resolved.has_value()) {
        return start;
    }

    // ===== 生物群系校验（必须在完整装配之前）=====
    // 原版 `Structure.findValidGenerationPoint` = `findGenerationPoint(...).filter(stub -> isValidBiome(stub))`，
    // 而 JigsawPlacement 返回的 GenerationStub 里，**完整递归装配被包在惰性 lambda 中**
    // （`p_352014_ -> { addPieces(...); list.forEach(...); }`）：只有通过群系校验、调用
    // `getPiecesBuilder()` 时才会执行。因此被拒绝的候选只付起始块代价，绝不做整族装配。
    // 这里必须同序——若先装配再校验，多条目结构集（如 5 个村庄变体）会为每个被拒绝的变体
    // 白做一次完整装配（数百次地形高度查询），并且装配消耗的随机数与后续重抽错位。
    if (!isValidBiomeAt(generator, resolved->placement.candidatePoint)) {
        return start;
    }

    // ===== 阶段二：完整装配（对应原版 GenerationStub.getPiecesBuilder()）=====
    std::vector<jigsaw::PlacedPiece> placedPieces = jigsaw::JigsawAssembler::assembleFromStartPlacement(patternRegistry,
        *resolved->startPiece,
        resolved->placement,
        m_config.size,
        m_config.useExpansionHack,
        rng,
        generator,
        resolved->aliasLookup,
        maxDistance,
        &m_config.dimensionPadding);

    // 为每个 PlacedPiece 创建适配器并添加到 StructureStart
    // MC 1.21.11: 结构起点只创建片段，不写入方块
    // 方块放置延迟到 FEATURES 阶段，由 placeInChunk() 调用 generate() 执行
    //
    // 【包围盒必须显式登记】`addPiece` 会把片段并入起点包围盒。原版 `StructurePiecesBuilder`
    // 的包围盒由每个 piece 的包围盒并入，这里等价；但空片段（EmptyJigsawPiece）不登记，
    // 其占位包围盒与原版一致地不计入。
    for (auto& placed : placedPieces) {
        if (placed.piece && !placed.piece->isEmpty()) {
            start->addPiece(std::make_unique<JigsawPlacedPieceAdapter>(std::move(placed)));
        }
    }
    start->recalculateStructureSize();

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
