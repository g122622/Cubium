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

#include "BlockState.hpp"

// TODO: include 路径使用了 ../.. 形式，违反代码规范。
// 应改为从 src/ 目录开始的绝对路径，如 "common/world/IWorld.hpp"。
// 但这需要同时修改所有依赖此头文件的源文件，暂不处理。
#include "../IWorld.hpp"
#include "../fluid/Fluid.hpp"
#include "../fluid/FluidRegistry.hpp"
#include "../fluid/fluids/EmptyFluid.hpp"
#include "Block.hpp"
#include "BlockPos.hpp"
#include "BlockSoundType.hpp"
#include "Material.hpp"
#include <algorithm>
#include <vector>

namespace mc {

// ============================================================================
// BlockState
// ============================================================================

BlockState::BlockState(const Block& block,
    std::vector<size_t> valueIndices,
    const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
    const std::vector<BlockState*>* allStates,
    u32 stateId)
    : StateHolder<Block, BlockState>(&block, std::move(valueIndices), propertyLayouts, allStates, stateId)
{
    _cacheProperties();
}

void BlockState::_cacheProperties()
{
    // 缓存方块属性
    m_isSolid = m_owner->isSolid(*this);
    m_isOpaque = m_owner->isOpaque(*this);
    m_blocksMovement = m_owner->material().blocksMovement();
    m_isLiquid = m_owner->material().isLiquid();
    m_isFlammable = m_owner->material().isFlammable();
    m_lightLevel = m_owner->lightLevel();
    // 通过虚函数计算缓存值，确保子类重写生效。
    m_opacity = m_owner->getOpacity(*this, nullptr, nullptr);
    m_propagatesSkylightDown = m_owner->propagatesSkylightDown(*this, nullptr, nullptr);
    m_useShapeForLightOcclusion = m_owner->useShapeForLightOcclusion(*this);
    m_hardness = m_owner->hardness();
    m_resistance = m_owner->resistance();
    m_blockId = m_owner->blockId();
    m_harvestTool = m_owner->harvestTool();
    m_harvestLevel = m_owner->harvestLevel();
    m_mapColor = m_owner->getMapColor(*this, nullptr, nullptr);
}

bool BlockState::isAir() const
{
    return m_owner->isAir(*this);
}

const CollisionShape& BlockState::getCollisionShape() const
{
    return m_owner->getCollisionShape(*this);
}

const CollisionShape& BlockState::getShape() const
{
    return m_owner->getShape(*this);
}

const CollisionShape& BlockState::getOcclusionShape() const
{
    return m_owner->getOcclusionShape(*this);
}

CollisionShape BlockState::getFaceOcclusionShape(Direction direction) const
{
    return m_owner->getFaceOcclusionShape(*this, direction);
}

bool BlockState::hasOpaqueCollisionShape() const
{
    // 如果方块不透明且有碰撞，则有不透明碰撞形状
    return m_isOpaque && m_owner->material().blocksMovement();
}

f32 BlockState::getAmbientOcclusionLightValue() const
{
    // 如果方块有不透明碰撞形状，返回0.2（产生阴影）
    // 否则返回1.0（透明方块如玻璃、树叶不产生阴影）
    return hasOpaqueCollisionShape() ? 0.2f : 1.0f;
}

bool BlockState::isSolidSide(IWorld& world, const BlockPos& pos, Direction side) const
{
    return m_owner->isSolidSide(*this, world, pos, side);
}

bool BlockState::isOpaqueCube(IWorld& world, const BlockPos& pos) const
{
    // 如果方块是固体的且有不透明碰撞形状，则为不透明完整方块
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return m_isSolid && m_isOpaque && hasOpaqueCollisionShape();
}

const ResourceLocation& BlockState::blockLocation() const
{
    return m_owner->blockLocation();
}

const fluid::FluidState* BlockState::getFluidState() const
{
    return m_owner->getFluidState(*this);
}

const Material& BlockState::getMaterial() const
{
    return m_owner->material();
}

const BlockSoundType& BlockState::getSoundType() const
{
    return m_owner->getSoundType();
}

std::string BlockState::toModelKey() const
{
    if (m_valueIndices.empty()) {
        return "";
    }

    // 按属性名排序，确保模型键稳定且与资源系统缓存键一致
    // 直接拼接字符串，避免 ostringstream 的格式化和分配开销。
    std::vector<std::pair<const IProperty*, size_t>> sortedValues;
    const auto& layouts = propertyLayouts();
    sortedValues.reserve(layouts.size());
    for (size_t i = 0; i < layouts.size(); ++i) {
        sortedValues.emplace_back(layouts[i].property, m_valueIndices[i]);
    }

    std::sort(sortedValues.begin(), sortedValues.end(), [](const auto& a, const auto& b) {
        return a.first->name() < b.first->name();
    });

    std::string result;
    result.reserve(sortedValues.size() * 16);
    bool first = true;
    for (const auto& [prop, valueIndex] : sortedValues) {
        if (!first) {
            result.push_back(',');
        }
        result += prop->name();
        result.push_back('=');
        result += prop->valueToString(valueIndex);
        first = false;
    }
    return result;
}

std::string BlockState::ownerName() const
{
    return m_owner->toString();
}

u8 BlockState::getHarvestTool() const
{
    return m_harvestTool;
}

i32 BlockState::getHarvestLevel() const
{
    return m_harvestLevel;
}

world::map::MaterialColorId BlockState::getMapColor(IWorld* world, const BlockPos* pos) const
{
    return m_owner->getMapColor(*this, world, pos);
}

bool BlockState::isToolEffective(u8 toolType, i32 harvestLevel) const
{
    // 检查工具类型是否匹配
    if (m_harvestTool != toolType) {
        return false;
    }
    // 检查工具等级是否足够
    return harvestLevel >= m_harvestLevel;
}

bool BlockState::requiresTool() const
{
    return m_owner->requiresTool();
}

bool BlockState::isStickyBlock() const
{
    return m_owner->isStickyBlock(*this);
}

bool BlockState::canStickTo(const BlockState& other) const
{
    return m_owner->canStickTo(*this, other);
}

// ============================================================================
// 火焰相关
// ============================================================================

i32 BlockState::getFlammability(IWorld* world, const BlockPos* pos, Direction face) const
{
    return m_owner->getFlammability(*this, world, pos, face);
}

i32 BlockState::getFireSpreadSpeed(IWorld* world, const BlockPos* pos, Direction face) const
{
    return m_owner->getFireSpreadSpeed(*this, world, pos, face);
}

bool BlockState::isFireSource(IWorld& world, const BlockPos& pos, Direction side) const
{
    return m_owner->isFireSource(*this, world, pos, side);
}

void BlockState::catchFire(IWorld& world, const BlockPos& pos, Direction face, Entity* igniter) const
{
    m_owner->catchFire(*this, world, pos, face, igniter);
}

} // namespace mc
