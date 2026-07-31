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

#include "server/registry/RegistryBootstrap.hpp"

#include "common/advancement/AdvancementLoader.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/entity/damage/tag/DamageTypeTagLoader.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTagLoader.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/special/ArmorDyeRecipe.hpp"
#include "common/item/crafting/special/BookCloningRecipe.hpp"
#include "common/item/crafting/special/DecoratedPotRecipe.hpp"
#include "common/item/crafting/special/MapCloningRecipe.hpp"
#include "common/item/crafting/special/MapExtendingRecipe.hpp"
#include "common/item/crafting/special/RepairItemRecipe.hpp"
#include "common/item/crafting/special/TippedArrowRecipe.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/loot/LootPredicateLoader.hpp"
#include "common/item/loot/LootPredicateManager.hpp"
#include "common/item/loot/LootTableLoader.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/tag/ItemTagLoader.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/backend/java/mappings/JavaBlockStateIdMap.hpp"
#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/sound/jukebox/JukeboxSongs.hpp"
#include "common/world/biome/BiomeLoader.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeTagLoader.hpp"
#include "common/world/biome/JavaBiomeRegistryIdMap.hpp"
#include "common/world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/JavaBlockEntityTypeIdMap.hpp"
#include "common/world/entity/JavaEntityTypeIdMap.hpp"
#include "common/world/gen/carver/ConfiguredCarverLoader.hpp"
#include "common/world/gen/density/DensityFunctionLoader.hpp"
#include "common/world/gen/feature/ConfiguredFeatureLoader.hpp"
#include "common/world/gen/feature/FeatureTypeRegistry.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/jigsaw/ProcessorListLoader.hpp"
#include "common/world/gen/noise/NoiseLoader.hpp"
#include "common/world/gen/placement/PlacedFeatureLoader.hpp"
#include "common/world/gen/placement/PlacementRegistry.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorPresetLoader.hpp"
#include "common/world/gen/settings/NoiseSettingsLoader.hpp"
#include "common/world/gen/settings/WorldPresetLoader.hpp"
#include "common/world/gen/structure/StructureDefinitionLoader.hpp"
#include "common/world/gen/structure/StructureManager.hpp"
#include "common/world/gen/structure/StructureSet.hpp"
#include "common/world/gen/structure/StructureSetLoader.hpp"
#include "common/world/gen/structure/StructureTagLoader.hpp"
#include "common/world/gen/structure/StructureTags.hpp"
#include "common/world/gen/structure/pools/Pools.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"
#include "server/function/FunctionLoader.hpp"
#include "server/function/FunctionManager.hpp"
#include "server/network/EnchantmentNbtBuilder.hpp"
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server {

RegistryBootstrap::RegistryBootstrap(mc::resource::DataPackRepository& dataPackList,
    mc::loot::LootTableManager& lootTableManager,
    mc::loot::LootPredicateManager& predicateManager,
    mc::function::FunctionManager& functionManager)
    : m_dataPackList(dataPackList)
    , m_lootTableManager(lootTableManager)
    , m_predicateManager(predicateManager)
    , m_functionManager(functionManager)
{}

void RegistryBootstrap::registerSpecialRecipes()
{
    using namespace crafting;

    // 注册物品修复配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<RepairItemRecipe>(ResourceLocation("minecraft", "repair_item")));

    // 注册盔甲染色配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<ArmorDyeRecipe>(ResourceLocation("minecraft", "armor_dye")));

    // 注册书复制配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<BookCloningRecipe>(ResourceLocation("minecraft", "book_cloning")));

    // 注册地图复制配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<MapCloningRecipe>(ResourceLocation("minecraft", "map_cloning")));

    // 注册地图扩展配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<MapExtendingRecipe>(ResourceLocation("minecraft", "map_extending")));

    // 注册药水箭配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<TippedArrowRecipe>(ResourceLocation("minecraft", "tipped_arrow")));

    // 注册饰纹陶罐配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<DecoratedPotRecipe>(ResourceLocation("minecraft", "decorated_pot")));

    spdlog::info("Special recipes registered (7 recipes)");
}

void RegistryBootstrap::initializeAll(bool registerEntities)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll");

    // 初始化方块注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Blocks");
        VanillaBlocks::initialize();
    }
    spdlog::info("Vanilla blocks initialized");

    // 初始化物品注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Items");
        Items::initialize();
    }
    spdlog::info("Vanilla items initialized");

    // 初始化唱片机歌曲注册表（必须在 SoundEvents 初始化后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::JukeboxSongs");
        JukeboxSongs::initialize();
    }
    spdlog::info("Jukebox songs initialized");

    // 初始化附魔注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Enchantments");
        item::enchant::EnchantmentRegistry::initialize();
    }
    spdlog::info("Enchantments initialized");

    // 初始化方块物品注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::BlockItems");
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
    spdlog::info("Block items initialized");

    // 初始化物品标签（必须在所有物品注册后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::ItemTags");
        item::tag::ItemTags::initialize();
    }
    spdlog::info("Item tags initialized");

    // 从数据包加载物品标签（追加到或替换内置默认值）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::ItemTagLoader");
        auto dataPackLoadResult = item::tag::ItemTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load item tags from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} item tags from data packs", dataPackLoadResult.value());
        }
    }

    // 注册 enchantment 内联 NBT 构建所需的 datapack 源。须在 ItemTags 加载之后（enchantment
    // 的 supported_items/primary_items 展平依赖 ItemTags::getTag），握手阶段才会懒构建。
    mc::server::net::setEnchantmentDatapackSource(m_dataPackList);

    // 初始化发射器行为注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::DispenseBehaviors");
        blocks::DispenseItemBehaviorRegistry::instance().initDefaultBehaviors();
    }
    spdlog::info("Dispense item behaviors initialized");

    // 初始化战利品表管理器（从数据包加载）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::LootTables");
        loot::LootTableLoader lootLoader(m_lootTableManager);
        auto dataPackLoadResult = lootLoader.loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load loot tables from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            const auto& result = dataPackLoadResult.value();
            spdlog::info("Loaded {} loot tables from data packs ({} failed)", result.successCount, result.failedCount);
            for (const auto& err : result.errors) {
                spdlog::error("Loot table error: {}", err);
            }
        }
    }

    // 初始化战利品谓词管理器（从数据包加载）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Predicates");
        loot::LootPredicateLoader predicateLoader(m_predicateManager);
        auto dataPackLoadResult = predicateLoader.loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load predicates from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            const auto& result = dataPackLoadResult.value();
            spdlog::info("Loaded {} predicates from data packs ({} failed)", result.successCount, result.failedCount);
            for (const auto& err : result.errors) {
                spdlog::error("Predicate error: {}", err);
            }
        }
        // 将谓词管理器关联到掉落表管理器，使 LootContext 可通过掉落表管理器解析命名谓词
        m_lootTableManager.setPredicateManager(&m_predicateManager);
    }

    // 加载配方（从数据包加载）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Recipes");
        RecipeLoader recipeLoader;
        auto dataPackLoadResult = recipeLoader.loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load crafting recipes from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} crafting recipes from data packs ({} failed)",
                dataPackLoadResult.value().successCount,
                dataPackLoadResult.value().failedCount);
        }

        // 注册特殊配方（动态配方，不从数据包加载）
        registerSpecialRecipes();
    }

    // 加载函数（从数据包加载 .mcfunction 文件）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Functions");
        function::FunctionLoader functionLoader(m_functionManager);
        auto funcLoadResult = functionLoader.loadFromDataPackRepository(m_dataPackList);
        if (funcLoadResult.failed()) {
            spdlog::error("Failed to load functions from data packs: {}", funcLoadResult.error().toString());
        } else {
            const auto& result = funcLoadResult.value();
            spdlog::info("Loaded {} functions from data packs ({} failed, {} macros skipped)",
                result.successCount,
                result.failedCount,
                result.skippedCount);
            for (const auto& err : result.errors) {
                spdlog::error("Function error: {}", err);
            }
        }
        m_functionManager.notifyReload();
    }

    // 加载进度（从数据包加载）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Advancements");
        mc::advancement::AdvancementLoader advancementLoader;
        auto advancementLoadResult = advancementLoader.loadFromDataPackRepository(m_dataPackList);
        if (advancementLoadResult.failed()) {
            spdlog::error("Failed to load advancements from data packs: {}", advancementLoadResult.error().toString());
        } else {
            const auto& result = advancementLoadResult.value();
            spdlog::info("Loaded {} advancements from data packs ({} failed)", result.successCount, result.failedCount);
            for (const auto& err : result.errors) {
                spdlog::error("Advancement error: {}", err);
            }
        }
    }

    // 从数据包加载噪声参数（worldgen/noise/*.json）
    // 最底层依赖：density_function / noise_settings 的噪声叶子节点引用噪声名，
    // 故必须先于一切 worldgen Loader 加载。Noises::initialize() 硬编码兜底由
    // NoiseLoader clear() 清空后注入，markLoadedFromDatapack(true) 使 get()/has()
    // 跳过兜底。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Noises");
        auto noiseResult = world::gen::noise::NoiseLoader::loadFromDataPackRepository(m_dataPackList);
        if (noiseResult.failed()) {
            spdlog::error("Failed to load noise parameters from data packs: {}", noiseResult.error().toString());
        } else {
            spdlog::info("Loaded {} noise parameters from data packs", noiseResult.value());
        }
    }

    // 从数据包加载密度函数（worldgen/density_function/*.json）
    // 依赖噪声（noise 叶子节点引用噪声名）。35 个 density_function 经两阶段 Holder
    // 引用解析（前向引用 + 共享子图 + 循环检测）注册到 DensityFunctionRegistry。
    // 噪声叶子节点解析期存 UnboundNoiseLeaf 占位，由 NoiseBindingVisitor 在
    // RandomState 组装 NoiseRouter 时按 name-hash 绑定真实 NormalNoise（阶段3 noise_settings）。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::DensityFunctions");
        auto dfResult = world::gen::density::DensityFunctionLoader::loadFromDataPackRepository(m_dataPackList);
        if (dfResult.failed()) {
            spdlog::error("Failed to load density functions from data packs: {}", dfResult.error().toString());
        } else {
            spdlog::info("Loaded {} density functions from data packs", dfResult.value());
        }
    }

    // 从数据包加载 noise_settings（worldgen/noise_settings/*.json）
    // 依赖 density_function（noise_router 15 字段是 DF Holder，字符串引用查 DensityFunctionRegistry）。
    // DimensionSettings::fromJson 解析 noise 4 尺寸 + 15 DF 路由模板（m_routerDfs，噪声叶子为
    // UnboundNoiseLeaf 占位）+ surface_rule + spawn_target + 标量字段，注册到 NoiseSettingsRegistry。
    // RandomState::create 据此走数据驱动唯一路径，经 NoiseBindingVisitor 绑定真实 NormalNoise。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::NoiseSettings");
        auto nsResult = world::gen::settings::NoiseSettingsLoader::loadFromDataPackRepository(m_dataPackList);
        if (nsResult.failed()) {
            spdlog::error("Failed to load noise_settings from data packs: {}", nsResult.error().toString());
        } else {
            spdlog::info("Loaded {} noise_settings from data packs", nsResult.value());
        }
    }

    // 加载模板池（先注册硬编码基础池，再从数据包加载 JSON 模板池）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::TemplatePools");
        world::gen::structure::pools::Pools::initialize();
        size_t poolCount = world::gen::structure::StructureRegistry::loadTemplatePoolsFromDataPacks(m_dataPackList);
        spdlog::info("Loaded {} template pools from data packs", poolCount);
    }

    // 数据驱动加载结构定义（worldgen/structure/*.json）
    // 顺序：模板池 → 结构定义（jigsaw 结构引用模板池）→ … → 结构标签（依赖结构已注册）。
    // 先 clear() 重置硬编码 initialize() 兜底写入的状态，再由 Loader 按 type 工厂构造并注册。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Structures");
        world::gen::structure::StructureRegistry::clear();
        auto structureResult =
            world::gen::structure::StructureDefinitionLoader::loadFromDataPackRepository(m_dataPackList);
        if (structureResult.failed()) {
            spdlog::error("Failed to load structures from data packs: {}", structureResult.error().toString());
        } else {
            spdlog::info("Loaded {} structures from data packs", structureResult.value());
        }
        // 数据驱动注册完成后置位，使区块生成器兜底守卫不再触发硬编码注册。
        world::gen::structure::StructureRegistry::markInitialized();
    }

    // 加载处理器列表（从数据包加载，补充硬编码注册未覆盖的列表）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::ProcessorLists");
        auto processorResult = world::gen::jigsaw::ProcessorListLoader::loadFromDataPackRepository(m_dataPackList);
        if (processorResult.success()) {
            spdlog::info("Loaded {} processor lists from data packs", processorResult.value());
        } else {
            spdlog::warn("Failed to load processor lists from data packs: {}", processorResult.error().message());
        }
    }

    // 设置 JigsawAssembler 的 TemplateManager 数据包列表（用于加载结构模板 .nbt 文件）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::JigsawTemplateManager");
        world::gen::jigsaw::JigsawAssembler::getTemplateManager().setDataPackRepository(&m_dataPackList);
        spdlog::info("Jigsaw TemplateManager configured with data pack list");
    }

    // ============================================================================
    // 数据驱动世界生成管线
    // ============================================================================
    // 顺序：放置器类型 → 特征类型 → configured_feature → placed_feature →
    //       configured_carver → biome（biome 必须最后，引用上述注册表）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::PlacementRegistry");
        PlacementRegistry::instance().initialize();
    }
    spdlog::info("Placement registry initialized");

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::FeatureTypeRegistry");
        world::gen::feature::initializeBuiltinFeatureTypes();
    }
    spdlog::info("Builtin feature types initialized");

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::ConfiguredFeatures");
        auto featureResult = world::gen::feature::ConfiguredFeatureLoader::loadFromDataPackRepository(m_dataPackList);
        if (featureResult.failed()) {
            spdlog::error("Failed to load configured features from data packs: {}", featureResult.error().toString());
        } else {
            spdlog::info("Loaded {} configured features from data packs", featureResult.value());
        }
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::PlacedFeatures");
        auto placedResult = world::gen::placement::PlacedFeatureLoader::loadFromDataPackRepository(m_dataPackList);
        if (placedResult.failed()) {
            spdlog::error("Failed to load placed features from data packs: {}", placedResult.error().toString());
        } else {
            spdlog::info("Loaded {} placed features from data packs", placedResult.value());
        }
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::ConfiguredCarvers");
        auto carverResult = world::gen::carver::ConfiguredCarverLoader::loadFromDataPackRepository(m_dataPackList);
        if (carverResult.failed()) {
            spdlog::error("Failed to load configured carvers from data packs: {}", carverResult.error().toString());
        } else {
            spdlog::info("Loaded {} configured carvers from data packs", carverResult.value());
        }
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Biomes");
        // 确保 BiomeFactory 构造的默认 Biome 已注册（BiomeLoader 在其上叠加 JSON 字段）
        BiomeRegistry::instance().initialize();
        auto biomeResult = world::biome::BiomeLoader::loadFromDataPackRepository(m_dataPackList);
        if (biomeResult.failed()) {
            spdlog::error("Failed to load biomes from data packs: {}", biomeResult.error().toString());
        } else {
            spdlog::info("Loaded {} biomes from data packs", biomeResult.value());
        }
    }

    // 从数据包加载生物群系标签（须在 Biome 注册后，结构/结构集合引用标签前）
    // 填充 stronghold_biased_to、has_structure/* 等标签的 BiomeId 集合。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::BiomeTags");
        auto biomeTagResult = world::biome::BiomeTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (biomeTagResult.failed()) {
            spdlog::error("Failed to load biome tags from data packs: {}", biomeTagResult.error().toString());
        } else {
            spdlog::info("Loaded {} biome tags from data packs", biomeTagResult.value());
        }
    }

    // 从数据包加载 flat_level_generator_preset（须在方块/biome 注册后：layers 的 block RL 经
    // BlockRegistry 取默认 BlockState，biome RL 经 BiomeLoader::biomeIdByName 映射 BiomeId）。
    // FlatLevelGeneratorPresetLoader 解析 9 个预设 JSON，注册到 FlatLevelGeneratorPresetRegistry，
    // 供 ServerDimensionManager flat 分支查表构造 FlatChunkGenerator。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::FlatPresets");
        auto flatResult =
            world::gen::settings::FlatLevelGeneratorPresetLoader::loadFromDataPackRepository(m_dataPackList);
        if (flatResult.failed()) {
            spdlog::error(
                "Failed to load flat_level_generator_presets from data packs: {}", flatResult.error().toString());
        } else {
            spdlog::info("Loaded {} flat_level_generator_presets from data packs", flatResult.value());
        }
    }

    // 从数据包加载 world_preset（须在 noise_settings + flat_preset 之后：flat 维度的内联 settings
    // 复用 FlatLevelGeneratorSettings::fromSettingsObject 依赖 BlockRegistry/BiomeLoader；noise 维度
    // 仅存 noise_settings RL，装配期由 RandomState::create 查 NoiseSettingsRegistry）。
    // WorldPresetLoader 解析 6 个预设 JSON，注册到 WorldPresetRegistry，
    // 供 ServerDimensionManager 三维度装配查表。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::WorldPresets");
        auto presetResult = world::gen::settings::WorldPresetLoader::loadFromDataPackRepository(m_dataPackList);
        if (presetResult.failed()) {
            spdlog::error("Failed to load world_presets from data packs: {}", presetResult.error().toString());
        } else {
            spdlog::info("Loaded {} world_presets from data packs", presetResult.value());
        }
    }

    // 初始化结构标签（必须在结构集合注册后，海豚寻宝等玩法依赖此标签）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::StructureTags");
        world::gen::structure::StructureTags::initialize();
    }
    spdlog::info("Structure tags initialized");

    // 从数据包加载结构标签（追加到或替换内置默认值）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::StructureTagLoader");
        auto dataPackLoadResult = world::gen::structure::StructureTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load structure tags from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} structure tags from data packs", dataPackLoadResult.value());
        }
    }

    // 数据驱动加载结构集合（worldgen/structure_set/*.json）
    // 顺序：结构定义（已注册）→ 生物群系标签（stronghold_biased_to 已填充）→ 结构集合
    // （要塞集合的 preferred_biomes 依赖生物群系标签）。先 clear() 重置硬编码兜底状态，
    // 再由 Loader 按 placement 类型构造并注册，最后 markInitialized() 置位。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::StructureSets");
        world::gen::structure::StructureSetRegistry::instance().clear();
        auto setResult = world::gen::structure::StructureSetLoader::loadFromDataPackRepository(m_dataPackList);
        if (setResult.failed()) {
            spdlog::error("Failed to load structure sets from data packs: {}", setResult.error().toString());
        } else {
            spdlog::info("Loaded {} structure sets from data packs", setResult.value());
        }
        world::gen::structure::StructureSetRegistry::instance().markInitialized();
    }

    // 注册实体类型（可选）
    if (registerEntities) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Entities");
        entity::VanillaEntities::registerAll();
    }

    // 初始化实体类型标签（必须在所有实体类型注册后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::EntityTypeTags");
        EntityTypeTags::initialize();
    }
    spdlog::info("Entity type tags initialized");

    // 从数据包加载实体类型标签（追加到或替换内置默认值）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::EntityTypeTagLoader");
        auto dataPackLoadResult = EntityTypeTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load entity type tags from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} entity type tags from data packs", dataPackLoadResult.value());
        }
    }

    // 初始化伤害类型标签（用于狼铠吸收判定、伤害分类等）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::DamageTypeTags");
        DamageTypeTags::initialize();
    }
    spdlog::info("Damage type tags initialized");

    // 从数据包加载伤害类型标签（追加到或替换内置默认值）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::DamageTypeTagLoader");
        auto dataPackLoadResult = DamageTypeTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load damage type tags from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} damage type tags from data packs", dataPackLoadResult.value());
        }
    }

    // 初始化预定义日程（村民AI行为日程）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::Schedules");
        entity::ai::brain::schedule::Schedule::initialize();
    }
    spdlog::info("Schedules initialized");

    // 初始化记忆模块类型
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::MemoryModules");
        entity::ai::brain::memory::MemoryModuleTypes::initialize();
    }
    spdlog::info("Memory module types initialized");

    // 初始化村民交易配方表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::VillagerTrades");
        world::village::trade::VillagerTrades::initialize();
    }

    // 初始化 Java id 映射表（level_chunk_with_light vanilla wire 用）。
    // 顺序依赖：block 表遍历 Block::forEachBlockState（须在 VanillaBlocks::initialize 之后）；
    // biome 表遍历 BiomeRegistry::allBiomes（须在 BiomeRegistry::initialize 之后）；
    // blockentity 表仅依赖 blockEntityTypeToId 静态映射，无注册顺序依赖。
    // item 表遍历 ItemRegistry::forEachItem（须在 Items::initialize 之后）。四者均在上方已完成。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "RegistryBootstrap::initializeAll::JavaIdMaps");
        if (auto r = ::mc::network::backend::java::JavaBlockStateIdMap::instance().initialize(); r.failed()) {
            spdlog::error("Failed to initialize JavaBlockStateIdMap: {}", r.error().toString());
        }
        if (auto r = world::biome::JavaBiomeRegistryIdMap::instance().initialize(); r.failed()) {
            spdlog::error("Failed to initialize JavaBiomeRegistryIdMap: {}", r.error().toString());
        }
        if (auto r = JavaBlockEntityTypeIdMap::instance().initialize(); r.failed()) {
            spdlog::error("Failed to initialize JavaBlockEntityTypeIdMap: {}", r.error().toString());
        }
        if (auto r = JavaEntityTypeIdMap::instance().initialize(); r.failed()) {
            spdlog::error("Failed to initialize JavaEntityTypeIdMap: {}", r.error().toString());
        }
        if (auto r = ::mc::network::backend::java::JavaItemIdMap::instance().initialize(); r.failed()) {
            spdlog::error("Failed to initialize JavaItemIdMap: {}", r.error().toString());
        }
    }
}

} // namespace mc::server
