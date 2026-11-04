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
 * @brief 差分能力：用剪刀拆绊线时的**解除武装**与游戏事件（`TripWireBlock#playerWillDestroy`）。
 *
 * `list`：主手物品（空手 / 剪刀 / 木棍）
 * `run` ：一行 -> { "disarmed": 破坏前是否被置为已解除, "gameEvents": [...] }
 *
 * ## 为什么「拆」也分两种
 *
 * 绊线有两种离开方式：
 *
 * - **被扯断 / 被踩触发**：正常的触发链，两端钩子走 BLOCK_DEACTIVATE
 * - **用剪刀拆掉**：玩家想**安静地**解除陷阱，不该触发
 *
 * 实现方式是：剪断时先把绊线置成 `DISARMED = true`，再移除方块。这样两端钩子在
 * 重新计算状态时看到的是「已解除」的绊线，走拆除路径而不是触发路径。
 *
 * ## 事件
 *
 * 剪断发 `shear`（和剪羊毛、剪蜂巢、剪南瓜同一类）。
 *
 * ## 抄错的后果
 *
 * 不置 `DISARMED`：玩家用剪刀拆陷阱反而把它**引爆一次** —— 这正好是剪刀存在的
 * 意义（安全拆除）的反面。
 *
 * 漏发事件：剪断在幽匿电路里隐形。
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
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/TripWireBlock.hpp"
#include "common/world/block/registry/RedstoneBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
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

constexpr const char* HELD_LABELS[] = {"empty", "shears", "stick"};
constexpr const char* HELD_IDS[] = {"", "minecraft:shears", "minecraft:stick"};

/** 绊线所在世界：记录 setBlockState 的最终状态与 gameEvent。 */
class WireSceneWorld final : public IWorld {
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
        throw std::logic_error("tripwire_disarm 场景不推进刻度：拆除不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("tripwire_disarm 场景不推进刻度：拆除不应调度计划刻");
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
    for (const char* label : HELD_LABELS) {
        std::fprintf(out, "%s\n", label);
    }
}

const Item* findItem(std::string_view id)
{
    if (id.empty()) {
        return nullptr;
    }
    return ItemRegistry::instance().getItem(ResourceLocation(std::string(id)));
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const char* heldId = nullptr;
    for (std::size_t i = 0; i < std::size(HELD_LABELS); ++i) {
        if (unit == HELD_LABELS[i]) {
            heldId = HELD_IDS[i];
            break;
        }
    }
    if (heldId == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    Block* wire = block_registry::RedstoneBlocks::TRIPWIRE;
    if (wire == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const BlockState state = wire->defaultState();

    WireSceneWorld world;
    world.reset();
    world.setBlockState(0, 64, 0, &state);

    Player player(EntityInstanceId(0), "diff_wire", *world.entityRegistry());
    const Item* held = findItem(heldId);
    if (held != nullptr) {
        player.setMainHandItem(ItemStack(*held, 1));
    }

    auto& wireBlock = static_cast<blocks::TripWireBlock&>(*wire);
    wireBlock.playerWillDestroy(world, BlockPos(0, 64, 0), state, player);

    // 是否被置成 DISARMED：看最后一次写入的状态
    const BlockState* after = world.currentState();
    const bool disarmed = after != nullptr && !after->isAir() && after->get(BlockStateProperties::DISARMED());

    json result{{"unit", unit}};
    result["disarmed"] = disarmed;
    result["gameEvents"] = world.events();
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("tripwire_disarm", &writeList, &runLine)

} // namespace mc::diff
