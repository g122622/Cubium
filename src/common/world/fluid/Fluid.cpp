#include "Fluid.hpp"
#include "FluidRegistry.hpp"
#include "FluidTags.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"
#include "../block/BlockPos.hpp"
#include "../../physics/collision/CollisionShape.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/property/FluidProperties.hpp"
#include <sstream>

namespace mc {
namespace fluid {

// ============================================================================
// FluidState 实现
// ============================================================================

FluidState::FluidState(const Fluid& fluid,
                       std::unordered_map<const IProperty*, size_t> values,
                       u32 stateId)
    : StateHolder<Fluid, FluidState>(&fluid, std::move(values), stateId) {
    m_fluidId = fluid.fluidId();
}

bool FluidState::isSource() const {
    return m_owner->isSource(*this);
}

i32 FluidState::getLevel() const {
    return m_owner->getLevel(*this);
}

bool FluidState::isFalling() const {
    auto& falling = FluidProperties::FALLING();
    auto opt = getOptional(falling);
    return opt.has_value() && opt.value();
}

f32 FluidState::getHeight() const {
    i32 level = getLevel();
    return static_cast<f32>(level) / 9.0f;
}

f32 FluidState::getActualHeight(IWorld& world, const BlockPos& pos) const {
    // 检查上方是否有同种流体（满高度情况）
    BlockPos above = pos.up();
    const FluidState* aboveFluid = world.getFluidState(above);

    if (aboveFluid != nullptr &&
        !aboveFluid->isEmpty() &&
        aboveFluid->getFluid().isEquivalentTo(*m_owner)) {
        // 上方有同种流体，返回满高度
        return 1.0f;
    }

    // 返回基础高度
    return getHeight();
}

bool FluidState::isEmpty() const {
    return m_owner->isEmpty();
}

const Fluid& FluidState::getFluid() const {
    return *m_owner;
}

const BlockState* FluidState::getBlockState() const {
    if (m_cachedBlockState == nullptr) {
        m_cachedBlockState = m_owner->getBlockState(*this);
    }
    return m_cachedBlockState;
}

Vector3 FluidState::getFlow(IBlockReader& world, const BlockPos& pos) const {
    return m_owner->getFlow(world, pos, *this);
}

bool FluidState::canDisplace(IWorld& world, const BlockPos& pos,
                              const Fluid& fluid, Direction dir) const {
    return m_owner->canDisplace(*this, world, pos, fluid, dir);
}

std::string FluidState::ownerName() const {
    return m_owner->toString();
}

// ============================================================================
// Fluid 实现
// ============================================================================

Fluid* Fluid::getFluid(u32 fluidId) {
    return FluidRegistry::instance().getFluid(fluidId);
}

Fluid* Fluid::getFluid(const ResourceLocation& id) {
    return FluidRegistry::instance().getFluid(id);
}

FluidState* Fluid::getFluidState(u32 stateId) {
    return FluidRegistry::instance().getFluidState(stateId);
}

void Fluid::forEachFluid(std::function<void(Fluid&)> callback) {
    FluidRegistry::instance().forEachFluid(callback);
}

void Fluid::forEachFluidState(std::function<void(const FluidState&)> callback) {
    FluidRegistry::instance().forEachFluidState(callback);
}

void Fluid::createFluidState(std::unique_ptr<StateContainer<Fluid, FluidState>> container) {
    m_stateContainer = std::move(container);
}

void Fluid::setDefaultState(const FluidState& state) {
    m_defaultState = &state;
}

Vector3 Fluid::getFlow(IBlockReader& world, const BlockPos& pos, const FluidState& state) const {
    // 默认实现：无流动
    (void)world;
    (void)pos;
    (void)state;
    return Vector3(0.0f, 0.0f, 0.0f);
}

void Fluid::tick(IWorld& world, const BlockPos& pos, FluidState& state) {
    // 默认实现：无操作
    (void)world;
    (void)pos;
    (void)state;
}

void Fluid::randomTick(IWorld& world, const BlockPos& pos,
                        const FluidState& state, math::IRandom& random) {
    // 默认实现：无操作
    (void)world;
    (void)pos;
    (void)state;
    (void)random;
}

bool Fluid::canDisplace(const FluidState& state, IWorld& world,
                         const BlockPos& pos, const Fluid& fluid, Direction dir) const {
    // 默认实现：如果新流体相同，则不可替换
    (void)state;
    (void)world;
    (void)pos;
    (void)dir;
    return !isEquivalentTo(fluid);
}

CollisionShape Fluid::getShape(const FluidState& state, IBlockReader& world,
                                const BlockPos& pos) const {
    // 默认实现：根据高度返回形状
    // TODO: 实现实际的碰撞形状
    (void)state;
    (void)world;
    (void)pos;
    return CollisionShape::empty();
}

bool Fluid::isIn(const FluidTag& tag) const {
    return tag.contains(*this);
}

} // namespace fluid
} // namespace mc
