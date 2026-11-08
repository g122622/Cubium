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
 * @brief 差分能力：岩浆与周围流体/方块相遇时的产物（原版 `LiquidBlock#shouldSpreadLiquid`）。
 *
 * `list`：岩浆源 / 流动岩浆 × 五个水平方向的邻居种类
 * `run` ：一行 -> { "result": 产物方块 id 或 null, "levelEvents": [...] }
 *
 * ## 四条规则
 *
 * | 自身 | 相邻 | 产物 |
 * |---|---|---|
 * | 岩浆源 | 水 | **黑曜石** |
 * | 流动岩浆 | 水 | **圆石** |
 * | 岩浆（任意，下方是灵魂土） | 蓝冰 | **玄武岩** |
 * | 其它 | — | 不变（返回 true 继续调度流体刻） |
 *
 * ## 两个易错点
 *
 * 1. **源与流动的产物不同**：写成一个（都出黑曜石或都出圆石）是最常见的错误。
 *    玩家浇岩浆造石头时得到黑曜石，或者造黑曜石时只得到圆石。
 * 2. **只看水平四个方向，不看正上方**：原版 `POSSIBLE_FLOW_DIRECTIONS` 是
 *    DOWN + 四个水平方向。往岩浆正上方倒水不会当场凝固 —— 水会流到旁边再反应。
 *    把上方算进去，会让「水流过岩浆上方」这种常见场景瞬间凝固成一片石头。
 */

#include "common/world/effect/base/LevelEventId.hpp"
#include <algorithm>
#include <cstdio>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/blocks/fluid/LiquidBlock.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include "probe.hpp"
#include "probe_world.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static const bool s_initialized = []() {
        datapack::initializeRegistries();
        Items::initialize();
        return true;
    }();
    (void)s_initialized;
}

constexpr const char* SELF_KINDS[] = {"lava_source", "lava_flowing"};
constexpr const char* NEIGHBOR_KINDS[] = {"water", "blue_ice", "air"};
constexpr const char* DIRECTIONS[] = {"north", "south", "east", "west"};

/** 岩浆反应场景世界。 */
class ReactSceneWorld final : public IWorld {
public:
    void reset()
    {
        m_blocks.clear();
        m_levelEvents.clear();
        m_random = math::Random(12345);
    }

    void put(i32 x, i32 y, i32 z, const BlockState* state)
    {
        m_blocks[key(x, y, z)] = state;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(key(x, y, z));
        if (it != m_blocks.end() && it->second != nullptr) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[key(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        return getBlockState(x, y, z)->getFluidState();
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        return world::MIN_BUILD_HEIGHT;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    void playEvent(::mc::world::LevelEventId eventId, const BlockPos& /*pos*/, i32 /*data*/) override
    {
        m_levelEvents.push_back(static_cast<i32>(eventId));
    }

    [[nodiscard]] const std::vector<i32>& levelEvents() const noexcept
    {
        return m_levelEvents;
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

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId) override
    {
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId) const override
    {
        return nullptr;
    }

    [[nodiscard]] DimensionId dimension() const override
    {
        return 0;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 0;
    }

    [[nodiscard]] u64 currentTick() const override
    {
        return 0;
    }

    [[nodiscard]] i64 dayTime() const override
    {
        return 6000;
    }

    [[nodiscard]] bool isClientSide() const override
    {
        return false;
    }

    [[nodiscard]] bool isHardcore() const override
    {
        return false;
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return Difficulty::Normal;
    }

    [[nodiscard]] bool isRaining() const override
    {
        return false;
    }

    [[nodiscard]] bool isThundering() const override
    {
        return false;
    }

    [[nodiscard]] math::IRandom& getRandom() override
    {
        return m_random;
    }

    [[nodiscard]] const math::IRandom& getRandom() const override
    {
        return m_random;
    }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        return m_worldBorder;
    }

    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        return m_worldBorder;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::logic_error("fluid_lava_react 场景不推进刻度");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("fluid_lava_react 场景不推进刻度");
    }

    [[nodiscard]] ecs::EntityRegistry* entityRegistry() override
    {
        return m_world.entityRegistry();
    }

    [[nodiscard]] const ecs::EntityRegistry* entityRegistry() const override
    {
        return m_world.entityRegistry();
    }

private:
    static i64 key(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x & 0x3FFFFFF) << 38) ^ (static_cast<i64>(y & 0xFFF) << 26) ^ (z & 0x3FFFFFF);
    }

    probe::NullWorld m_world;
    std::map<i64, const BlockState*> m_blocks;
    std::vector<i32> m_levelEvents;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

void writeList(FILE* out)
{
    ensureInitialized();
    for (const char* self : SELF_KINDS) {
        for (const char* dir : DIRECTIONS) {
            for (const char* neighbor : NEIGHBOR_KINDS) {
                std::fprintf(out, "%s:%s:%s\n", self, dir, neighbor);
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t first = unit.find(':');
    const std::size_t last = unit.rfind(':');
    if (first == std::string::npos || first == last) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string self = unit.substr(0, first);
    const std::string dirName = unit.substr(first + 1, last - first - 1);
    const std::string neighbor = unit.substr(last + 1);

    if (self != "lava_source" && self != "lava_flowing") {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    ReactSceneWorld world;
    world.reset();

    // 岩浆本体：源是 level=0，流动是 level=2
    const BlockState& lavaBase = VanillaBlocks::LAVA->defaultState();
    const BlockState lava = self == "lava_source" ? lavaBase.with(BlockStateProperties::LEVEL_0_15(), 0)
                                                  : lavaBase.with(BlockStateProperties::LEVEL_0_15(), 2);
    world.put(0, 64, 0, &lava);

    // 邻居
    const BlockState* neighborState = &VanillaBlocks::AIR->defaultState();
    if (neighbor == "water") {
        neighborState = &VanillaBlocks::WATER->defaultState();
    } else if (neighbor == "blue_ice") {
        neighborState = &VanillaBlocks::BLUE_ICE->defaultState();
    }
    // 直接按方位写死偏移，避开 Direction 枚举索引的依赖。
    i32 nx = 0;
    i32 nz = 0;
    if (dirName == "north") {
        nz = -1;
    } else if (dirName == "south") {
        nz = 1;
    } else if (dirName == "east") {
        nx = 1;
    } else {
        nx = -1;
    }
    world.put(nx, 64, nz, neighborState);

    // 灵魂土在下方，用于蓝冰 -> 玄武岩路径
    world.put(0, 63, 0, &VanillaBlocks::SOUL_SOIL->defaultState());

    auto& lavaBlock = static_cast<block::LiquidBlock&>(*VanillaBlocks::LAVA);
    MC_UNUSED(lavaBlock.reactWithNeighbors(world, BlockPos(0, 64, 0), lava));

    json result{{"unit", unit}};
    const BlockState* after = world.getBlockState(0, 64, 0);
    result["result"] = after != nullptr ? after->getBlock().blockLocation().toString() : "";
    result["levelEvents"] = world.levelEvents();
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(fluid_lava_react, &writeList, &runLine)

} // namespace mc::diff
