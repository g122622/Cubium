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
 * @brief 差分能力：方块状态是否**红石导体**（原版 `BlockBehaviour#isRedstoneConductor`）。
 *
 * `list`：全部已注册方块的全部状态
 * `run` ：一行 -> { "conductor": bool }
 *
 * ## 这个标志决定什么
 *
 * 红石粉向四周传播信号时会穿过导体方块向下找信号源；非导体则不会。
 * 它同时影响：
 *
 * - 红石粉能否给上方/下方的方块充能
 * - 火把、拉杆、中继器能否隔着方块被充能
 * - 观察者、活塞的充能判定
 *
 * ## 默认与例外
 *
 * 原版默认判据是「碰撞形状完整覆盖整格」（`isCollisionShapeFullBlock`），
 * 然后由 22 处 `BlockProperties#isRedstoneConductor` 逐个覆盖，绝大多数是
 * `Blocks::never`。
 *
 * 两类最容易写错的方块：
 *
 * - **红石块**：不透明、形状完整，却显式注册成 `never`（否则红石块会把自己
 *   周围的粉充能两次）
 * - **玻璃、树叶**：形状完整但显式排除
 * - **单层台阶、楼梯**：材质不透明但形状不完整，必须判为非导体
 *
 * 两个判据（形状完整 / 不透明）单独用都不对，这是本能力的核心价值。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/redstone/RedstoneHelper.hpp"
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

struct StateEntry {
    std::string name;
    const BlockState* state;
};

const std::vector<StateEntry>& states()
{
    static const std::vector<StateEntry> s_states = []() {
        std::vector<StateEntry> all;
        BlockRegistry::instance().forEachBlock([&all](Block& block) {
            for (const BlockState* state : block.defaultState().getAllStates()) {
                all.push_back({stateToString(*state), state});
            }
        });
        std::sort(all.begin(), all.end(), [](const StateEntry& a, const StateEntry& b) {
            return a.name < b.name;
        });
        all.erase(std::unique(all.begin(),
                      all.end(),
                      [](const StateEntry& a, const StateEntry& b) {
                          return a.name == b.name;
                      }),
            all.end());
        return all;
    }();
    return s_states;
}

/** 只放被测方块的极简世界。 */
class SingleBlockWorld final : public IWorld {
public:
    void reset(const BlockState* state)
    {
        m_state = state;
        m_random = math::Random(12345);
    }

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override
    {
        return m_state != nullptr ? m_state : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32, i32, i32, const BlockState*) override
    {
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

    void gameEvent(const gameevent::GameEvent&, const BlockPos&, const gameevent::GameEvent::Context&) override {}

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
        throw std::logic_error("block_redstone_conductor 场景不推进刻度");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("block_redstone_conductor 场景不推进刻度");
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
    probe::NullWorld m_world;
    const BlockState* m_state = nullptr;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

void writeList(FILE* out)
{
    ensureInitialized();
    for (const StateEntry& entry : states()) {
        std::fprintf(out, "%s\n", entry.name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    for (const StateEntry& entry : states()) {
        if (entry.name != unit) {
            continue;
        }
        SingleBlockWorld world;
        world.reset(entry.state);
        json result{{"unit", unit}};
        result["conductor"] =
            ::mc::world::redstone::RedstoneHelper::isRedstoneConductor(world, BlockPos(0, 64, 0), *entry.state);
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_redstone_conductor", &writeList, &runLine)

} // namespace mc::diff
