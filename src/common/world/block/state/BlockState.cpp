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

#include "common/world/block/state/BlockState.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/sound/BlockSoundType.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/support/SupportType.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/map/MaterialColor.hpp"
#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// BlockState
// ============================================================================

BlockState::BlockState(const Block& block,
    StateValueIndices valueIndices,
    size_t propertyCount,
    const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
    const std::vector<BlockState*>* allStates,
    u32 stateId)
    : StateHolder<Block, BlockState>(&block, valueIndices, propertyCount, propertyLayouts, allStates, stateId)
{
    _cacheProperties();
}

void BlockState::_cacheProperties()
{
    // 缓存方块属性
    m_isAir = m_owner->isAir(*this);
    m_isSolid = m_owner->isSolid(*this);
    m_isOpaque = m_owner->isOpaque(*this);
    // canOcclude 由注册属性决定，状态构造期即为最终值（与方块子类构造顺序无关）。
    m_canOccludeCache = m_owner->canOcclude();
    m_blocksMovement = m_owner->material().blocksMovement();
    m_isLiquid = m_owner->material().isLiquid();
    m_isFlammable = m_owner->material().isFlammable();
    m_canBeReplaced = m_owner->isReplaceable();
    m_lightLevel = m_owner->lightLevel();
    // 通过虚函数计算缓存值，确保子类重写生效。
    m_opacity = m_owner->getOpacity(*this, nullptr, nullptr);
    m_propagatesSkylightDown = m_owner->propagatesSkylightDown(*this, nullptr, nullptr);
    m_useShapeForLightOcclusion = m_owner->useShapeForLightOcclusion(*this);
    m_hardness = m_owner->hardness();
    m_resistance = m_owner->resistance();
    m_blockId = m_owner->blockId();
    // 地图颜色在注册时按**本状态**求值并缓存：默认实现等价于取方块自身的静态颜色，
    // 但保留按状态求值的入口，原版快照表可按状态字符串给出不同结果。
    // toString() 依赖属性表，此刻已就绪。
    m_mapColor = m_owner->getMapColor(*this, nullptr, nullptr);
}

const CollisionShape& BlockState::getCollisionShape() const
{
    return m_owner->getCollisionShape(*this);
}

const CollisionShape& BlockState::getCollisionShapeForEntity(const EntityCollisionContext& ctx, i32 blockY) const
{
    return m_owner->getCollisionShapeForEntity(*this, ctx, blockY);
}

const CollisionShape& BlockState::getShape() const
{
    return m_owner->getShape(*this);
}

CollisionShape BlockState::getShapeAt(const BlockPos& pos) const
{
    return m_owner->getShapeAt(*this, pos);
}

CollisionShape BlockState::getCollisionShapeAt(const BlockPos& pos) const
{
    return m_owner->getCollisionShapeAt(*this, pos);
}

CollisionShape BlockState::getBlockSupportShapeAt(const BlockPos& pos) const
{
    return m_owner->getBlockSupportShapeAt(*this, pos);
}

const std::vector<const BlockState*>& BlockState::getAllStates() const
{
    // 索引表与 BlockState 同生命周期且构造后不变，故首次转换后缓存复用，
    // 避免每次调用都重建 vector。
    static thread_local std::unordered_map<const std::vector<BlockState*>*, std::vector<const BlockState*>> cache;
    const auto& slots = allStateSlots();
    auto iter = cache.find(&slots);
    if (iter == cache.end()) {
        std::vector<const BlockState*> view;
        view.reserve(slots.size());
        for (const BlockState* state : slots) {
            view.push_back(state);
        }
        iter = cache.emplace(&slots, std::move(view)).first;
    }
    return iter->second;
}

bool BlockState::isRandomlyTicking() const
{
    // 对齐 vanilla BlockBehaviour.BlockStateBase#isRandomlyTicking：带状态的判断，
    // 随机刻调度与存档的 tick 标志都必须用它，而不是方块级的 Block::ticksRandomly()。
    return m_owner->isRandomlyTicking(*this);
}

const CollisionShape& BlockState::getOcclusionShape() const
{
    return m_owner->getOcclusionShape(*this);
}

const CollisionShape& BlockState::getBlockSupportShape() const
{
    return m_owner->getBlockSupportShape(*this);
}

CollisionShape BlockState::getFaceOcclusionShape(Direction direction) const
{
    // 对齐 vanilla BlockStateBase：occlusionShape = canOcclude ? getOcclusionShape() : Shapes.empty()，
    // 再取面投影。noOcclusion 方块不提供任何遮挡面，光照引擎的形状遮挡检测对它整体失效。
    if (!m_canOccludeCache) {
        return CollisionShape::empty();
    }
    return m_owner->getFaceOcclusionShape(*this, direction);
}

void BlockState::refreshLightCache()
{
    m_opacity = m_owner->getOpacity(*this, nullptr, nullptr);
    m_propagatesSkylightDown = m_owner->propagatesSkylightDown(*this, nullptr, nullptr);
    // useShapeForLightOcclusion 与另外两个一样是虚函数结果，必须在方块构造完成后再取一次：
    // 状态构造期（StateContainer 建表）里取到的是基类默认值 false，子类重写要等到方块对象
    // 完全构造之后才生效。早期这里漏了这一行，凡是在子类里重写该方法做形状遮挡的方块
    // （土径、耕地、讲台、切石机等）运行时都退回成了默认值。
    m_useShapeForLightOcclusion = m_owner->useShapeForLightOcclusion(*this);
}

