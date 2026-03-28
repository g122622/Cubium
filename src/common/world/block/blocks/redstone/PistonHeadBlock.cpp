#include "PistonHeadBlock.hpp"
#include "../../../IWorld.hpp"
#include <unordered_map>

namespace mc {

// EnumProperty Traits 实现 - 必须在 mc 命名空间
String EnumProperty<blocks::PistonHeadBlock::Type>::Traits::toString(const blocks::PistonHeadBlock::Type& value) {
    switch (value) {
        case blocks::PistonHeadBlock::Type::Normal: return "normal";
        case blocks::PistonHeadBlock::Type::Sticky: return "sticky";
        default: return "normal";
    }
}

Optional<blocks::PistonHeadBlock::Type> EnumProperty<blocks::PistonHeadBlock::Type>::Traits::fromName(StringView name) {
    if (name == "normal") return blocks::PistonHeadBlock::Type::Normal;
    if (name == "sticky") return blocks::PistonHeadBlock::Type::Sticky;
    return std::nullopt;
}

namespace blocks {

// 活塞头类型属性 - 使用静态函数返回引用
const EnumProperty<PistonHeadBlock::Type>& TYPE_PROP() {
    static auto prop = EnumProperty<PistonHeadBlock::Type>::create("type", {
        PistonHeadBlock::Type::Normal,
        PistonHeadBlock::Type::Sticky
    });
    return *prop;
}

// 静态方法实现 - 返回类型属性
const EnumProperty<PistonHeadBlock::Type>& PistonHeadBlock::getTypeProperty() {
    return TYPE_PROP();
}

PistonHeadBlock::PistonHeadBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING())
        .add(TYPE_PROP())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING(), Direction::North)
        .with(TYPE_PROP(), Type::Normal));
}

Direction PistonHeadBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::FACING());
}

PistonHeadBlock::Type PistonHeadBlock::getType(const BlockState& state) {
    return state.get(TYPE_PROP());
}

BlockState PistonHeadBlock::withType(BlockState state, Type type) {
    return state.with(TYPE_PROP(), type);
}

BlockState PistonHeadBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facingState);
    MC_UNUSED(currentPos);

    // 检查活塞主体是否还存在
    Direction pistonFacing = getFacing(state);
    BlockPos pistonPos = currentPos.offset(Directions::opposite(pistonFacing));

    const BlockState* pistonState = world.getBlockState(pistonPos.x, pistonPos.y, pistonPos.z);
    if (!pistonState || pistonState->isAir()) {
        // 活塞主体不存在，活塞头应该消失
        // 返回空气状态（通过设置 null）
        return state;
    }

    MC_UNUSED(facingPos);
    return state;
}

} // namespace blocks
} // namespace mc
