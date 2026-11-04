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
 * @brief 差分能力：吃一片蛋糕时发出的**游戏事件**（`CakeBlock#eat`）。
 *
 * `list`：吃之前的 BITES（0..6）
 * `run` ：一行 -> { "gameEvents": [...], "bitesAfter": 吃后片数 | null }
 *
 * ## 规则
 *
 * 吃蛋糕发**两种**事件，且是叠加的：
 *
 * - 每一片都发 `eat` —— 幽匿感测体据此分辨「有人吃东西」，而不只是「方块变了」
 * - 吃的是**最后一片**（BITES 已是 6）时，额外发 `block_destroy` —— 蛋糕整块消失了
 *
 * 也就是说 BITES=6 会连发两个事件，其余只发一个。这个「叠加」很容易漏：
 * 只写 EAT 会让最后一片消失时幽匿电路少一次 destroy；只写 DESTROY 则前六片
 * 完全静默。
 *
 * ## 抄错的后果
 *
 * 事件不对不会崩溃、不掉日志，蛋糕的片数变化和音效都正常，只有红石侦测端
 * 的行为跟着偏 —— 依赖「吃蛋糕」触发的装置（例如隐藏在餐桌里的幽匿机关）
 * 会少触发或多触发。
 *
 * 另外 `bitesAfter` 记录吃完后的片数：`null` 表示蛋糕已被移除。
 */

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
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/functional/CakeBlock.hpp"
#include "common/world/block/registry/BuildingBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
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

/** 蛋糕所在世界：记录每次 setBlockState 与 gameEvent。 */
class CakeSceneWorld final : public IWorld {
public:
    void reset()
    {
        m_events.clear();
        m_state.reset();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override
    {
        return m_state.has_value() ? &m_state.value() : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32, i32, i32, const BlockState* state) override
    {
        // 持有拷贝而非指针：调用方传入的多半是栈上的临时状态，存指针会在函数返回后悬垂。
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

    [[nodiscard]] const BlockState* currentState() const noexcept
    {
        return m_state.has_value() ? &m_state.value() : nullptr;
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
        throw std::logic_error("cake_eat 场景不推进刻度：吃蛋糕不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("cake_eat 场景不推进刻度：吃蛋糕不应调度计划刻");
    }

private:
    std::optional<BlockState> m_state;
    std::vector<std::string> m_events;
    math::Random m_random{0};
    world::border::WorldBorder m_worldBorder;
};

void writeList(FILE* out)
{
    ensureInitialized();
    // BITES 0..6，共 7 档；6 是最后一片
    for (i32 bites = 0; bites <= 6; ++bites) {
        std::fprintf(out, "%d\n", bites);
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const i32 bites = std::stoi(unit);
    if (bites < 0 || bites > 6) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    Block* cake = block_registry::BuildingBlocks::CAKE;
    if (cake == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const BlockState state = cake->defaultState().with(BlockStateProperties::BITES_0_6(), bites);

    CakeSceneWorld world;
    world.reset();
    world.setBlockState(0, 64, 0, &state);

    const bool ok = blocks::CakeBlock::eatSlice(world, BlockPos(0, 64, 0), state);
    if (!ok) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    json result{{"unit", unit}};
    result["gameEvents"] = world.events();

    // 吃完后的片数：蛋糕被移除时为 null
    const BlockState* after = world.currentState();
    if (after == nullptr || after->isAir()) {
        result["bitesAfter"] = nullptr;
    } else {
        result["bitesAfter"] = blocks::CakeBlock::getBites(*after);
    }
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("cake_eat", &writeList, &runLine)

} // namespace mc::diff
