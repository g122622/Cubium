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

#pragma once

/**
 * @file loot_probe_common.hpp
 * @brief 三个战利品差分探针（清单 / 方块 / 实体）共享的加载与掷骰逻辑。
 *
 * 三个探针比对的是同一批战利品表的三种上下文：
 *
 *   - `loot_table`   ：同名单元，但**不提供任何参数**——把「表能不能按参数集被枚举、
 *                      池与条目能不能在没有实体/方块的情况下掷出来」单独隔离开；
 *   - `loot_table_block`：笨拙地按方块掉落的方式构造上下文（`block_state` + `origin` + 工具 + 时运/精准采集）；
 *   - `loot_table_entity`：按生物死亡的方式构造上下文（`this_entity` + `attacking_entity` + `looting`）。
 *
 * 三者共用这里的加载流程：从原版数据包目录加载全部 `loot_table/*.json` 与
 * `predicates/*.json`，注册到同一个管理器，供探针枚举与掷骰。
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/loot/LootPredicateLoader.hpp"
#include "common/item/loot/LootPredicateManager.hpp"
#include "common/item/loot/LootCallbackInvoker.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableLoader.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "server/item/instrument/InstrumentLoader.hpp"
#include "server/item/instrument/InstrumentTagLoader.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "loot_world.hpp"
#include "probe.hpp"

namespace mc::diff::lootprobe {

using nlohmann::json;

/// 掷骰次数：原版侧每次都用同一个 RandomSource 种子（`LootTable.getRandomItems(params, seed)`）。
constexpr u64 ROLL_SEED = 0x0005EED2026A11CE0ULL;

/// 样本数：多跑几次才能暴露「权重/掷骰次数算错，但恰好蒙对一次」的问题。
constexpr int ROLL_SAMPLES = 4;

/// 每个样本之间推进随机源的次数（原版侧同样推进），避免所有样本看到同一段序列。
constexpr std::uint64_t SAMPLE_STRIDE = 64;

/// 时运 / 掠夺等级：取 0 与 3（原版上限是 3 级）。
constexpr i32 LOOTING_LEVELS[] = {0, 3};

/**
 * @brief 探针运行期的共享状态：数据包、注册表与实体。
 *
 * 只构造一次（三个探针各自一份），因为加载 1300 多张表 + 全部谓词的开销远大于掷骰。
 */
struct LootEnv {
    loot::LootTableManager tables;
    loot::LootPredicateManager predicates;
    std::unique_ptr<lootworld::NullWorld> world;
    std::unique_ptr<Entity> thisEntity;
    std::unique_ptr<Entity> attacker;
    /// 探针的 list 输出（排序后的表 ID）；加载完成时填好。
    std::vector<std::string> tableIds;
    /// 原版侧按表列出了哪些表在两侧都存在；探针只输出自己有的表。
    std::string loadError;
};

/// 数据包根目录（含 `data/` 的那一层）。由 harness 通过环境变量传入。
[[nodiscard]] inline std::filesystem::path dataPackRoot()
{
    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    return std::filesystem::path(root);
}

/**
 * @brief 加载数据包里的战利品表与谓词。
 *
 * 与 `RegistryBootstrap::initializeAll` 的顺序保持一致：方块 → 物品 → 物品标签 →
 * 战利品表 → 谓词 → 实体类型。实体类型放在最后，是因为「实体」探针需要用它构造
 * `this_entity`；其余两个探针用不到，但统一初始化能保证三个探针看到同一套注册表。
 */
