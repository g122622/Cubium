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
 * @brief 差分能力：钟被放置时推导出的**附着方式**（`BellBlock#getStateForPlacement`）。
 *
 * `list`：点击面 × 水平朝向 × 四邻坚固性 的组合
 * `run` ：一行 -> { "facing": 朝向, "attachment": 附着方式 } 或 { "rejected": true }
 *
 * ## 钟的四种挂法
 *
 * 钟是少数「同一个方块按摆放方式长出不同形状」的方块：
 *
 * - **floor**：点顶面，钟立在地上（碰撞箱是 16×16×8 的底座）
 * - **ceiling**：点底面，钟吊在天花板下
 * - **single_wall**：点侧面且只有一侧是墙，钟单面挂在墙上
 * - **double_wall**：点侧面且**对向两侧都是墙**，钟跨在两面墙之间
 *
 * 四种的碰撞形状、音效衰减、被敲击时的共振范围都不同，所以附着方式必须先算对。
 *
 * ## 判定规则
 *
 * 点击面在 Y 轴上时：顶面 -> floor、底面 -> ceiling，朝向取玩家的水平朝向。
 * 点击面在水平轴上时：朝向取**点击面的反方向**（钟面朝外）；
 * 再检查点击轴**对向的两侧**是否都有朝内的固体面 —— 都有就是 double_wall，
 * 否则 single_wall。
 *
 * ## 抄错的后果
 *
 * - 双墙写成单墙：两面墙之间的钟只挂在一侧，视觉上缺半边，碰撞箱也少一半；
 * - 朝向用点击面本身而不是反方向：钟面朝墙里，玩家看到的是钟的背面；
 * - 顶面/底面颠倒：钟要么吊在半空要么陷进地里，且 `canSurvive` 判定随之出错。
 *
 * 这些错误都不会崩溃，只会让钟「看起来不对」，把钟作为红石计时器或装饰的
 * 建筑里尤其明显。
 */

#include <algorithm>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/border/WorldBorder.hpp"
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

