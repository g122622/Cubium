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
 * @brief 差分能力：拨动拉杆时**发出的游戏事件**与音效（`LeverBlock#play`）。
 *
 * `list`：拉杆初始 POWERED × 拨动后 POWERED
 * `run` ：一行 -> { "sound": 音效 id, "gameEvent": 事件 id }
 *
 * ## 为什么单独测「发事件」
 *
 * 游戏事件（`GameEvent`）是幽匿感测体、尖啸体、监守者、悦灵这些「听声音」机制的唯一输入。
 * 方块交互除了改状态、放音效，还必须发一个对应的事件，否则：
 *
 * - 幽匿感测体收不到振动，红石信号不会出现
 * - 依赖它的监守者召唤、悦灵拾取等机制全部失效
 *
 * 拉杆拨动的规则是最简单的一种：**开启发 `block_activate`、关闭发 `block_deactivate`**。
 *
 * ## 抄错的后果
 *
 * 事件发错或漏发**不会崩溃、不会掉日志**，方块本身的状态和音效都正常，只有红石侦测端
 * 完全没反应。这类缺陷只能靠逐方块驱动一遍、把实际发出的事件记录出来比对。
 */

#include <cstdio>
#include <map>
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
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/LeverBlock.hpp"
#include "common/world/block/registry/RedstoneBlocks.hpp"
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

/**
 * @brief 充当拉杆所在的世界：记录被驱动的方块状态、播放的音效与发出的游戏事件。
 *
 * 只实现「拨动拉杆」会用到的部分：读写方块状态、放音效、发游戏事件。邻居更新在场景里
 * 没有消费者，留空。
 */
class LeverSceneWorld final : public IWorld {
public:
    void setBlock(const BlockState* state)
    {
        m_state = state;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override
    {
        return m_state;
    }

    bool setBlockState(i32, i32, i32, const BlockState* state) override
    {
        m_state = state;
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
        f32 /*volume*/,
        f32 /*pitch*/) override
    {
        m_sound = soundEvent.toString();
    }

    void gameEvent(const gameevent::GameEvent& event,
        const BlockPos& /*pos*/,
        const gameevent::GameEvent::Context& /*context*/) override
    {
        // Cubium 的事件 id 是裸路径（"block_activate"），原版是完整标识符
        // （"minecraft:block_activate"）；补上前缀，让两侧用同一种表示法比对。
        m_gameEvent = std::string("minecraft:") + event.id();
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
        throw std::logic_error("lever_game_event 场景不推进刻度：拨动拉杆不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("lever_game_event 场景不推进刻度：拨动拉杆不应调度计划刻");
    }

    [[nodiscard]] const std::string& sound() const noexcept
    {
        return m_sound;
    }

    [[nodiscard]] const std::string& gameEvent() const noexcept
    {
        return m_gameEvent;
    }

private:
    const BlockState* m_state = nullptr;
    std::string m_sound;
    std::string m_gameEvent;
    math::Random m_random{0};
    world::border::WorldBorder m_worldBorder;
};

void writeList(FILE* out)
{
    ensureInitialized();
    // 拨动一定翻转状态，所以只需枚举初始 POWERED
    for (const char* powered : {"false", "true"}) {
        std::fprintf(out, "%s\n", powered);
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const bool initiallyPowered = unit == "true";

    Block* lever = block_registry::RedstoneBlocks::LEVER;
    if (lever == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const BlockState state = lever->defaultState().with(BlockStateProperties::POWERED(), initiallyPowered);

    LeverSceneWorld world;
    world.setBlock(&state);
    blocks::LeverBlock::toggle(world, BlockPos(0, 64, 0), state);

    json result{{"unit", unit}};
    result["sound"] = world.sound().empty() ? json(nullptr) : json(world.sound());
    result["gameEvent"] = world.gameEvent().empty() ? json(nullptr) : json(world.gameEvent());
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(lever_game_event, &writeList, &runLine)

} // namespace mc::diff
