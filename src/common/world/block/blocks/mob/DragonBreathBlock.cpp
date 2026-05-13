#include "DragonBreathBlock.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

DragonBreathBlock::DragonBreathBlock(const BlockProperties& properties)
    : Block(properties) {
}

void DragonBreathBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);
    // TODO: 造成伤害
}

const CollisionShape& DragonBreathBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

const CollisionShape& DragonBreathBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
