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
 * @brief 差分能力：幽匿感测体被踩踏时是否发出 STEP 振动（`SculkSensorBlock#stepOn`）。
 *
 * `list`：相位 × 踩踏者 × 是否潜行
 * `run` ：一行 -> { "stepEvent": 是否发出 STEP }
 *
 * ## 三个前置条件
 *
 * 感测体不只靠监听周围事件，自己被踩也会产生一次 STEP 振动。三个条件缺一不可：
 *
 * 1. **相位为 Inactive** —— 活跃/冷却期间不重复接收，否则踩上去会持续刷新
 * 2. **踩踏者不是监守者** —— 监守者与感测体同源，踩上去不该自激
 * 3. **踩踏者不在潜行** —— 潜行不产生振动，这是「悄悄绕过感测体」的玩法基础
 *
 * ## 抄错的后果
 *
 * - 漏掉相位检查：站上去不走也会一直触发，感测体变成压力板
 * - 漏掉潜行检查：潜行绕过感测体这个核心玩法直接失效
 * - 漏掉监守者检查：监守者路过自己的感测体会自激
 * - 发错事件（发 BLOCK_CHANGE 而非 STEP）：依赖脚步声的装置分辨不出来
 */

#include <cstdio>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/blocks/sculk/SculkSensorBlock.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/SculkBlocks.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
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

constexpr const char* PHASES[] = {"inactive", "active", "cooldown"};
constexpr const char* WALKERS[] = {"normal", "sneaking", "warden"};

/** 感测体所在世界：记录 gameEvent。 */
class SensorSceneWorld final : public IWorld {
public:
    void reset()
    {
        m_events.clear();
        m_state.reset();
        m_random = math::Random(12345);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override
    {
        return m_state.has_value() ? &m_state.value() : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32, i32, i32, const BlockState* state) override
    {
        if (state == nullptr) {
            m_state.reset();
        } else {
            m_state = *state;
        }
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
        throw std::logic_error("sculk_sensor_step 场景不推进刻度：踩踏本身不该调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("sculk_sensor_step 场景不推进刻度：踩踏本身不该调度计划刻");
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
    std::optional<BlockState> m_state;
    std::vector<std::string> m_events;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

void writeList(FILE* out)
{
    ensureInitialized();
    for (const char* phase : PHASES) {
        for (const char* walker : WALKERS) {
            std::fprintf(out, "%s:%s\n", phase, walker);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t sep = unit.find(':');
    if (sep == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string phaseLabel = unit.substr(0, sep);
    const std::string walkerLabel = unit.substr(sep + 1);

    BlockStateProperties::SculkSensorPhase phase;
    if (phaseLabel == "inactive") {
        phase = BlockStateProperties::SculkSensorPhase::Inactive;
    } else if (phaseLabel == "active") {
        phase = BlockStateProperties::SculkSensorPhase::Active;
    } else if (phaseLabel == "cooldown") {
        phase = BlockStateProperties::SculkSensorPhase::Cooldown;
    } else {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    if (walkerLabel != "normal" && walkerLabel != "sneaking" && walkerLabel != "warden") {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    Block* sensor = block_registry::SculkBlocks::SCULK_SENSOR;
    if (sensor == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const BlockState state = sensor->defaultState()
                                 .with(BlockStateProperties::SCULK_SENSOR_PHASE(), phase)
                                 .with(BlockStateProperties::POWER_0_15(), 0);

    SensorSceneWorld world;
    world.reset();
    world.setBlockState(0, 64, 0, &state);

    Player walker(EntityInstanceId(0), "diff_walker", *world.entityRegistry());
    if (walkerLabel == "sneaking") {
        walker.setSneaking(true);
    }

    // 监守者是独立实体类。踩踏判定只关心 getTypeId()，所以用 setTypeId 把玩家的
    // 类型标识改成 warden 即可走到真实分支，不必构造完整 WardenEntity。
    // TODO: 等 World 提供更轻的实体构造入口后，改为真正的 WardenEntity。
    if (walkerLabel == "warden") {
        walker.setTypeId("minecraft:warden");
    }

    const auto& sensorBlock = static_cast<const blocks::SculkSensorBlock&>(*sensor);
    sensorBlock.onEntityWalk(state, world, BlockPos(0, 64, 0), walker);

    json result{{"unit", unit}};
    bool hasStep = false;
    for (const auto& event : world.events()) {
        if (event == "minecraft:step") {
            hasStep = true;
            break;
        }
    }
    result["stepEvent"] = hasStep;
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(sculk_sensor_step, &writeList, &runLine)

} // namespace mc::diff
