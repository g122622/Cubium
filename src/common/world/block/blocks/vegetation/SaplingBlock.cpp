#include "SaplingBlock.hpp"
#include "../../BlockRegistry.hpp"
#include "../../VanillaBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/math/random/Random.hpp"

#include <algorithm>

namespace mc {
namespace blocks {

namespace {

[[nodiscard]] bool isSaplingGround(const BlockState& groundState) {
    return (VanillaBlocks::GRASS_BLOCK != nullptr && groundState.is(VanillaBlocks::GRASS_BLOCK)) ||
           (VanillaBlocks::DIRT != nullptr && groundState.is(VanillaBlocks::DIRT)) ||
           (VanillaBlocks::COARSE_DIRT != nullptr && groundState.is(VanillaBlocks::COARSE_DIRT)) ||
           (VanillaBlocks::PODZOL != nullptr && groundState.is(VanillaBlocks::PODZOL)) ||
           (VanillaBlocks::FARMLAND != nullptr && groundState.is(VanillaBlocks::FARMLAND));
}

} // namespace

// ========== 构造函数 ==========

SaplingBlock::SaplingBlock(TreeGenerator treeGenerator, const BlockProperties& properties)
    : BushBlock(properties)
    , m_treeGenerator(std::move(treeGenerator)) {

    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::STAGE_0_1())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::STAGE_0_1(), 0));

    // 树苗形状：小型植物
    m_shape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
}

// ========== 状态属性 ==========

i32 SaplingBlock::getStage(const BlockState& state) const {
    return state.get(BlockStateProperties::STAGE_0_1());
}

const BlockState& SaplingBlock::withStage(i32 stage) const {
    return defaultState().with(BlockStateProperties::STAGE_0_1(), std::clamp(stage, 0, 1));
}

// ========== 放置逻辑 ==========

BlockState SaplingBlock::getStateForPlacement(BlockItemUseContext& context) {
    MC_UNUSED(context);
    return defaultState();
}

// ========== 生长逻辑 ==========

void SaplingBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    const BlockPos lightPos = pos.up();
    const i32 blockLight = static_cast<i32>(world.getBlockLight(lightPos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(lightPos));
    if (std::max(blockLight, skyLight) < 9) {
        return;
    }

    if (random.nextInt(7) != 0) {
        return;
    }

    grow(world, pos, state);
}

bool SaplingBlock::grow(IWorld& world, const BlockPos& pos, BlockState& state) {
    i32 stage = getStage(state);

    if (stage < 1) {
        const BlockState& nextState = withStage(stage + 1);
        world.setBlockState(pos, &nextState, 2);
        return true;
    }

    if (m_treeGenerator) {
        u64 seed = world.seed();
        seed ^= static_cast<u64>(static_cast<i64>(pos.x)) * 3129871ULL;
        seed ^= static_cast<u64>(static_cast<i64>(pos.y)) * 116129781ULL;
        seed ^= static_cast<u64>(static_cast<i64>(pos.z)) * 42317861ULL;

        math::Random rng(0);
        rng.setSeedWithHash(static_cast<i64>(seed));

        const BlockState* airState = BlockRegistry::instance().airState();
        world.setBlockState(pos, airState, 2);
        m_treeGenerator(world, pos, rng);
        return true;
    }

    return false;
}

// ========== 形状 ==========

const CollisionShape& SaplingBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

// ========== 保护方法 ==========

bool SaplingBlock::canSustain(
    const BlockState& groundState,
    IWorld& world,
    const BlockPos& groundPos) const {

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    return isSaplingGround(groundState);
}

} // namespace blocks
} // namespace mc