bool BlockState::isSolid() const
{
    // 实时委托给所属方块：isSolid 由碰撞形状推断，而许多方块（门、台阶、栅栏门）的形状
    // 要到子类构造函数里才设置好，构造期缓存会拿到空形状并永久判成 false。
    return m_owner->isSolid(*this);
}

bool BlockState::isSolidRender() const
{
    // vanilla BlockStateBase：#501 occlusionShape = canOcclude ? owner.getOcclusionShape() : Shapes.empty()，
    // #502 solidRender = Block.isShapeFullBlock(occlusionShape)。
    if (!m_canOccludeCache) {
        return false;
    }
    // 用 coversFullBlock 而不是 isFullBlock：前者按形状覆盖整格做判定，等价于 vanilla
    // isShapeFullBlock（joinIsNotEmpty + NOT_SAME）；后者只认「类型上就是完整立方体」，
    // 对由多个方盒拼出的整格形状会漏判。
    return m_owner->getOcclusionShape(*this).coversFullBlock();
}

bool BlockState::hasOpaqueCollisionShape() const
{
    // 如果方块不透明且有碰撞，则有不透明碰撞形状。
    // noOcclusion 方块（铁栏杆、树叶、门等）不产生遮挡，也就没有不透明碰撞形状
    // （vanilla 的 AO 与遮挡判定均以 canOcclude 为前提）。
    if (!m_canOccludeCache) {
        return false;
    }
    return m_isOpaque && m_owner->material().blocksMovement();
}

f32 BlockState::getAmbientOcclusionLightValue() const
{
    // 委托到方块的 getShadeBrightness 虚方法，
    // 子类可重写以实现特殊遮光行为（如 MudBlock 碰撞形状不完整但仍需阴影）。
    return getShadeBrightness();
}

f32 BlockState::getShadeBrightness() const
{
    return m_owner->getShadeBrightness(*this);
}

bool BlockState::isSolidSide(IWorld& world, const BlockPos& pos, Direction side) const
{
    return m_owner->isSolidSide(*this, world, pos, side);
}

bool BlockState::isFaceFull(Direction direction) const
{
    return Block::isFaceFull(getCollisionShape(), direction);
}

bool BlockState::isFaceSturdy(
    IWorld& world, const BlockPos& pos, Direction direction, const SupportType& supportType) const
{
    // 原版 `isFaceSturdy` 取 `getBlockSupportShape(level, pos)`，其默认实现转调带位置、
    // 空碰撞上下文的 `getCollisionShape(..., pos, ...)`；带 `offsetType` 的方块（竹子、
    // 花、滴水石锥）会在其中叠加坐标哈希偏移。若走无位置版本，偏移被丢弃，中心柱
    // 判定随之翻转。
    return supportType.isSupportingAt(*this, world, pos, direction);
}

bool BlockState::isOpaqueCube(IWorld& world, const BlockPos& pos) const
{
    // 如果方块是固体的且有不透明碰撞形状，则为不透明完整方块。
    // 以 canOcclude + 碰撞形状完整为判定（对齐 vanilla `collisionShapeFullBlock && canOcclude` 语义），
    // 与具体材质无关：铁栏杆等 noOcclusion 方块虽有碰撞形状，但不遮挡。
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return m_canOccludeCache && m_isSolid && m_owner->getCollisionShape(*this).isFullBlock();
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
    return m_owner->getSoundType(*this);
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
    // 直接读方块，不缓存：采集工具与档位在全部方块注册完成后由
    // BlockRegistry::applyHarvestToolTable() 依原版数据包标签统一校正，
    // 构造期缓存会保留校正前的旧值（曾导致校正整体失效）。
    return m_owner->harvestTool();
}

i32 BlockState::getHarvestLevel() const
{
    // 同上，不缓存。
    return m_owner->harvestLevel();
}

world::map::MaterialColorId BlockState::getMapColor(IWorld* world, const BlockPos* pos) const
{
    return m_owner->getMapColor(*this, world, pos);
}

bool BlockState::isToolEffective(u8 toolType, i32 harvestLevel) const
{
    // 检查工具类型是否匹配
    if (m_owner->harvestTool() != toolType) {
        return false;
    }
    // 检查工具等级是否足够
    return harvestLevel >= m_owner->harvestLevel();
}

bool BlockState::requiresTool() const
{
    return m_owner->requiresTool();
}

bool BlockState::isStickyBlock() const
{
    return m_owner->isStickyBlock(*this);
}

bool BlockState::isInvisibleRenderType() const
{
    return m_owner->getRenderType(*this) == BlockRenderType::INVISIBLE;
}

bool BlockState::canBeReplacedByFluid() const
{
    // 对应 MC 的 BlockBehaviour.canBeReplaced(BlockState, Fluid)：canBeReplaced() || !isSolid()。
    // 不可用构造期缓存 m_isSolid：门/告示牌/梯子等动态形状方块在状态容器构造时还没设置碰撞
    // 形状，缓存会把它们判成非固体，使流体误认为可以流入（门/梯子因此被错误替换）。
    // isSolid() 实时按当前碰撞形状计算，与原版 initCache 之后再取 legacySolid 的时序等价。
    return m_canBeReplaced || !isSolid();
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
