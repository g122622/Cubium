/**
 * @file structure_probe_common.hpp
 * @brief 结构类差分探针共用的世界生成注册表装配。
 *
 * `structure_starts` 与 `mineshaft_pieces` 都要按服务端启动顺序把世界生成注册表
 * （方块、群系、噪声、密度函数、特征、结构、模板池、结构集）装好才能跑结构生成。
 * 这段顺序如果两处各抄一份，迟早会漂移——而漂移的表现是「某个能力突然多出一堆差异」。
 * 因此抽到这里，谁需要结构生成就 include 它。
 */

#pragma once

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "common/core/Types.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/BiomeLoader.hpp"
#include "server/world/gen/biome/BiomeTagLoader.hpp"
#include "server/world/gen/biome/source/EndBiomeSource.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/carver/ConfiguredCarverLoader.hpp"
#include "server/world/gen/density/DensityFunctionLoader.hpp"
#include "server/world/gen/feature/ConfiguredFeatureLoader.hpp"
#include "server/world/gen/feature/FeatureTypeRegistry.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/feature/template/TemplateManagerHostBinding.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/jigsaw/ProcessorListLoader.hpp"
#include "server/world/gen/noise/NoiseLoader.hpp"
#include "server/world/gen/placement/PlacedFeatureLoader.hpp"
#include "server/world/gen/placement/PlacementRegistry.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/gen/settings/NoiseSettingsLoader.hpp"
#include "server/world/gen/structure/StructureDefinitionLoader.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "server/world/gen/structure/StructureSet.hpp"
#include "server/world/gen/structure/StructureSetLoader.hpp"
#include "server/world/gen/structure/StructureTagLoader.hpp"
#include "server/world/gen/structure/pools/Pools.hpp"

namespace mc::diff {

inline std::filesystem::path dataPackRoot()
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
inline resource::DataPackRepository& dataPackRepository()
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
        const auto probeTemplate =
            repo->readResource("minecraft/structure/village/plains/town_centers/plains_meeting_point_1.nbt");
        if (!probeTemplate.success() || probeTemplate.value().empty()) {
            throw std::runtime_error(
                "structure_starts: 数据包里读不到结构模板，仓库路径可能配错：" + dataPackRoot().string());
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
inline world::gen::feature::template_::TemplateManagerHostBinding& templateBinding()
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

/**
 * @brief 按服务端启动顺序装配全部世界生成注册表
 *
 * 顺序与 `RegistryBootstrap::initializeAll` 一致：模板池必须先于结构定义，结构标签必须先于
 * 结构集。注册表是进程级单例，重复调用幂等。
 */
inline void initializeStructureRegistries()
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
            throw std::runtime_error("structure probe: structure definition loading failed");
        }
        world::gen::jigsaw::ProcessorListLoader::loadFromDataPackRepository(repo);
        if (!world::gen::structure::StructureTagLoader::loadFromDataPackRepository(repo).success()) {
            throw std::runtime_error("structure probe: structure tag loading failed");
        }
        // 生物群系标签必须先于结构集加载：同心环放置的 preferred_biomes 引用了
        // `#minecraft:stronghold_biased_to`，缺了这一步 preferredBiomes 恒为空。
        if (!world::biome::BiomeTagLoader::loadFromDataPackRepository(repo).success()) {
            throw std::runtime_error("structure probe: biome tag loading failed");
        }
        world::gen::structure::StructureSetRegistry::instance().clear();
        if (!world::gen::structure::StructureSetLoader::loadFromDataPackRepository(repo).success()) {
            throw std::runtime_error("structure probe: structure set loading failed");
        }
        world::gen::structure::StructureSetRegistry::instance().validateAgainstStructureRegistry();
        return true;
    }();
    (void)initialized;
}

/** 一个维度的设置与随机状态。 */
struct WorldSetup {
    DimensionSettings settings;
    std::shared_ptr<world::gen::RandomState> randomState;
};

/**
 * @brief 按维度名构造维度设置与随机状态。
 *
 * 群系源必须与生成器共享**同一个** RandomState 实例：群系源内部持有该实例的裸指针
 * （采样器指针 + 末地群系的随机状态指针），两份独立实例会让群系采样读到已释放的内存。
 */
inline WorldSetup makeWorldSetup(const std::string& name, u64 seed)
{
    // 【必须按维度 + 种子缓存】RandomState::create 会重建密度函数树并对 15 个 root 做编译
    // （McToAst → 优化 → 字节码 → JIT），而 JIT 机器码由进程级 JitRuntime 持有**至进程结束、
    // 不释放**。每个单元新建一份，几百个单元就能把提交量推到上 GB，进而触到分配失败而静默
    // 退出（实测跑满 250 行后必定崩溃、单独跑任一行却不会）。原版同样按维度缓存 RandomState，
    // 不随区块重建；这里对齐同一生命周期。
    struct CachedState {
        u64 seed;
        std::shared_ptr<world::gen::RandomState> state;
    };

    static std::unordered_map<std::string, CachedState> cache;

    const auto makeSettings = [&name]() {
        if (name == "overworld") {
            return DimensionSettings::overworld();
        }
        if (name == "the_nether") {
            return DimensionSettings::nether();
        }
        if (name == "the_end") {
            return DimensionSettings::end();
        }
        throw std::runtime_error("structure probe: unknown dimension " + name);
    };

    const auto found = cache.find(name);
    if (found != cache.end() && found->second.seed == seed) {
        return {makeSettings(), found->second.state};
    }

    DimensionSettings settings = makeSettings();
    auto shared = std::shared_ptr<world::gen::RandomState>(world::gen::RandomState::create(settings, seed));
    cache[name] = CachedState{seed, shared};
    return {std::move(settings), std::move(shared)};
}

/** @brief 用共享随机状态构造该维度的群系源。 */
inline std::unique_ptr<world::biome::IBiomeSource> makeBiomeSource(
    const std::string& name, const world::gen::RandomState& randomState)
{
    if (name == "overworld") {
        return world::biome::source::MultiNoiseBiomeSource::createOverworld(randomState, false, false);
    }
    if (name == "the_nether") {
        return world::biome::source::MultiNoiseBiomeSource::createNether(randomState);
    }
    if (name == "the_end") {
        return std::make_unique<world::biome::source::EndBiomeSource>(randomState);
    }
    throw std::runtime_error("structure probe: unknown dimension " + name);
}

} // namespace mc::diff
