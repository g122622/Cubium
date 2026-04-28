#include "BedBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../dimension/DimensionType.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

// ========== BedBlock 实现 ==========

BedBlock::BedBlock(u32 color, const BlockProperties& properties)
    : Block(properties)
    , m_color(color) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::BED_PART())
        .add(BlockStateProperties::OCCUPIED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot)
        .with(BlockStateProperties::OCCUPIED(), false));

    // 预计算各朝向的形状
    // 床的形状：主体 + 四个床腿
    constexpr f32 P = 1.0f / 16.0f;

    // 主体形状 (高度9像素，从Y=3开始)
    CollisionShape baseShape = CollisionShape::box(0.0f, 3.0f * P, 0.0f, 16.0f * P, 9.0f * P, 16.0f * P);

    // 床腿形状
    CollisionShape legNW = CollisionShape::box(0.0f, 0.0f, 0.0f, 3.0f * P, 3.0f * P, 3.0f * P);
    CollisionShape legNE = CollisionShape::box(13.0f * P, 0.0f, 0.0f, 16.0f * P, 3.0f * P, 3.0f * P);
    CollisionShape legSW = CollisionShape::box(0.0f, 0.0f, 13.0f * P, 3.0f * P, 3.0f * P, 16.0f * P);
    CollisionShape legSE = CollisionShape::box(13.0f * P, 0.0f, 13.0f * P, 16.0f * P, 3.0f * P, 16.0f * P);

    // 北朝向形状 (头部在南)
    m_shapesByFacing[static_cast<size_t>(Direction::North)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::combine(baseShape, legNW),
                legNE
            ),
            CollisionShape::combine(legSW, legSE)
        );

    // 南朝向形状 (头部在北)
    m_shapesByFacing[static_cast<size_t>(Direction::South)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::combine(baseShape, legNW),
                legNE
            ),
            CollisionShape::combine(legSW, legSE)
        );

    // 西朝向形状 (头部在东)
    m_shapesByFacing[static_cast<size_t>(Direction::West)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::combine(baseShape, legNW),
                legNE
            ),
            CollisionShape::combine(legSW, legSE)
        );

    // 东朝向形状 (头部在西)
    m_shapesByFacing[static_cast<size_t>(Direction::East)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::combine(baseShape, legNW),
                legNE
            ),
            CollisionShape::combine(legSW, legSE)
        );
}

BlockState BedBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.horizontalDirection();
    BlockPos pos = context.blockPos();
    BlockPos headPos(pos.x + Directions::xOffset(facing), pos.y, pos.z + Directions::zOffset(facing));

    // 检查头部位置是否可放置
    const BlockState* headState = context.getWorld().getBlockState(headPos);
    if (headState != nullptr && headState->isAir()) {
        return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }

    // 无法放置完整的床
    return defaultState();
}

BlockState BedBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());
    Direction bedFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 计算另一部分的方向
    Direction otherDir = (part == BlockStateProperties::BedPart::Foot) ? bedFacing : Directions::opposite(bedFacing);

    if (facing == otherDir) {
        // 另一半被移除
        if (facingState.isAir()) {
            return world.getBlockState(currentPos)->getBlock().defaultState();
        }
        // 同步占用状态
        if (facingState.hasProperty(BlockStateProperties::OCCUPIED())) {
            return state.with(BlockStateProperties::OCCUPIED(), facingState.get(BlockStateProperties::OCCUPIED()));
        }
    }

    return state;
}

const CollisionShape& BedBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = static_cast<size_t>(facing);
    MC_ASSERT(index < Directions::COUNT && Directions::isHorizontal(facing));
    return m_shapesByFacing[index];
}

void BedBlock::setOccupied(IWorld& world, const BlockPos& pos, BlockState& state, bool occupied) {
    if (state.hasProperty(BlockStateProperties::OCCUPIED())) {
        world.setBlockState(pos, &state.with(BlockStateProperties::OCCUPIED(), occupied), 2);
    }
}

bool BedBlock::isBed(IWorld& world, const BlockPos& pos) {
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    // 检查是否为床方块
    // TODO: 添加方块类型检查
    return state->hasProperty(BlockStateProperties::BED_PART());
}

ActionResultType BedBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // MC Java: 床的交互逻辑
    // 1. 检查维度 - 在下界和末地床会爆炸
    // 2. 在主世界检查时间 - 只能在夜间睡眠

    // 获取维度信息
    DimensionId dimId = world.dimension();
    DimensionType dimType = (dimId == 0) ? DimensionType::overworld() :
                            (dimId == 1) ? DimensionType::nether() :
                            DimensionType::theEnd();

    // 检查床是否可用（主世界可用，下界和末地会爆炸）
    if (!dimType.bedWorks()) {
        // 在下界或末地使用床会爆炸
        // 移除床方块
        world.setBlockState(pos, nullptr, 11);

        // 检查是否为床的头部，如果是脚部则同时移除头部
        BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());
        if (part == BlockStateProperties::BedPart::Foot) {
            Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
            BlockPos headPos = pos.offset(facing);
            const BlockState* headState = world.getBlockState(headPos);
            if (headState && headState->hasProperty(BlockStateProperties::BED_PART()) &&
                headState->get(BlockStateProperties::BED_PART()) == BlockStateProperties::BedPart::Head) {
                world.setBlockState(headPos, nullptr, 11);
            }
        }

        // TODO: 创建爆炸
        // world.createExplosion(pos, 5.0f, true, Explosion::Mode::DESTROY);

        // 播放爆炸音效
        world.playSound(
            ResourceLocation("minecraft:entity.generic.explode"),
            sound::SoundCategory::Blocks,
            pos.center(),
            4.0f,
            1.0f
        );

        return ActionResultType::Success;
    }

    // 在主世界检查是否被占用
    if (state.get(BlockStateProperties::OCCUPIED())) {
        // 床已被占用
        return ActionResultType::Pass;
    }

    // 检查时间 - 只能在夜间睡眠
    // MC Java: 夜间范围是 12541-23458
    i64 currentTime = world.dayTime();
    bool isNight = (currentTime >= 12541 && currentTime <= 23458);

    // TODO: 还需要检查是否有怪物在床附近
    // if (!isNight && !player.isCreative()) {
    //     // 显示消息：你只能在夜间或雷暴时睡眠
    //     return ActionResultType::Pass;
    // }

    // 设置玩家的重生点
    // TODO: player.setSpawnPoint(pos, true, dimId);

    // 播放睡眠音效
    world.playSound(
        ResourceLocation("minecraft:block.bed.use"),
        sound::SoundCategory::Blocks,
        pos.center(),
        1.0f,
        1.0f
    );

    // 标记床为占用状态
    BlockState newState = state.with(BlockStateProperties::OCCUPIED(), true);
    world.setBlockState(pos, &newState, 2);

    // 如果是脚部，也要标记头部
    BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());
    if (part == BlockStateProperties::BedPart::Foot) {
        Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
        BlockPos headPos = pos.offset(facing);
        const BlockState* headState = world.getBlockState(headPos);
        if (headState && headState->hasProperty(BlockStateProperties::BED_PART())) {
            BlockState newHeadState = headState->with(BlockStateProperties::OCCUPIED(), true);
            world.setBlockState(headPos, &newHeadState, 2);
        }
    }

    // TODO: 让玩家进入睡眠状态
    // player.sleep(pos);

    return ActionResultType::Success;
}

} // namespace blocks
} // namespace mc
