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
 * @brief 差分能力：可疑沙/沙砾坠落碎开时的效果与事件（`BrushableBlock#onBrokenAfterFall`）。
 *
 * `list`：方块种类（可疑沙 / 可疑沙砾）
 * `run` ：一行 -> { "block": 方块 id, "levelEvents": [...], "gameEvents": [...] }
 *
 * ## 两个动作，缺一不可
 *
 * 可疑沙/沙砾属于会下落的方块。落下来摔碎时：
 *
 * 1. 放 **方块破碎粒子**（`levelEvent(2001)`）
 * 2. 发 **`block_destroy`** 游戏事件
 *
 * 只发粒子不发事件，坠落破碎在幽匿电路里就是隐形的；
 * 只发事件不放粒子，玩家看不到「摔碎了」的视觉反馈。
 *
 * 原版 `FallingBlockEntity` 有 `onBroken` 与 `onBrokenAfterFall` 两个回调，
 * 前者是外部摧毁，后者是落地碎开。Cubium 把两者合并成了 `onBroken`，
 * 按调用点（`_handleLanding`）判断它等价于原版的 `onBrokenAfterFall`。
 */

#include <cmath>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/blocks/functional/BrushableBlock.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/TrailsBlocks.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/effect/base/LevelEventId.hpp"
#include "common/world/gameevent/GameEvent.hpp"
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

constexpr const char* BLOCKS[] = {"suspicious_sand", "suspicious_gravel"};

/** 碎开场景世界：记录 playEvent 与 gameEvent。 */
class BreakSceneWorld final : public IWorld {
public:
    void reset()
    {
        m_events.clear();
        m_levelEvents.clear();
        m_random = math::Random(12345);
    }

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override
    {
        return &VanillaBlocks::AIR->defaultState();
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

    void playEvent(::mc::world::LevelEventId eventId, const BlockPos& /*pos*/, i32 data) override
    {
        // 只记录事件 id：data 是方块状态的全局 id（原版用 Block.getId），
        // 跨实现不可比，本能力不比对它。
        MC_UNUSED(data);
        m_levelEvents.push_back(static_cast<i32>(eventId));
    }

    void gameEvent(const gameevent::GameEvent& event,
        const BlockPos& /*pos*/,
        const gameevent::GameEvent::Context& /*context*/) override
    {
        // Cubium 的事件 id 是裸路径，原版是完整标识符；补前缀对齐表示法。
        m_events.push_back(std::string("minecraft:") + event.id());
    }

    [[nodiscard]] const std::vector<std::string>& events() const noexcept
    {
        return m_events;
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
        throw std::logic_error("brushable_fall 场景不推进刻度：碎开本身不该调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("brushable_fall 场景不推进刻度：碎开本身不该调度计划刻");
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
    std::vector<std::string> m_events;
    std::vector<i32> m_levelEvents;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

void writeList(FILE* out)
{
    ensureInitialized();
    for (const char* name : BLOCKS) {
        std::fprintf(out, "%s\n", name);
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    Block* brushable = nullptr;
    if (unit == "suspicious_sand") {
        brushable = block_registry::TrailsBlocks::SUSPICIOUS_SAND;
    } else if (unit == "suspicious_gravel") {
        brushable = block_registry::TrailsBlocks::SUSPICIOUS_GRAVEL;
    } else {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    if (brushable == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    BreakSceneWorld world;
    world.reset();

    entity::FallingBlockEntity falling(*world.entityRegistry());
    falling.setFallingState(&brushable->defaultState());
    // 落点故意取非整数坐标，确认取整逻辑没被写死成 0。
    falling.setPosition(3.5, 62.0, -7.5);

    auto& brushableBlock = static_cast<blocks::BrushableBlock&>(*brushable);
    brushableBlock.onBroken(world, BlockPos(3, 62, -7), falling);

    json result{{"unit", unit}};
    result["block"] = brushable->blockLocation().toString();
    result["levelEvents"] = world.levelEvents();
    result["gameEvents"] = world.events();
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(brushable_fall, &writeList, &runLine)

} // namespace mc::diff
