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
 * @brief 差分能力：绊线钩状态变化时的音效与游戏事件（`TripWireHookBlock#emitState`）。
 *
 * `list`：变化前 powered/attached × 变化后 powered/attached = 16 个单元
 * `run` ：一行 -> { "sound": 音效 id | null, "volume": 音量, "gameEvent": 事件 id | null }
 *
 * ## 四个分支
 *
 * 绊线钩有两种独立的状态，各对应一组音效 + 事件：
 *
 * | 变化 | 音效 | 音高 | 事件 |
 * |---|---|---|---|
 * | 未通电 -> 通电 | `block.tripwire.click_on` | 0.6 | `block_activate` |
 * | 通电 -> 未通电 | `block.tripwire.click_off` | 0.5 | `block_deactivate` |
 * | 未挂上 -> 挂上 | `block.tripwire.attach` | 0.7 | `block_attach` |
 * | 挂上 -> 未挂 | `block.tripwire.detach` | `1.2/(random*0.2+0.9)` | `block_detach` |
 * | 都不变 | 不发声 | — | 不发事件 |
 *
 * ## 最关键的一条：优先级
 *
 * 分支是 **if / else if**，不是并列的四个 if。通电/断电**优先于**挂上/脱开 ——
 * 两者同时变化时只报电量那次。
 *
 * 写成并列四个 if 的后果：一次性扯断整条绊线时，钩子既断电又脱开，会**连发两个事件
 * 和两个音效**，幽匿感测体被连续唤醒两次，红石计时类装置会算错 tick。
 *
 * 另外「都不变」必须**完全静默**：状态没变就发声是绊线钩最常见的错误，会让每个
 * 方块更新都响一声。
 */

#include <bit>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/blocks/redstone/TripWireHookBlock.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/RedstoneBlocks.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include "vanilla_datapack.hpp"

#include "probe.hpp"

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

/** 绊线钩所在世界：只记录 playSound 与 gameEvent，随机数固定种子保证确定性。 */
class HookSceneWorld final : public IWorld {
public:
    void reset()
    {
        m_sound.clear();
        m_volume = 0.0f;
        m_gameEvent.clear();
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

    void playSound(const ResourceLocation& soundEvent,
        sound::SoundCategory /*category*/,
        const Vector3& /*pos*/,
        f32 volume,
        f32 /*pitch*/) override
    {
        m_sound = soundEvent.toString();
        m_volume = volume;
    }

    void gameEvent(const gameevent::GameEvent& event,
        const BlockPos& /*pos*/,
        const gameevent::GameEvent::Context& /*context*/) override
    {
        // Cubium 的事件 id 是裸路径，原版是完整标识符；补前缀对齐表示法。
        m_gameEvent = std::string("minecraft:") + event.id();
    }

    [[nodiscard]] const std::string& sound() const noexcept
    {
        return m_sound;
    }

    [[nodiscard]] f32 volume() const noexcept
    {
        return m_volume;
    }

    [[nodiscard]] const std::string& gameEvent() const noexcept
    {
        return m_gameEvent;
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
        throw std::logic_error("tripwire_state 场景不推进刻度：状态发声不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("tripwire_state 场景不推进刻度：状态发声不应调度计划刻");
    }

private:
    std::string m_sound;
    std::string m_gameEvent;
    f32 m_volume = 0.0f;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

void writeList(FILE* out)
{
    ensureInitialized();
    // 单元形如 "<wasPowered>:<wasAttached>:<powered>:<attached>"
    for (const char* wasPowered : {"false", "true"}) {
        for (const char* wasAttached : {"false", "true"}) {
            for (const char* powered : {"false", "true"}) {
                for (const char* attached : {"false", "true"}) {
                    std::fprintf(out, "%s:%s:%s:%s\n", wasPowered, wasAttached, powered, attached);
                }
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        const std::size_t sep = unit.find(':', start);
        if (sep == std::string::npos) {
            parts.push_back(unit.substr(start));
            break;
        }
        parts.push_back(unit.substr(start, sep - start));
        start = sep + 1;
    }
    if (parts.size() != 4) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const bool wasPowered = parts[0] == "true";
    const bool wasAttached = parts[1] == "true";
    const bool powered = parts[2] == "true";
    const bool attached = parts[3] == "true";

    Block* hook = block_registry::RedstoneBlocks::TRIPWIRE_HOOK;
    if (hook == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const BlockState state = hook->defaultState()
                                 .with(BlockStateProperties::POWERED(), powered)
                                 .with(BlockStateProperties::ATTACHED(), attached);

    HookSceneWorld world;
    world.reset();

    blocks::TripWireHookBlock::emitState(world, BlockPos(0, 64, 0), state, powered, attached, wasPowered, wasAttached);

    json result{{"unit", unit}};
    result["sound"] = world.sound().empty() ? json(nullptr) : json(world.sound());
    // 音量是 f32，必须按位模式输出：Gson 会把它展开成 double（0.4 -> 0.4000000059604645），
    // 直接比十进制会产生大量伪差异。
    result["volume"] = std::bit_cast<u32>(world.volume());
    result["gameEvent"] = world.gameEvent().empty() ? json(nullptr) : json(world.gameEvent());
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(tripwire_state, &writeList, &runLine)

} // namespace mc::diff
