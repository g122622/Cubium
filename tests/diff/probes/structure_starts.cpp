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
 * @file structure_starts.cpp
 * @brief 差分能力：结构起点（STRUCTURE_STARTS 阶段的产物）。
 *
 * 输入一行一个样本：维度名、世界种子、区块坐标。输出该区块里落下的全部结构起点，
 * 每个起点给出被选中的结构、整体包围盒、各个片段（顺序即装配顺序）与地面高度偏移、
 * 以及每个片段拥有的连接点。
 *
 * 这一层不写任何方块、不依赖邻区：STRUCTURE_STARTS 阶段只读本区块坐标与结构集，
 * 因此逐区块独立测量即可，与区块的处理顺序无关。
 *
 * 单元由共同语料决定（`harnesses/structure_starts.ts`），本探针不提供 `list`。
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "common/core/GameDirectory.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "probe.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/BiomeLoader.hpp"
#include "server/world/gen/biome/source/EndBiomeSource.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/carver/ConfiguredCarverLoader.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/feature/ConfiguredFeatureLoader.hpp"
#include "server/world/gen/feature/FeatureTypeRegistry.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/feature/template/TemplateManagerHostBinding.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/jigsaw/ProcessorListLoader.hpp"
#include "server/world/gen/placement/PlacedFeatureLoader.hpp"
#include "server/world/gen/placement/PlacementRegistry.hpp"
#include "server/world/gen/density/DensityFunctionLoader.hpp"
#include "server/world/gen/noise/NoiseLoader.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/gen/settings/NoiseSettingsLoader.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructureDefinitionLoader.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "server/world/gen/structure/StructurePlacementState.hpp"
#include "server/world/gen/structure/StructureSetLoader.hpp"
#include "server/world/gen/structure/StructureTagLoader.hpp"
#include "server/world/gen/structure/pools/Pools.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

// ============================================================================
// 数据包装配（进程内只做一次）
// ============================================================================