inline LootEnv& environment()
{
    static LootEnv s_env = [] {
        LootEnv env;
        // 注册顺序对齐 RegistryBootstrap：方块 → 方块标签 → 物品 → 物品标签 → 附魔。
        // 战利品表里的 item/tag 条目按资源名查物品与标签，`copy_state` 按方块默认状态
        // 取属性，所以这四者都必须先就位。
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
        item::tag::ItemTags::initialize();
        item::enchant::EnchantmentRegistry::initialize();
        // 方块物品：战利品表大量产出方块物品（`minecraft:stone` 等），
        // 没有它们这些表会「掷出空堆」而不是掉落方块。
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        // 实体探针要用 this_entity / attacking_entity 构造上下文。
        entity::VanillaEntities::registerAll();


        const auto root = dataPackRoot();

        // 乐器与乐器标签：`set_instrument` 函数按标签名解析，标签必须先于战利品表加载。
        // 与 RegistryBootstrap 一样从数据包目录读，两边结果一致。
        resource::DataPackRepository packs;
        if (std::filesystem::exists(root / "data")) {
            // scanDirectory 扫描的是「数据包所在目录」，每个子目录自身必须是数据包
            // （含 pack.mcmeta），所以这里传的不是数据包根而是它的父目录。
            packs.scanDirectory(root.parent_path());
        }
        if (auto loaded = server::item::InstrumentLoader::loadFromDataPackRepository(packs); loaded.failed()) {
            env.loadError += "乐器加载失败: " + loaded.error().toString() + "\n";
        }
        if (auto tags = server::item::InstrumentTagLoader::loadFromDataPackRepository(packs); tags.failed()) {
            env.loadError += "乐器标签加载失败: " + tags.error().toString() + "\n";
        }

        const auto lootDir = root / "data" / "minecraft" / "loot_table";
        if (!std::filesystem::exists(lootDir)) {
            env.loadError = "找不到战利品表目录：" + lootDir.string();
            return env;
        }

        // 逐文件加载而不是用 DataPackRepository：探针不需要资源包堆叠语义，
        // 直接遍历目录能让失败原因落到具体文件上。
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(lootDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());

        loot::LootTableLoader loader(env.tables);
        for (const auto& file : files) {
            auto result = loader.loadFile(file.generic_string());
            if (!result.success()) {
                // 加载失败的表不会出现在 list 里；原版侧输出同名单元时会以
                // "<absent in probe>" 报出，缺口因此不会被静默吞掉。
                env.loadError += file.generic_string() + ": " + result.error().toString() + "\n";
            }
        }

        // 谓词：`minecraft:reference` 条件通过谓词管理器解析。
        // 谓词 ID 由路径里 `predicates/` 的位置推导；原版 1.21.11 数据包不带任何谓词
        // 文件（命名谓词是数据包功能），目录不存在时跳过即可。
        const auto predicateDir = root / "data" / "minecraft" / "predicates";
        if (std::filesystem::exists(predicateDir)) {
            loot::LootPredicateLoader predicateLoader(env.predicates);
            auto predicateResult = predicateLoader.loadFromDirectory(predicateDir.generic_string());
            if (!predicateResult.success()) {
                env.loadError += "谓词加载失败: " + predicateResult.error().toString() + "\n";
            }
        }
        env.tables.setPredicateManager(&env.predicates);

        env.tableIds = env.tables.getAllTableIds();
        std::sort(env.tableIds.begin(), env.tableIds.end());

        env.world = std::make_unique<lootworld::NullWorld>(&env.tables);
        env.thisEntity = lootworld::createEntity(*env.world, "minecraft:pig");
        env.attacker = lootworld::createEntity(*env.world, "minecraft:zombie");

        return env;
    }();

    if (!s_env.loadError.empty()) {
        // 加载失败一律写 stderr：stdout 是协议通道。
        std::fprintf(stderr, "%s", s_env.loadError.c_str());
    }
    return s_env;
}

/**
 * @brief 主动释放 {@link environment()} 持有的状态，必须在 `main` 返回**之前**调用。
 *
 * 为什么不能交给静态析构：`LootEnv` 里持有 `Entity`（其中包含脚本句柄）。进程退出时
 * C 运行时会按「构造的逆序」析构静态对象，而注册表类单例（如 `ScriptHandleRegistry`）
 * 可能先于 `LootEnv` 被销毁；`Entity::~Entity` 再去访问它们就是访问已析构的内存，
 * 表现为退出阶段访问违例（退出码 0xC0000005）。这会让编排层把**成功的运行**判成环境问题。
 *
 * 显式在 main 里调用，析构顺序就完全确定了。
 */
inline void releaseEnvironment()
{
    environment().world.reset();
    environment().attacker.reset();
    environment().thisEntity.reset();
}

