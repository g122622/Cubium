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
 * @file crop_growth.cpp
 * @brief 差分能力：作物的一次随机刻（生长速率、抽芽、结果、耕地保墒）。
 *
 * `list`：固定场景名
 * `run` ：{ "unit": "<场景名>" }
 *         → { "unit": ..., "ok": true, "growthSpeed": 3.0,
 *             "afterBlock": "minecraft:wheat", "afterAge": 1,
 *             "neighbours": { "north": "<air>", ... }, "ground": "minecraft:farmland#0" }
 *
 * ## 为什么要手工编地形
 *
 * 生长速率不是方块自己的属性，而是**周围 3×3 地形**的函数：脚下九格的耕地与湿度、
 * 左右前后有没有同类作物、四个斜角有没有。出错的地方也都在这里——相邻耕地少算一格、
 * 湿度判成 0、斜角漏判——而且都不报错，只是「这块地的作物长得比原版慢」。这类错误
 * 只有在给定地形下算一遍速率才看得见。
 *
 * 每个场景给一片 7×7 地形（行优先，中心是作物），两侧用同一份地形重建世界，跑一次
 * 随机刻，再比对速率、生长结果与四个邻居。
 *
 * 地形用字符编码，两侧各有一份同样的解码表：
 * - `.` 空气
 * - `0`-`7` 耕地，数字就是湿度值
 * - `c` 与探针作物同种的作物（同一年龄）
 * - `b` 竹子（带 `minecraft:dirt` 标签，用于梗结果时的支撑判定）
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"
#include "probe_world.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

// ============================================================================
// 场景表
// ============================================================================

/**
 * @brief 一个场景：作物 + 年龄 + 随机种子 + 7×7 地形。
 *
 * 地形第 3 行第 3 列是作物所在格，写什么字符都行（由 @c subject 决定）；它脚下那格
 * 的湿度取自同一个字符（数字即湿度，非数字即 0）。
 */
struct Scenario {
    const char* unit;
    const char* subject; ///< 作物方块 ID
    int age;             ///< 作物年龄（0-7）
    int seed;            ///< 随机数种子
    u8 skyLight;         ///< 天空光照（原版作物要求 ≥9，`canSurvive` 要求 ≥8）
    bool raining;        ///< 是否下雨（只影响耕地，作物不看）
    const char* rows[7]; ///< 7×7 地形
};

