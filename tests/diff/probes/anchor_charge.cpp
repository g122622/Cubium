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
 * @brief 差分能力：重生锚充能/放电时的层数变化与游戏事件（`RespawnAnchorBlock#charge`）。
 *
 * `list`：CHARGES 0..4 × 动作（charge / discharge）
 * `run` ：一行 -> { "chargesAfter": 变化后层数, "gameEvents": [...] }
 *
 * ## 规则
 *
 * 重生锚有四层充能，用萤石加满后可以设置重生点，每次死亡消耗一层：
 *
 * - **充能**：层数 +1，最多到 4；已满 4 层时**不变、不发事件**
 * - **放电**：层数 -1，最少到 0；已空 0 层时**不变、不发事件**
 *
 * 两种变化都发 `block_change`。
 *
 * ## 抄错的后果
 *
 * 漏发事件：充能/掉层在幽匿电路里隐形。
 * 已满还发事件：每次往满锚上点萤石都会惊动侦测器一次，萤石也不该被消耗。
 * 已达上限还继续加：层数溢出到 5，光照等级（charges*3.75）算出 18，超出亮度上限。
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
#include "common/world/block/blocks/functional/RespawnAnchorBlock.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
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

constexpr const char* ACTIONS[] = {"charge", "discharge"};

/** 重生锚所在世界：持有方块状态拷贝、记录事件。 */
class AnchorSceneWorld final : public IWorld {
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
        throw std::logic_error("anchor_charge 场景不推进刻度：充放电不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("anchor_charge 场景不推进刻度：充放电不应调度计划刻");
    }

private:
    std::optional<BlockState> m_state;
    std::vector<std::string> m_events;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

void writeList(FILE* out)
{
    ensureInitialized();
    for (i32 charges = 0; charges <= 4; ++charges) {
        for (const char* action : ACTIONS) {
            std::fprintf(out, "%d:%s\n", charges, action);
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
    const i32 charges = std::stoi(unit.substr(0, sep));
    const std::string action = unit.substr(sep + 1);
    if (charges < 0 || charges > 4 || (action != "charge" && action != "discharge")) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    Block* anchor = block_registry::NetherBlocks::RESPAWN_ANCHOR;
    if (anchor == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const BlockState state = anchor->defaultState().with(BlockStateProperties::CHARGES_0_4(), charges);

    AnchorSceneWorld world;
    world.reset();
    world.setBlockState(0, 64, 0, &state);

    BlockState current = state;
    if (action == "charge") {
        blocks::RespawnAnchorBlock::charge(world, BlockPos(0, 64, 0), current);
    } else {
        blocks::RespawnAnchorBlock::discharge(world, BlockPos(0, 64, 0), current);
    }

    const BlockState* after = world.currentState();
    if (after == nullptr || after->isAir()) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    json result{{"unit", unit}};
    result["chargesAfter"] = after->get(BlockStateProperties::CHARGES_0_4());
    result["gameEvents"] = world.events();
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(anchor_charge, &writeList, &runLine)

} // namespace mc::diff