/// 把一批物品堆写成 JSON 数组：`{"id": ..., "count": ...}`，按物品名、再按数量排序。
///
/// 只比「哪个物品、多少个」：附魔、药水等随机组件由 item_components harness 单独比对，
/// 混进这里会让掉落表的差异被组件差异淹没。
[[nodiscard]] inline json summariseStacks(const std::vector<ItemStack>& stacks)
{
    json result = json::array();
    for (const auto& stack : stacks) {
        if (stack.isEmpty() || stack.getItem() == nullptr) {
            continue;
        }
        result.push_back(json{
            {"id", stack.getItem()->itemLocation().toString()},
            {"count", stack.getCount()},
        });
    }
    std::sort(result.begin(), result.end(), [](const json& left, const json& right) {
        const auto leftId = left.at("id").get<std::string>();
        const auto rightId = right.at("id").get<std::string>();
        if (leftId != rightId) {
            return leftId < rightId;
        }
        return left.at("count").get<i32>() < right.at("count").get<i32>();
    });
    return result;
}

/**
 * @brief 掷骰 {@link #ROLL_SAMPLES} 次，每次的上下文由 `prepare` 往构建器里填参数。
 *
 * 随机源与原版 oracle 逐位一致：每个样本都从 {@link #ROLL_SEED} 重新开始，先推进
 * `sample × SAMPLE_STRIDE` 次，再掷骰（对齐 `LootRoll.randomForSample`）。
 *
 * 参数集校验与 **原版** 对齐：原版 `LootParams.Builder#create(ContextKeySet)` 在缺
 * 必需参数时直接抛错，调用方（此处即 oracle）按「该样本无产出」处理。Cubium 的
 * `LootContextBuilder::build(paramSet)` 只打警告仍返回上下文，因此这里显式做同一
 * 项校验并跳过不合格的样本，两侧结果口径才一致。
 */
template <typename PrepareContext>
[[nodiscard]] json rollSamples(
    const loot::LootTable& table, lootworld::NullWorld& world,
    const loot::LootParameterSet& paramSet, PrepareContext&& prepare)
{
    json samples = json::array();
    for (int sample = 0; sample < ROLL_SAMPLES; ++sample) {
        math::Xoroshiro128ppRandom random(ROLL_SEED);
        for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(sample) * SAMPLE_STRIDE; ++i) {
            static_cast<void>(random.nextU64());
        }

        loot::LootContextBuilder builder(world);
        builder.withRandom(random)
            .withLootTableResolver([&world](const std::string& id) -> const loot::LootTable* {
                const auto* manager = world.lootTableManager();
                return manager != nullptr ? manager->getTable(id) : nullptr;
            })
            .withPredicateResolver([&world](const std::string& id) -> const loot::LootCondition* {
                const auto* manager = world.lootTableManager();
                return manager != nullptr ? manager->getPredicate(id) : nullptr;
            });
        prepare(builder);

        // 先构建一次上下文，取出「原始参数表」——原版 `LootParams` 就是这一份。
        auto context = builder.build(paramSet);
        const bool canRoll = context != nullptr && paramSet.validate(context->getParamIds());
        if (!canRoll) {
            samples.push_back(json::array());
            continue;
        }
        // 顶层函数只读自己的参数集：用原始参数表重建上下文再掷骰，
        // 与构造阶段塞进去的额外键（例如实体表的 `tool`）彻底隔离。
        //
        // 走服务端同一个入口 `LootTable::generate`：装饰链（条目 → 池 → 表）逐层展开，
        // 产出按原版 `createStackSplitter` 的口径合并——与 oracle 的收集方式一致。
        std::vector<ItemStack> stacks;
        world.rollLoot(table, *context, stacks);
        samples.push_back(summariseStacks(stacks));
    }
    return samples;
}

/// 解析一行输入里的 `origin`：[x, y, z]；缺省 (0.5, 64, 0.5) 与原版 oracle 一致。
[[nodiscard]] inline Vector3 parseOrigin(const json& input)
{
    if (!input.contains("origin")) {
        return Vector3(0.5f, 64.0f, 0.5f);
    }
    const auto& origin = input.at("origin");
    return Vector3(origin.at(0).get<f32>(), origin.at(1).get<f32>(), origin.at(2).get<f32>());
}

} // namespace mc::diff::lootprobe