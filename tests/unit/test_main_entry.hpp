/**
 * @file test_main_entry.hpp
 * @brief 单元测试进程的入口组件（全局环境与监听器）。
 *
 * 被 tests/unit/main.cpp 直接 include；tests/ci-probes 的启动成本对照入口也 include 本文件，
 * 以复用完全相同的环境与监听器实现。
 *
 * @note 本文件没有包含守卫：它刻意是"被包含的代码片段"而非独立头文件，整段只在一个 TU 里
 *       展开一次即可（两个使用者都各自把它作为唯一的入口来源）。
 */

#include "common/core/GameDirectory.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/loot/LootPredicateManager.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/tag/ItemTagLoader.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/backend/java/mappings/JavaBlockIdMap.hpp"
#include "common/network/backend/java/mappings/JavaBlockStateIdMap.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "server/function/FunctionManager.hpp"
#include "server/registry/RegistryBootstrap.hpp"
#include "server/world/ChunkTaskScheduler.hpp"
#include "server/world/gen/density/DensityFunctionLoader.hpp"
#include "server/world/gen/noise/NoiseLoader.hpp"
#include "server/world/gen/settings/FlatLevelGeneratorPresetLoader.hpp"
#include "server/world/gen/settings/NoiseSettingsLoader.hpp"
#include "server/world/gen/settings/WorldPresetLoader.hpp"

#include <gtest/gtest.h>

#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>

#ifdef __linux__
#include <sys/resource.h>
#endif

