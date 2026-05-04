#include "DetectorRailBlock.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../util/AxisAlignedBB.hpp"

namespace mc {
namespace blocks {

DetectorRailBlock::DetectorRailBlock(const BlockProperties& properties)
    : AbstractRailBlock(properties, true)
{
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(SHAPE())
        .add(POWERED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(SHAPE(), RailShape::NorthSouth)
        .with(POWERED(), false));
}

void DetectorRailBlock::fillStateContainer(StateContainer<Block, BlockState>& container) {
    // 状态容器在构造函数中创建，此方法留空
    MC_UNUSED(container);
}

void DetectorRailBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(random);

    // MC 1.16.5: 检测矿车并更新状态
    bool shouldBePowered = false;

    // 创建检测区域（铁轨上方一格高度）
    AxisAlignedBB searchBox = AxisAlignedBB::fromBlock(pos.x, pos.y, pos.z);

    // 获取区域内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(searchBox, nullptr);

    // 检查是否有矿车
    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        // 检查是否为矿车类型
        // 使用 LegacyEntityType::Minecart 检测
        if (entity->legacyType() == LegacyEntityType::Minecart) {
            shouldBePowered = true;
            break;
        }
    }

    bool isCurrentlyPowered = isPowered(state);
    if (shouldBePowered != isCurrentlyPowered) {
        // 更新状态 - 修改传入的state引用
        state = state.with(POWERED(), shouldBePowered);
        world.setBlockState(pos.x, pos.y, pos.z, &state, 3);

        // MC 1.16.5: 通知相邻方块更新
        world.updateNeighbors(pos, *this);
    }
}

i32 DetectorRailBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    if (isPowered(state)) {
        return 15;
    }
    return 0;
}

i32 DetectorRailBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // MC 1.16.5: 探测铁轨只向上输出强信号
    if (isPowered(state) && side == Direction::Up) {
        return 15;
    }
    return 0;
}

RailShape DetectorRailBlock::getRailShape(const BlockState& state) const {
    return state.get(SHAPE());
}

BlockState DetectorRailBlock::withRailShape(const BlockState& state, RailShape shape) const {
    return state.with(SHAPE(), shape);
}

bool DetectorRailBlock::isPowered(const BlockState& state) {
    return state.get(POWERED());
}

} // namespace blocks
} // namespace mc
