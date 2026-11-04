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
 * @brief 差分能力：讲台**放书/取书**时的状态复位与游戏事件（`LecternBlock#resetBookState`）。
 *
 * `list`：操作前的 `HAS_BOOK` × `POWERED` × 目标 `hasBook`
 * `run` ：一行 -> { "hasBook": 操作后, "powered": 操作后, "gameEvent": 事件 id }
 *
 * ## 讲台的两条状态
 *
 * - `HAS_BOOK`：讲台上有没有书
 * - `POWERED`：比较器脉冲是否还在
 *
 * 放书或取书时原版走 `resetBookState`：**`POWERED` 一律复位为 false**，
 * `HAS_BOOK` 置为目标值，然后发 `BLOCK_CHANGE`。
 *
 * ## 为什么 POWERED 必须复位
 *
 * 讲台翻页时会拉高一次 `POWERED` 当作比较器脉冲（翻页信号的来源）。
 * 如果放书/取书时不清掉这个脉冲：
 *
 * - 取走书之后讲台仍「报告翻过页」，下游比较器一直读到一个不该存在的信号
 * - 再放一本新书时，脉冲状态会延续下来，比较器分不清新旧
 *
 * ## 抄错的后果
 *
 * 状态不复位：取书后红石输出卡在非零值，玩家得手动再翻一次页才能清掉。
 * 事件漏发：讲台在幽匿电路里隐形，放书取书都感知不到。
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
#include "common/world/block/blocks/functional/LecternBlock.hpp"
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

/** 讲台所在世界：持有方块状态拷贝、记录事件。 */
class LecternSceneWorld final : public IWorld {
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
        // 持有拷贝：调用方传入的多半是栈上状态，存指针会在返回后悬垂。
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
        throw std::logic_error("lectern_book 场景不推进刻度：放书取书不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("lectern_book 场景不推进刻度：放书取书不应调度计划刻");
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
    for (const char* hasBook : {"false", "true"}) {
        for (const char* powered : {"false", "true"}) {
            for (const char* target : {"false", "true"}) {
                std::fprintf(out, "%s:%s:%s\n", hasBook, powered, target);
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    // 单元形如 "<hasBook>:<powered>:<target>"
    const std::size_t p1 = unit.find(':');
    const std::size_t p2 = unit.find(':', p1 + 1);
    if (p1 == std::string::npos || p2 == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const bool initiallyHasBook = unit.substr(0, p1) == "true";
    const bool initiallyPowered = unit.substr(p1 + 1, p2 - p1 - 1) == "true";
    const bool targetHasBook = unit.substr(p2 + 1) == "true";

    Block* lectern = block_registry::BuildingBlocks::LECTERN;
    if (lectern == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const BlockState state = lectern->defaultState()
                                 .with(BlockStateProperties::HAS_BOOK(), initiallyHasBook)
                                 .with(BlockStateProperties::POWERED(), initiallyPowered);

    LecternSceneWorld world;
    world.reset();
    world.setBlockState(0, 64, 0, &state);

    blocks::LecternBlock::setHasBook(world, BlockPos(0, 64, 0), targetHasBook);

    const BlockState* after = world.currentState();
    if (after == nullptr || after->isAir()) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    json result{{"unit", unit}};
    result["hasBook"] = after->get(BlockStateProperties::HAS_BOOK());
    result["powered"] = after->get(BlockStateProperties::POWERED());
    result["gameEvents"] = world.events();
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(lectern_book, &writeList, &runLine)

} // namespace mc::diff
