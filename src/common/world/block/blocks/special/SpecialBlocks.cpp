#include "SpecialBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../../entity/Player.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../core/BlockRaycastResult.hpp"

namespace mc {
namespace blocks {

// ========== BarrierBlock ==========

BarrierBlock::BarrierBlock(const BlockProperties& properties)
    : Block(properties) {
    // 屏障没有状态属性
}

const CollisionShape& BarrierBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

// ========== StructureVoidBlock ==========

StructureVoidBlock::StructureVoidBlock(const BlockProperties& properties)
    : Block(properties) {
    // 结构空位没有状态属性
}

const CollisionShape& StructureVoidBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 结构空位有一个小的可见轮廓
    static CollisionShape shape = CollisionShape::box(0.375f, 0.375f, 0.375f, 0.625f, 0.625f, 0.625f);
    return shape;
}

const CollisionShape& StructureVoidBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== StructureBlock ==========

StructureBlock::StructureBlock(const BlockProperties& properties)
    : Block(properties) {
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
}

StructureBlock::Mode StructureBlock::getMode(const BlockState& state) const {
    MC_UNUSED(state);
    // TODO: 实现 MODE 属性
    return Mode::Save;
}

ActionResult StructureBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // TODO: 打开结构方块界面
    return ActionResult::Success;
}

// ========== JigsawBlock ==========

JigsawBlock::JigsawBlock(const BlockProperties& properties)
    : Block(properties) {
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
}

ActionResult JigsawBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // TODO: 打开拼图方块界面
    return ActionResult::Success;
}

// ========== CommandBlock ==========

CommandBlock::CommandBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING())
        .add(BlockStateProperties::CONDITIONAL())
        .add(BlockStateProperties::POWERED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING(), Direction::North)
        .with(BlockStateProperties::CONDITIONAL(), false)
        .with(BlockStateProperties::POWERED(), false));
}

Direction CommandBlock::getFacing(const BlockState& state) const {
    return state.get(BlockStateProperties::FACING());
}

bool CommandBlock::isConditional(const BlockState& state) const {
    return state.get(BlockStateProperties::CONDITIONAL());
}

bool CommandBlock::isPowered(const BlockState& state) const {
    return state.get(BlockStateProperties::POWERED());
}

BlockState CommandBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = Directions::opposite(context.getClickedFace());
    return defaultState().with(BlockStateProperties::FACING(), facing);
}

void CommandBlock::neighborChanged(
    IWorld& world,
    const BlockPos& pos,
    Block& neighborBlock,
    const BlockPos& neighborPos,
    bool isMoving) {

    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // TODO: 检测红石信号并触发命令
}

i32 CommandBlock::getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 命令方块不输出信号
    return 0;
}

const BlockState& CommandBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const BlockState& CommandBlock::mirror(const BlockState& state, Mirror mirror) const {
    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

ActionResult CommandBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // TODO: 打开命令方块界面
    return ActionResult::Success;
}

void CommandBlock::execute(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // TODO: 执行命令
}

// ========== RepeatingCommandBlock ==========

RepeatingCommandBlock::RepeatingCommandBlock(const BlockProperties& properties)
    : CommandBlock(properties) {
}

void RepeatingCommandBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 每个 tick 执行命令
    execute(world, pos, state);
}

// ========== ChainCommandBlock ==========

ChainCommandBlock::ChainCommandBlock(const BlockProperties& properties)
    : CommandBlock(properties) {
}

// ========== SlimeBlock ==========

SlimeBlock::SlimeBlock(const BlockProperties& properties)
    : Block(properties) {
}

void SlimeBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 弹跳效果
    // entity.setVelocity(entity.getVelocity().x, 1.0, entity.getVelocity().z);
}

Material::PushReaction SlimeBlock::getPushReaction(const BlockState& state) const {
    MC_UNUSED(state);
    return Material::PushReaction::Normal;
}

// ========== HoneyBlock ==========

HoneyBlock::HoneyBlock(const BlockProperties& properties)
    : Block(properties) {

    // 蜂蜜块碰撞箱稍小
    m_collisionShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.9375f, 1.0f);
}

void HoneyBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 减速效果
    // entity.setVelocity(entity.getVelocity().x * 0.4, entity.getVelocity().y, entity.getVelocity().z * 0.4);
}

Material::PushReaction HoneyBlock::getPushReaction(const BlockState& state) const {
    MC_UNUSED(state);
    return Material::PushReaction::Normal;
}

const CollisionShape& HoneyBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_collisionShape;
}

// ========== SpongeBlock ==========

SpongeBlock::SpongeBlock(const BlockProperties& properties)
    : Block(properties) {
}

bool SpongeBlock::tryAbsorbWater(IWorld& world, const BlockPos& pos) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 实现吸水逻辑
    return false;
}

// ========== WetSpongeBlock ==========

WetSpongeBlock::WetSpongeBlock(const BlockProperties& properties)
    : Block(properties) {
}

} // namespace blocks
} // namespace mc