/// 全局测试环境：在所有用例运行前一次性加载 noise_settings 数据驱动注册表。
///
/// RandomState::create 现为数据驱动唯一路径（查 NoiseSettingsRegistry），
/// 故任何调用 create() 的测试都需 Noises / DensityFunctionRegistry / NoiseSettingsRegistry
/// 已从原版数据包加载。注册表为进程级单例，加载一次即对全部测试可见。
/// 数据包目录缺失时（非开发机）静默跳过——此类测试会因 registry 为空而断言失败，
/// 属预期（CI/开发机数据包应存在）。
///
/// 放在全局命名空间而非匿名命名空间：tests/ci-probes 的启动成本对照入口 include 本文件后
/// 需要引用它（匿名命名空间内的名字在另一 TU 中不可见）。
class WorldGenRegistryEnvironment : public ::testing::Environment {
public:
    void SetUp() override
    {
        // 注册组件序列化器（进程单例，幂等）。Entity::writeToNBT/readFromNBT 经
        // ComponentSerializerRegistry::saveAll/loadAll 把已 ECS 组件化的字段（Pos/Motion/
        // Rotation/Health/Equipment 等 13+ 字段）序列化到 NBT。未注册时 saveAll/loadAll 遍历空
        // m_entries 静默空转，实体的组件化字段不写不读，NBT 往返测试（装备/位置持久化等）会静默
        // 丢字段。此注册不依赖数据包/方块注册表，放最前无条件执行，确保任何环境下的测试都能
        // 正确序列化组件字段。原先各测试需自行调 VanillaEntities::registerAll() 才间接触发，
        // 漏调即静默失败；集中到全局环境根治此类 setup 陷阱。
        mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();

        // 初始化伤害类型标签（进程单例，s_initialized 守卫幂等，纯硬编码成员集不依赖数据包/方块）。
        // DamageTypeTags::initialize() 注册 BYPASSES_ENCHANTMENTS/WITCH_RESISTANT_TO/IS_FIRE 等全部标签的
        // vanilla 成员集（DamageTypeTags.cpp:434）。未初始化时所有 source.is(tag) 查询命中空集合恒返 false，
        // 致依赖标签的单元测试静默失败（如 WitchEntityTest.MagicDamageReducedBy85Percent 依赖
        // WITCH_RESISTANT_TO 命中、BypassesEnchantmentsTest 依赖 BYPASSES_ENCHANTMENTS 命中）。集中到全局
        // 环境根治此类 setup 陷阱，使依赖标签的测试自包含——与上方 ComponentSerializerRegistry 同模式。
        mc::DamageTypeTags::initialize();

        // 初始化物品与方块物品注册表，按服务端启动序列（RegistryBootstrap::initializeAll）的
        // 依赖拓扑：方块 → 物品 → 方块物品。
        //
        // 方块物品（BlockItem）不在 Items::initialize() 中登记，而由 BlockItemRegistry 的
        // 后置步骤统一注册（Items 显式注册的物品会被复用，不重复注册）。原先此环境不执行该
        // 步骤，导致进程内"方块物品不存在"：村民交易表在首次生成交易时构建，其中的石匠条目
        // 直接按资源位置引用方块物品 minecraft:dripstone_block，取到 nullptr 后触发
        // _stackOf 的 MC_ASSERT_RELEASE 断言，使所有进入村民交易/职业工作逻辑的用例崩溃。
        //
        // 必须放在数据包目录检查之前：方块物品的注册不依赖数据包，且任何用例（含数据包缺失的
        // 非开发机）都期望以相同方式看到完整注册表。
        // 幂等：VanillaBlocks 与 Items 各有 initializing 标志，BlockItemRegistry 以 m_initialized
        // 守卫，用例夹具重复调用均无副作用。
        mc::VanillaBlocks::initialize();
        mc::Items::ensureInitialized();
        mc::BlockItemRegistry::instance().initializeVanillaBlockItems();

        const auto dataPackDir = mc::GameDirectory::defaultDirectory().dataPacksDir();
        if (!std::filesystem::exists(dataPackDir)) {
            return;
        }
        // 仓库作为成员而不是局部变量：模板管理器等组件会持有它的指针（同 MinecraftServer
        // 以 m_dataPackList 成员持有的做法），局部对象在 SetUp 返回后销毁会留下悬垂指针。
        auto scanResult = m_dataPackRepository.scanDirectory(dataPackDir);
        if (!scanResult.success() || scanResult.value() == 0) {
            return;
        }

        // 方块标签与物品标签：数据包是**唯一来源**（BlockTags/ItemTags 内置表已清空，不再有编译期
        // 快照），不在此加载则整张标签表为空，`BlockTags::STAIRS()/LOGS()/...`、
        // `ItemTags::FLOWERS()/...` 全部返回空集。后果是**静默**的行为退化而非报错：处理器里按
        // 标签匹配的分支永不命中（如 BlockAgeProcessor 对全部楼梯的替换、ProtectedBlocksProcessor
        // 对 FEATURES_CANNOT_REPLACE 的保护），合成/繁殖/交易等标签判定也一并失效。标签加载器本身
        // 不依赖 noise/density 注册表，故放在它们之前，与生产启动序列（RegistryBootstrap）一致。
        //
        // 方块标签走 RegistryBootstrap::loadBlockDataPackTags，而非在此重复 clear + 加载：生产路径
        // 与测试环境共用同一段实现，避免两边对「先清空还是后追加」的理解漂移。
        _loadDataPackTags();

        // noise_settings 的 JSON 解析会调 BlockStateParser::parse("minecraft:stone")，
        // 依赖 BlockRegistry 已注册原版方块（已由本环境上方的 VanillaBlocks::initialize 完成，
        // 此处不再重复调用）。

        // Java wire codec 出站边界（BlockUpdate/AddEntity.data/EntityMetadata BlockState 字段）
        // 依赖 JavaBlockStateIdMap 做内部 stateId ↔ Java globalId 翻译。须在方块注册完成后
        // 初始化，否则 codec 往返测试（PlayBlockUpdate 等）会因 miss 降级 air 而 fail。
        if (auto r = mc::network::backend::java::JavaBlockStateIdMap::instance().initialize(); r.failed()) {
            // 非致命：缺失时 codec 往返的 BlockState 字段会 warn+降级 air。
        }

        // BlockEvent wire 出站边界依赖 JavaBlockIdMap 做内部 blockId ↔ Java Block 注册表 id
        // 翻译。须在方块注册完成后初始化，否则 PlayBlockEvent 往返测试会因 miss 降级 air 而 fail。
        if (auto r = mc::network::backend::java::JavaBlockIdMap::instance().initialize(); r.failed()) {
            // 非致命：缺失时 codec 往返的 blockId 字段会 warn+降级 air。
        }

        // 加载顺序：noise → density_function → noise_settings → flat_level_generator_preset（依赖拓扑）。
        // flat preset 依赖 BlockRegistry（层方块，已由上方的 VanillaBlocks::initialize 注册）与
        // BiomeLoader::biomeIdByName（编译期静态 biome 名映射表），不依赖运行时 BiomeRegistry。
        mc::world::gen::noise::NoiseLoader::loadFromDataPackRepository(m_dataPackRepository);
        mc::world::gen::density::DensityFunctionLoader::loadFromDataPackRepository(m_dataPackRepository);
        mc::world::gen::settings::NoiseSettingsLoader::loadFromDataPackRepository(m_dataPackRepository);
        mc::world::gen::settings::FlatLevelGeneratorPresetLoader::loadFromDataPackRepository(m_dataPackRepository);
        // world_preset 依赖 noise_settings（noise 维度存 RL）+ flat_preset（flat 维度内联 settings 复用
        // FlatLevelGeneratorSettings::fromSettingsObject）。
        mc::world::gen::settings::WorldPresetLoader::loadFromDataPackRepository(m_dataPackRepository);
    }

private:
    /// 从当前数据包仓库重建方块标签与物品标签（均为「清空后重建」，数据包是唯一来源）。
    void _loadDataPackTags()
    {
        // 生产路径与测试环境共用同一段方块标签加载实现，避免「先清空还是后追加」的理解漂移。
        mc::server::RegistryBootstrap(m_dataPackRepository, m_lootTableManager, m_predicateManager, m_functionManager)
            .loadBlockDataPackTags();

        mc::item::tag::ItemTags::clear();
        auto itemTagResult = mc::item::tag::ItemTagLoader::loadFromDataPackRepository(m_dataPackRepository);
        if (itemTagResult.failed()) {
            spdlog::error("Failed to load item tags from data packs: {}", itemTagResult.error().message());
        }
    }