const Scenario kScenarios[] = {
    // ---- 生长速率：纯耕地排布 ----
    {.unit = "speed_open_field",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 1,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", "...0...", ".......", ".......", "......."}},
    {.unit = "speed_surrounded_dry",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 2,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", "..000..", "..000..", "..000..", ".......", "......."}},
    {.unit = "speed_surrounded_wet",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 3,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", "..777..", "..777..", "..777..", ".......", "......."}},
    {.unit = "speed_mixed_moisture",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 4,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", "..017..", "..304..", "..652..", ".......", "......."}},
    {.unit = "speed_no_farmland",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 5,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", ".......", ".......", ".......", "......."}},

    // ---- 生长速率：同类作物挤占 ----
    {.unit = "speed_cvenc_ns_line",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 6,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", "...c...", "..c0c..", "...c...", ".......", "......."}},
    {.unit = "speed_cvenc_we_line",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 7,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", ".c0c...", ".......", ".......", "......."}},
    // 只有一对斜角邻居（西北角），没有左右/前后成对：原版走 else 分支仍 ÷2。
    {.unit = "speed_cvenc_n_diag",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 8,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", "..c0...", "...c...", ".......", "......."}},
    // 只两侧成对（左右），没有斜角：÷2。
    {.unit = "speed_cvenc_nw_diag",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 9,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", ".c0c...", ".......", ".......", "......."}},
    {.unit = "speed_cvenc_se_diag",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 10,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", "...0...", "....c..", ".......", "......."}},
    {.unit = "speed_cvenc_axis_and_diag",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 11,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", "..cc...", "..c0c..", "...c...", ".......", "......."}},
    {.unit = "speed_cvenc_wet_crowded",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 12,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", "..777..", "c.707.c", "..777..", ".......", "......."}},

    // ---- 作物生长：随机刻的各分支 ----
    {.unit = "wheat_age_0_dark",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 13,
        .skyLight = 4,
        .raining = false,
        .rows = {".......", ".......", ".......", "...0...", ".......", ".......", "......."}},
    {.unit = "wheat_age_0_bright",
        .subject = "minecraft:wheat",
        .age = 0,
        .seed = 14,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", "...0...", ".......", ".......", "......."}},
    {.unit = "wheat_age_6_last_step",
        .subject = "minecraft:wheat",
        .age = 6,
        .seed = 15,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", "...0...", ".......", ".......", "......."}},
    {.unit = "wheat_age_7_mature",
        .subject = "minecraft:wheat",
        .age = 7,
        .seed = 16,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", "...0...", ".......", ".......", "......."}},
    {.unit = "carrots_age_3_wet",
        .subject = "minecraft:carrots",
        .age = 3,
        .seed = 17,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", "..777..", "..777..", "..777..", ".......", "......."}},
    {.unit = "beetroots_age_5_crowded",
        .subject = "minecraft:beetroots",
        .age = 5,
        .seed = 18,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", "..ccc..", "..c0c..", "..cc...", ".......", "......."}},

    // ---- 梗长成：结果与不结果 ----
    {.unit = "stem_age_3_grow",
        .subject = "minecraft:pumpkin_stem",
        .age = 3,
        .seed = 19,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", "...0...", ".......", ".......", "......."}},
    {.unit = "stem_age_7_no_support",
        .subject = "minecraft:pumpkin_stem",
        .age = 7,
        .seed = 20,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", "...0...", ".......", ".......", "......."}},
    {.unit = "stem_age_7_farmland_ring",
        .subject = "minecraft:pumpkin_stem",
        .age = 7,
        .seed = 21,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", "..777..", "..777..", "..707..", "..777..", "..777..", "......."}},
    {.unit = "stem_age_7_dirt_ring",
        .subject = "minecraft:melon_stem",
        .age = 7,
        .seed = 22,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", "..bbb..", "..bbb..", "..b0b..", "..bbb..", "..bbb..", "......."}},
    {.unit = "stem_age_7_blocked",
        .subject = "minecraft:melon_stem",
        .age = 7,
        .seed = 23,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", "..bbb..", "..bcb..", "..b0b..", "..bbb..", "..bbb..", "......."}},

    // ---- 耕地保墒 ----
    {.unit = "farmland_dry_no_crop",
        .subject = "minecraft:farmland",
        .age = 3,
        .seed = 24,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", "...0...", ".......", ".......", "......."}},
    {.unit = "farmland_dry_with_crop",
        .subject = "minecraft:farmland",
        .age = 3,
        .seed = 25,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", ".......", ".......", ".......", "......."}},
    {.unit = "farmland_wet_no_nearby_water",
        .subject = "minecraft:farmland",
        .age = 5,
        .seed = 26,
        .skyLight = 15,
        .raining = false,
        .rows = {".......", ".......", ".......", ".......", ".......", ".......", "......."}},
};

[[nodiscard]] std::vector<std::string> unitNames()
{
    std::vector<std::string> units;
    for (const auto& scenario : kScenarios) {
        units.emplace_back(scenario.unit);
    }
    std::sort(units.begin(), units.end());
    return units;
}

// ============================================================================
// 世界
// ============================================================================

/**
 * @brief 按场景摆好一片小地形的世界。
 *
 * 只实现随机刻会碰到的访问器：方块读写、流体、光照、天气、随机。其余走 IWorld 默认。
 */
class GrowthWorld final : public IBlockReader {
public:
    GrowthWorld() = default;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(Key{x, y, z});
        return it == m_blocks.end() ? nullptr : it->second;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(Key{x, y, z});
        } else {
            m_blocks[Key{x, y, z}] = state;
        }
        return true;
    }

    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override
    {
        // 只有中心格及其上方／四周是"露天的"，其余一律 0：与 oracle 写入的光照数组同口径。
        const i32 dx = x < 0 ? -x : x;
        const i32 dz = z < 0 ? -z : z;
        const bool exposed = dx <= 4 && dz <= 4 && y >= 0 && y <= 1;
        return exposed ? m_skyLight : 0;
    }

    [[nodiscard]] bool isRaining() const override
    {
        return m_raining;
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        return 256;
    }

    [[nodiscard]] math::IRandom& getRandom() override
    {
        return m_random;
    }

    [[nodiscard]] const math::IRandom& getRandom() const override
    {
        return m_random;
    }

    [[nodiscard]] u64 seed() const override
    {
        return m_seed;
    }

    void configure(u8 skyLight, bool raining, int seed)
    {
        m_skyLight = skyLight;
        m_raining = raining;
        m_seed = static_cast<u64>(seed);
        m_random = math::Random(static_cast<u64>(seed));
    }

    void put(i32 x, i32 y, i32 z, const BlockState& state)
    {
        m_blocks[Key{x, y, z}] = &state;
    }

    // ========== IWorld 的其余纯虚：随机刻用不到，给最小实现 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return nullptr;
    }

    [[nodiscard]] bool isHardcore() const override
    {
        return false;
    }

    [[nodiscard]] bool isThundering() const override
    {
        return false;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override
    {
        return {};
    }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] PhysicsEngine* physicsEngine() override
    {
        return nullptr;
    }

    [[nodiscard]] const PhysicsEngine* physicsEngine() const override
    {
        return nullptr;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::logic_error("GrowthWorld::tickManager 不可用：作物随机刻不应依赖世界 tick");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("GrowthWorld::tickManager 不可用：作物随机刻不应依赖世界 tick");
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] DimensionId dimension() const override
    {
        return 0; // 主世界
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return Difficulty::Normal;
    }

    [[nodiscard]] bool isClientSide() const override
    {
        return false;
    }

    [[nodiscard]] u64 currentTick() const override
    {
        return 0;
    }

    [[nodiscard]] i64 dayTime() const override
    {
        return 0;
    }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        return m_worldBorder;
    }

    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        return m_worldBorder;
    }

    [[nodiscard]] ecs::EntityRegistry* entityRegistry() override
    {
        return nullptr;
    }

    [[nodiscard]] const ecs::EntityRegistry* entityRegistry() const override
    {
        return nullptr;
    }

private:
    world::border::WorldBorder m_worldBorder;

    struct Key {
        i32 x;
        i32 y;
        i32 z;

        bool operator==(const Key& other) const noexcept
        {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct KeyHash {
        size_t operator()(const Key& key) const noexcept
        {
            size_t h = static_cast<size_t>(static_cast<u32>(key.x)) * 0x9E3779B1u;
            h ^= static_cast<size_t>(static_cast<u32>(key.y)) + 0x7F4A7C15u + (h << 6) + (h >> 2);
            h ^= static_cast<size_t>(static_cast<u32>(key.z)) + 0x9E3779B9u + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_map<Key, const BlockState*, KeyHash> m_blocks;
    math::Random m_random{0};
    u64 m_seed = 0;
    u8 m_skyLight = 15;
    bool m_raining = false;
};

// ============================================================================
// 场景装配与执行
// ============================================================================

/// 构造场景用的方块状态：作物/耕地都按 `age` 字段取值。
const BlockState& makeSubjectState(const Block& block, int age)
{
    const BlockState& base = block.defaultState();
    const auto& ageProp = BlockStateProperties::AGE_0_7();
    if (base.hasProperty(ageProp)) {
        return base.with(ageProp, std::clamp(age, 0, 7));
    }
    const auto& moisture = BlockStateProperties::MOISTURE_0_7();
    if (base.hasProperty(moisture)) {
        return base.with(moisture, std::clamp(age, 0, 7));
    }
    return base;
}

/// 把 7×7 地形铺进世界；中心格放作物，中心格脚下放耕地（湿度取自中心字符）。
void buildTerrain(GrowthWorld& world, const Scenario& scenario, const BlockState& subjectState)
{
    const Block* subjectBlock = BlockRegistry::instance().getBlock(ResourceLocation(scenario.subject));
    const BlockState* cropState = subjectBlock == nullptr ? nullptr : &makeSubjectState(*subjectBlock, scenario.age);

    const BlockState* farmland =
        VanillaBlocks::FARMLAND == nullptr ? nullptr : &VanillaBlocks::FARMLAND->defaultState();
    const Block* bamboo = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:bamboo"));

    for (i32 row = 0; row < 7; ++row) {
        for (i32 col = 0; col < 7; ++col) {
            const char cell = scenario.rows[row][col];
            const i32 x = col - 3;
            const i32 z = row - 3;

            if (row == 3 && col == 3) {
                world.put(x, 0, z, subjectState);
                continue;
            }
            if (cell == '.') {
                continue;
            }
            if (cell >= '0' && cell <= '7') {
                if (farmland != nullptr) {
                    // 耕地湿度是枚举属性，每个取值都是状态池里的固定实例，取址是安全的。
                    world.put(
                        x, 0, z, farmland->with(BlockStateProperties::MOISTURE_0_7(), static_cast<int>(cell - '0')));
                }
                continue;
            }
            if (cell == 'c' && cropState != nullptr) {
                world.put(x, 0, z, *cropState);
                continue;
            }
            if (cell == 'b' && bamboo != nullptr) {
                world.put(x, 0, z, bamboo->defaultState());
                continue;
            }
        }
    }

    if (farmland != nullptr) {
        const char centerGround = scenario.rows[3][3];
        const int moisture = (centerGround >= '0' && centerGround <= '7') ? centerGround - '0' : 0;
        world.put(0, -1, 0, farmland->with(BlockStateProperties::MOISTURE_0_7(), moisture));
    }
}

/// 方块状态的简短表示，用于两个耕地场景比对（`minecraft:farmland#3`）。
std::string describeState(const BlockState* state)
{
    if (state == nullptr) {
        return "<air>";
    }
    std::string text = state->getBlock().blockLocation().toString();
    const auto& moisture = BlockStateProperties::MOISTURE_0_7();
    if (state->hasProperty(moisture)) {
        text += "#" + std::to_string(state->get(moisture));
    }
    return text;
}

std::string runScenario(const Scenario& scenario)
{
    datapack::initializeRegistries();

    const Block* block = BlockRegistry::instance().getBlock(ResourceLocation(scenario.subject));
    if (block == nullptr) {
        return json{{"unit", scenario.unit}, {"ok", false}, {"error", "unknown block"}}.dump();
    }

    GrowthWorld world;
    world.configure(scenario.skyLight, scenario.raining, scenario.seed);

    // 状态对象由方块自己持有（状态池），这里只借用引用；随机刻需要可写的 BlockState，
    // 因此从世界表里取回同一个实例再改。
    buildTerrain(world, scenario, makeSubjectState(*block, scenario.age));

    json result = json{{"unit", scenario.unit}, {"ok", true}};

    // 生长速率：作物与梗共用同一条公式，耕地不适用。
    const bool isCrop = dynamic_cast<const blocks::CropBlock*>(block) != nullptr;
    const bool isStem = std::string_view(scenario.subject).find("_stem") != std::string_view::npos;
    if (isCrop || isStem) {
        result["growthSpeed"] =
            static_cast<double>(blocks::CropBlock::getGrowthChance(*block, world, BlockPos(0, 0, 0)));
    }

    // 一次随机刻。两侧从同一种子开始，掷骰序列必须一致。
    // 随机刻会改写状态：传进去的必须和世界表里是同一个可写实例。
    // 「暗场景」用盖顶表达：与 oracle 同一约定（光照本身是环境输入，不参与比对）。
    if (scenario.skyLight < 9 && VanillaBlocks::STONE != nullptr) {
        world.put(0, 1, 0, VanillaBlocks::STONE->defaultState());
    }

    BlockState* mutableState = const_cast<BlockState*>(world.getBlockState(0, 0, 0));
    const_cast<Block*>(block)->randomTick(world, BlockPos(0, 0, 0), *mutableState, world.getRandom());

    const BlockState* after = world.getBlockState(0, 0, 0);
    result["after"] = describeState(after);

    // 中心格上方与四个水平邻居：梗长成时应有一个变成果实，其余不变。
    static constexpr i32 kDirs[5][3] = {{0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};
    static const char* kDirNames[5] = {"up", "north", "south", "west", "east"};
    json neighbours = json::object();
    for (i32 i = 0; i < 5; ++i) {
        neighbours[kDirNames[i]] =
            describeState(world.getBlockState(0 + kDirs[i][0], 0 + kDirs[i][1], 0 + kDirs[i][2]));
    }
    result["neighbours"] = neighbours;
    result["ground"] = describeState(world.getBlockState(0, -1, 0));

    return result.dump();
}

void writeList(FILE* out)
{
    for (const auto& name : unitNames()) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    for (const auto& scenario : kScenarios) {
        if (unit == scenario.unit) {
            return runScenario(scenario);
        }
    }
    return json{{"unit", unit}, {"ok", false}, {"error", "unknown scenario"}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("crop_growth", &writeList, &runLine)

} // namespace mc::diff