std::filesystem::path dataPackRoot()
{
    const char* path = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (path == nullptr || *path == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    return std::filesystem::path(path);
}

/**
 * @brief 数据包仓库（模板来源）
 *
 * 模板管理器只持非拥有指针，仓库必须活到最后一个模板被使用，因此放在进程级静态里，
 * 不能是局部变量。
 */
resource::DataPackRepository& dataPackRepository()
{
    static const auto repository = [] {
        auto repo = std::make_unique<resource::DataPackRepository>();
        // 仓库扫描的是「包含若干数据包的目录」，而环境变量给的是**数据包本身**的根
        // （含 pack.mcmeta 与 data/ 的那一层），因此扫它的上一级。
        const auto scanResult = repo->scanDirectory(dataPackRoot().parent_path());
        if (!scanResult.success() || scanResult.value() == 0) {
            throw std::runtime_error("structure_starts: 数据包扫描失败：" + dataPackRoot().string());
        }
        // 模板是按路径逐个读取的非 JSON 资源：这里先抽查一个必定存在的模板，把「仓库能扫到
        // 数据包」与「仓库能读到包内资源」两件事分开验证——否则一处路径写错会让全部拼图
        // 结构静默退化成空模板（连接点为空、尺寸为 0），差分只会表现为构件数量对不上。
        const auto probeTemplate = repo->readResource("minecraft/structure/village/plains/town_centers/plains_meeting_point_1.nbt");
        if (!probeTemplate.success() || probeTemplate.value().empty()) {
            throw std::runtime_error("structure_starts: 数据包里读不到结构模板，仓库路径可能配错：" +
                dataPackRoot().string());
        }
        return repo;
    }();
    return *repository;
}

/**
 * @brief 模板管理器绑定令牌（进程内保持存活）
 *
 * 拼图结构的模板在**模板池加载期**就被读进片段对象，因此绑定必须早于结构定义加载；
 * 未绑定时模板静默失败（连接点为空、尺寸为 0），装配退化成「起始点上一个退化片段」，
 * 差分得到的将全是假差异。
 */
world::gen::feature::template_::TemplateManagerHostBinding& templateBinding()
{
    static const auto binding = [] {
        auto token = std::make_unique<world::gen::feature::template_::TemplateManagerHostBinding>(
            world::gen::jigsaw::JigsawAssembler::getTemplateManager());
        token->bindDataPackRepository(dataPackRepository());
        return token;
    }();
    return *binding;
}

/**
 * @brief 按服务端启动顺序装配全部世界生成注册表
 *
 * 顺序与 `RegistryBootstrap::initializeAll` 一致：模板池必须先于结构定义，结构标签必须先于
 * 结构集。注册表是进程级单例，重复调用幂等。
 */
void initializeRegistries()
{
    static const bool initialized = [] {
        (void)templateBinding();

        VanillaBlocks::initialize();
        BlockTags::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();

        auto& repo = dataPackRepository();
        PlacementRegistry::instance().initialize();
        world::gen::feature::initializeBuiltinFeatureTypes();
        world::gen::feature::ConfiguredFeatureLoader::loadFromDataPackRepository(repo);
        world::gen::placement::PlacedFeatureLoader::loadFromDataPackRepository(repo);
        world::gen::carver::ConfiguredCarverLoader::loadFromDataPackRepository(repo);
        world::biome::BiomeLoader::loadFromDataPackRepository(repo);

        // 噪声 → 密度函数 → noise_settings：生成器的 RandomState 只走数据驱动路径，
        // 三段缺一不可（noise_settings 里的噪声叶子按名引用前两者）。
        world::gen::noise::NoiseLoader::loadFromDataPackRepository(repo);
        world::gen::density::DensityFunctionLoader::loadFromDataPackRepository(repo);
        world::gen::settings::NoiseSettingsLoader::loadFromDataPackRepository(repo);

        world::gen::structure::pools::Pools::initialize();
        world::gen::structure::StructureRegistry::loadTemplatePoolsFromDataPacks(repo);
        world::gen::structure::StructureRegistry::clear();
        if (!world::gen::structure::StructureDefinitionLoader::loadFromDataPackRepository(repo).success()) {
            throw std::runtime_error("structure_starts: 结构定义加载失败");
        }
        world::gen::jigsaw::ProcessorListLoader::loadFromDataPackRepository(repo);
        if (!world::gen::structure::StructureTagLoader::loadFromDataPackRepository(repo).success()) {
            throw std::runtime_error("structure_starts: 结构标签加载失败");
        }
        world::gen::structure::StructureSetRegistry::instance().clear();
        if (!world::gen::structure::StructureSetLoader::loadFromDataPackRepository(repo).success()) {
            throw std::runtime_error("structure_starts: 结构集加载失败");
        }
        world::gen::structure::StructureSetRegistry::instance().validateAgainstStructureRegistry();
        return true;
    }();
    (void)initialized;
}

// ============================================================================
// 维度
// ============================================================================


/**
 * @brief 按维度名构造维度设置与随机状态
 *
 * 群系源必须与生成器共享**同一个** RandomState 实例：群系源内部持有该实例的裸指针
 * （采样器指针 + 末地群系的随机状态指针），两份独立实例会让群系采样读到已释放的内存。
 */
struct WorldSetup {
    DimensionSettings settings;
    std::shared_ptr<world::gen::RandomState> randomState;
};

WorldSetup makeWorldSetup(const std::string& name, u64 seed)
{
    if (name == "overworld") {
        DimensionSettings settings = DimensionSettings::overworld();
        auto resolved = world::gen::RandomState::create(settings, seed);
        return {std::move(settings), std::shared_ptr<world::gen::RandomState>(std::move(resolved))};
    }
    if (name == "the_end") {
        DimensionSettings settings = DimensionSettings::end();
        auto resolved = world::gen::RandomState::create(settings, seed);
        return {std::move(settings), std::shared_ptr<world::gen::RandomState>(std::move(resolved))};
    }
    throw std::runtime_error("structure_starts: unknown dimension " + name);
}

/** 用共享随机状态构造该维度的群系源。 */
std::unique_ptr<world::biome::IBiomeSource> makeBiomeSource(
    const std::string& name, const world::gen::RandomState& randomState)
{
    if (name == "overworld") {
        return world::biome::source::MultiNoiseBiomeSource::createOverworld(randomState, false, false);
    }
    if (name == "the_end") {
        return std::make_unique<world::biome::source::EndBiomeSource>(randomState);
    }
    throw std::runtime_error("structure_starts: unknown dimension " + name);
}

// ============================================================================
// 摘要
// ============================================================================

const char* rotationName(Rotation rotation)
{
    switch (rotation) {
        case Rotation::None:
            return "NONE";
        case Rotation::Clockwise90:
            return "CLOCKWISE_90";
        case Rotation::Clockwise180:
            return "CLOCKWISE_180";
        case Rotation::CounterClockwise90:
            return "COUNTERCLOCKWISE_90";
    }
    return "NONE";
}

const char* projectionName(StructurePieceProjection projection)
{
    return projection == StructurePieceProjection::Rigid ? "rigid" : "terrain_matching";
}

/// 一个片段的摘要。字段与 oracle 声明的一一对应，多出的字段不参与比较。
json summarisePiece(const world::gen::structure::StructurePiece& piece)
{
    const auto box = piece.getBoundingBox();
    json junctions = json::array();
    for (const auto& junction : piece.getJunctions()) {
        junctions.push_back(json{{"x", junction.getSourceX()},
            {"groundY", junction.getSourceGroundY()},
            {"z", junction.getSourceZ()},
            {"deltaY", junction.getDeltaY()},
            {"dest",
                    junction.getDestProjection() == world::gen::jigsaw::JigsawPlacementBehaviour::Rigid ? "rigid"
                                                                                                               : "terrain_matching"}});
    }
    return json{{"box", {box.minX(), box.minY(), box.minZ(), box.maxX(), box.maxY(), box.maxZ()}},
        {"rotation", rotationName(piece.getRotation())},
        {"groundLevelDelta", piece.getGroundLevelDelta()},
        {"template", std::string(piece.templateLocation())},
        {"projection", projectionName(piece.getProjection())},
        {"junctions", std::move(junctions)}};
}

// ============================================================================
// 单行
// ============================================================================

std::string runLine(std::string_view inputLine)
{
    initializeRegistries();

    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const std::string dimensionName = input.at("dimension").get<std::string>();
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const i32 chunkX = input.at("chunkX").get<i32>();
    const i32 chunkZ = input.at("chunkZ").get<i32>();

    // 群系源与生成器必须共享同一个 RandomState：气候采样器按值持有 6 个气候密度函数的
    // 裸指针，末地群系源还持有随机状态本身的裸指针；各自构造一份会让指针指向已释放对象。
    WorldSetup setup = makeWorldSetup(dimensionName, seed);
    auto biomeSource = makeBiomeSource(dimensionName, *setup.randomState);
    NoiseChunkGenerator generator(std::move(setup.settings), std::move(biomeSource), setup.randomState);

    // 放置状态要看到本维度的群系源，生成器自己持有同一份：这里从生成器重建一份用于判定的
    // 状态即可（判定只读群系源与结构集），二者必须是同一套群系。
    world::gen::structure::StructurePlacementState placementState(
        *generator.getBiomeSource(), static_cast<i64>(seed));
    auto& registry = world::gen::structure::StructureSetRegistry::instance();
    for (const auto& structureSetPtr : registry.getAll()) {
        if (structureSetPtr) {
            placementState.addStructureSet(*structureSetPtr);
        }
    }

    json starts = json::array();
    for (const auto* structureSet : placementState.structureSets()) {
        NoiseChunkGenerator::StructureSelection selection =
            generator._generateStructureStartsForSet(placementState, *structureSet, chunkX, chunkZ);
        if (selection.start == nullptr) {
            continue;
        }
        const auto& start = *selection.start;
        const auto& bbox = start.getBoundingBox();
        json pieces = json::array();
        for (const auto& piece : start.pieces()) {
            pieces.push_back(summarisePiece(*piece));
        }
        starts.push_back(json{{"structure", selection.structureId.toString()},
            {"box", {bbox.minX(), bbox.minY(), bbox.minZ(), bbox.maxX(), bbox.maxY(), bbox.maxZ()}},
            {"pieces", std::move(pieces)}});
    }

    return json{{"unit", input.value("unit", "")}, {"starts", std::move(starts)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("structure_starts", nullptr, &runLine)

} // namespace mc::diff