    /// 数据包仓库须比 SetUp 活得更久（有些组件持有它的指针），故作为成员。
    mc::resource::DataPackRepository m_dataPackRepository;

    // 仅被 RegistryBootstrap 构造函数引用（loadBlockDataPackTags 不使用它们），
    // 其对应的加载器不参与测试环境的装配。
    mc::loot::LootTableManager m_lootTableManager;
    mc::loot::LootPredicateManager m_predicateManager;
    mc::function::FunctionManager m_functionManager;
};

/// 跨用例隔离监听器：每用例结束后重置 ChunkTaskScheduler 的 thread_local 同步调度上下文。
///
/// 全二进制直跑（`mc_tests --gtest_filter=...`）时同 worker 线程跨用例复用 thread_local
/// SyncSchedulingContext，若上一用例残留 pending 队列，下一用例首次 runInlineAndDrain 会
/// 多一次虚假调度，是 GameEventServerTest 等用例全二进制 flaky 的污染源之一。
/// 生产路径下 depth 出口必归零、pending 必空，故重置对生产无影响。
/// 监听器挂默认事件转发器之后，仅 OnTestEnd 做清理，不干扰断言/输出。
class TestIsolationListener : public ::testing::EmptyTestEventListener {
public:
    /// `main` 进入 steady_clock 读数（毫秒），用于换算进程墙钟。
    static inline mc::i64 s_processStartMs = 0;

    void OnTestEnd(const ::testing::TestInfo& /*test_info*/) override
    {
        mc::server::ChunkTaskScheduler::resetThreadLocalContext();
    }

    /// 进程内全部用例结束（含全局环境 tear-down）时输出一行耗时诊断。
    ///
    /// gtest 报告里的 "(X ms)" 只统计环境 tear-down 结束之前的用例体；ctest 日志中
    /// 每用例 "Passed N sec" 却是整个进程的墙钟，二者之差包含进程启动（ELF 装载、
    /// 静态初始化、ASan 运行库初始化）、`main` 收尾（spdlog/单例析构、ASan 泄漏检查）
    /// 与退出。本行在 `main` 返回前打印，CPU 时间在进程退出前读到，用于把 ctest 口径
    /// 与用例口径对账，定位 CI 上单用例固定开销的来源。
    void OnTestProgramEnd(const ::testing::UnitTest& unitTest) override
    {
        const auto wallMs = unitTest.elapsed_time();
        const auto uptimeMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count() -
            s_processStartMs;
        spdlog::info("[TestTiming] tests_wall={}ms process_uptime={}ms cpu={}ms", wallMs, uptimeMs, _processCpuMs());
    }

private:
    /// 读取本进程累计消耗的用户态 + 内核态 CPU 时间（毫秒）。非 Linux 平台返回 -1。
    static mc::i64 _processCpuMs()
    {
#ifdef __linux__
        struct rusage usage{};
        if (::getrusage(RUSAGE_SELF, &usage) != 0) {
            return -1;
        }
        return static_cast<mc::i64>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000 +
            (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000;
#else
        return -1;
#endif
    }
};

// 进程启动成本对照实验（tests/ci-probes）复用本文件的全局环境与监听器，并以此宏跳过 main。
#ifndef MC_TESTSTART_EMBED_MAIN