/// 钟七种点击面（含 None 之外的全部方向）与四种水平朝向
constexpr Direction FACES[] = {
    Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
constexpr const char* FACE_NAMES[] = {"down", "up", "north", "south", "west", "east"};
/// 水平朝向（玩家面朝方向）
constexpr Direction YAWS[] = {Direction::North, Direction::South, Direction::West, Direction::East};
constexpr const char* YAW_NAMES[] = {"north", "south", "west", "east"};

/**
 * @brief 只读的方块场：四邻坚固性由单元里显式指定的位掩码决定。
 *
 * 位含义：1=西侧有朝东的固体面、2=东侧有朝西的、4=北侧有朝南的、8=南侧有朝北的。
 * 只有 `isFaceSturdy` 与 `getBlockState` 会被钟的放置逻辑用到。
 */
class BellSceneWorld final : public IWorld {
public:
    void setSturdyMask(u8 mask)
    {
        m_mask = mask;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        // 传入的是**绝对坐标**，换算成相对放置位 (0,64,0) 的偏移
        const BlockPos offset(x - 0, y - 64, z - 0);
        const bool sturdy = _isSturdyAt(offset);
        return sturdy ? &VanillaBlocks::STONE->defaultState() : &VanillaBlocks::AIR->defaultState();
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
        throw std::logic_error("bell_placement 场景不推进刻度");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("bell_placement 场景不推进刻度");
    }

private:
    /// 相对放置位（0,0,0）的偏移是否为坚固方块
    [[nodiscard]] bool _isSturdyAt(const BlockPos& offset) const noexcept
    {
        if (offset.x == -1 && offset.y == 0 && offset.z == 0) {
            return (m_mask & 1U) != 0;
        }
        if (offset.x == 1 && offset.y == 0 && offset.z == 0) {
            return (m_mask & 2U) != 0;
        }
        if (offset.x == 0 && offset.y == 0 && offset.z == -1) {
            return (m_mask & 4U) != 0;
        }
        if (offset.x == 0 && offset.y == 0 && offset.z == 1) {
            return (m_mask & 8U) != 0;
        }
        // 放置位下方恒为坚固方块（钟立在地上总要有地面），上方恒为空气 ——
        // 否则 FLOOR / CEILING 两种挂法都会因 canSurvive 失败而回退成默认状态。
        if (offset.x == 0 && offset.y == -1 && offset.z == 0) {
            return true;
        }
        return false;
    }

    u8 m_mask = 0;
    math::Random m_random{0};
    world::border::WorldBorder m_worldBorder;
};

const char* directionName(Direction direction)
{
    switch (direction) {
        case Direction::Down:
            return "down";
        case Direction::Up:
            return "up";
        case Direction::North:
            return "north";
        case Direction::South:
            return "south";
        case Direction::West:
            return "west";
        case Direction::East:
            return "east";
        case Direction::None:
            break;
    }
    return "none";
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const Direction face : FACES) {
        for (const Direction yaw : YAWS) {
            // 四邻坚固性只对水平点击面有意义（Y 轴点击不看侧面），但仍全量枚举以确认无副作用
            for (u8 mask = 0; mask < 16; ++mask) {
                std::fprintf(out, "%s:%s:%d\n", directionName(face), directionName(yaw), static_cast<i32>(mask));
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    // 单元形如 "<face>:<yaw>:<mask>"
    const std::size_t first = unit.find(':');
    const std::size_t second = unit.find(':', first + 1);
    if (first == std::string::npos || second == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string faceName = unit.substr(0, first);
    const std::string yawName = unit.substr(first + 1, second - first - 1);
    const i32 mask = std::stoi(unit.substr(second + 1));

    Direction face = Direction::None;
    for (std::size_t i = 0; i < std::size(FACES); ++i) {
        if (faceName == FACE_NAMES[i]) {
            face = FACES[i];
            break;
        }
    }
    Direction yaw = Direction::None;
    for (std::size_t i = 0; i < std::size(YAWS); ++i) {
        if (yawName == YAW_NAMES[i]) {
            yaw = YAWS[i];
            break;
        }
    }
    if (face == Direction::None || yaw == Direction::None || mask < 0 || mask > 15) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    Block* bell = VanillaBlocks::BELL;
    if (bell == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    BellSceneWorld world;
    world.setSturdyMask(static_cast<u8>(mask));

    // 玩家站在放置位附近、水平朝向由 yaw 决定
    const BlockPos placementPos(0, 64, 0);
    const f32 yawDegrees = yaw == Direction::South ? 0.0f
        : yaw == Direction::West                   ? 90.0f
        : yaw == Direction::North                  ? 180.0f
                                                   : -90.0f;

    // 点击点取方块在该面上的中心
    const Vector3 hitPos(0.5f + static_cast<f32>(Directions::xOffset(face)),
        64.5f + static_cast<f32>(Directions::yOffset(face)),
        0.5f + static_cast<f32>(Directions::zOffset(face)));

    ItemStack stack;
    BlockItemUseContext context(world, nullptr, stack, hitPos, placementPos, face, yawDegrees, 0.0f);

    // 原版在四种挂法都不可存活时 `getStateForPlacement` 返回 null（放置取消、不消耗物品）。
    // Cubium 用 `canPlaceAt` 表达这条语义，因此这里先问它、再取状态。
    if (!bell->canPlaceAt(context)) {
        return json{{"unit", unit}, {"rejected", true}}.dump();
    }

    const BlockState result = bell->getStateForPlacement(context);
    if (result.isAir() || &result.getBlock() != bell) {
        return json{{"unit", unit}, {"rejected", true}}.dump();
    }

    const Direction facing = result.get(BlockStateProperties::HORIZONTAL_FACING());
    const auto attachment = result.get(BlockStateProperties::BELL_ATTACHMENT());

    const char* attachmentName = "unknown";
    switch (attachment) {
        case BlockStateProperties::BellAttachment::Floor:
            attachmentName = "floor";
            break;
        case BlockStateProperties::BellAttachment::Ceiling:
            attachmentName = "ceiling";
            break;
        case BlockStateProperties::BellAttachment::SingleWall:
            attachmentName = "single_wall";
            break;
        case BlockStateProperties::BellAttachment::DoubleWall:
            attachmentName = "double_wall";
            break;
    }

    return json{{"unit", unit}, {"facing", directionName(facing)}, {"attachment", attachmentName}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(bell_placement, &writeList, &runLine)

} // namespace mc::diff
