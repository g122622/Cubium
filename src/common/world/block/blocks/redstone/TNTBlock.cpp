#include "TNTBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../IWorld.hpp"
#include "../../../../util/property/Properties.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

TNTBlock::TNTBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::UNSTABLE())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::UNSTABLE(), false));
}

bool TNTBlock::isUnstable(const BlockState& state) {
    return state.get(BlockStateProperties::UNSTABLE());
}

void TNTBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查是否有红石信号或火焰
    bool hasPower = world::redstone::RedstonePower::isPowered(world, pos);
    bool hasFire = hasFlammableNeighbor(world, pos);

    if (hasPower || hasFire) {
        ignite(world, pos, state);
    }
}

void TNTBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state) {
        return;
    }

    // 检查是否有红石信号
    bool hasPower = world::redstone::RedstonePower::isPowered(world, pos);

    if (hasPower) {
        ignite(world, pos, *state);
        return;
    }

    // 检查是否有火焰或熔岩
    bool hasFire = hasFlammableNeighbor(world, pos);
    if (hasFire) {
        ignite(world, pos, *state);
    }
}

void TNTBlock::ignite(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(state);

    // 移除TNT方块
    world.setBlockState(pos.x, pos.y, pos.z, nullptr, 11);

    // 生成点燃的TNT实体
    // TODO: 生成 PrimedTntEntity
    // world.spawnEntity(std::make_unique<PrimedTntEntity>(world, pos, ...));

    // 播放点燃音效
    // world.playSound(pos, SoundEvents::ENTITY_TNT_PRIMED, 1.0f, 1.0f);

    MC_UNUSED(world);
    MC_UNUSED(pos);
}

void TNTBlock::explode(IWorld& world, const BlockPos& pos, f32 power) {
    // 移除TNT方块
    world.setBlockState(pos.x, pos.y, pos.z, nullptr, 11);

    // 创建爆炸
    // TODO: 实现爆炸系统
    // world.createExplosion(pos, power, ExplosionMode::BreakBlocks);

    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(power);
}

bool TNTBlock::hasFlammableNeighbor(IWorld& world, const BlockPos& pos) const {
    // 检查六个方向是否有火焰或熔岩
    for (Direction dir : {Direction::North, Direction::East, Direction::South,
                          Direction::West, Direction::Up, Direction::Down}) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);

        if (neighborState) {
            // TODO: 检查是否是火焰或熔岩
            // if (neighborState->is(Blocks::FIRE) ||
            //     neighborState->is(Blocks::LAVA)) {
            //     return true;
            // }
            MC_UNUSED(neighborState);
        }
    }

    return false;
}

} // namespace blocks
} // namespace mc
