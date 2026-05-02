#include "Template.hpp"
#include "../../../../world/IWorldWriter.hpp"
#include "../../../../world/block/BlockRegistry.hpp"
#include "../../../../world/block/Block.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

// ============================================================================
// BlockInfo
// ============================================================================

BlockInfo::BlockInfo() : pos(), blockStateId(0), nbt(nullptr)
{
}

BlockInfo::BlockInfo(const BlockPos& p, u32 stateId)
    : pos(p), blockStateId(stateId), nbt(nullptr)
{
}

BlockInfo::BlockInfo(const BlockInfo& other)
    : pos(other.pos), blockStateId(other.blockStateId)
{
    if (other.nbt) {
        nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
    }
}

BlockInfo::BlockInfo(BlockInfo&& other) noexcept
    : pos(std::move(other.pos))
    , blockStateId(other.blockStateId)
    , nbt(std::move(other.nbt))
{
}

BlockInfo& BlockInfo::operator=(const BlockInfo& other) {
    if (this != &other) {
        pos = other.pos;
        blockStateId = other.blockStateId;
        if (other.nbt) {
            nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
        } else {
            nbt.reset();
        }
    }
    return *this;
}

BlockInfo& BlockInfo::operator=(BlockInfo&& other) noexcept {
    if (this != &other) {
        pos = std::move(other.pos);
        blockStateId = other.blockStateId;
        nbt = std::move(other.nbt);
    }
    return *this;
}

BlockInfo::~BlockInfo() = default;

// ============================================================================
// TemplateEntityInfo
// ============================================================================

TemplateEntityInfo::TemplateEntityInfo() : pos()
{
}

TemplateEntityInfo::TemplateEntityInfo(const TemplateEntityInfo& other)
    : typeId(other.typeId), pos(other.pos)
{
    if (other.nbt) {
        nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
    }
}

TemplateEntityInfo::TemplateEntityInfo(TemplateEntityInfo&& other) noexcept
    : typeId(std::move(other.typeId))
    , pos(std::move(other.pos))
    , nbt(std::move(other.nbt))
{
}

TemplateEntityInfo& TemplateEntityInfo::operator=(const TemplateEntityInfo& other) {
    if (this != &other) {
        typeId = other.typeId;
        pos = other.pos;
        if (other.nbt) {
            nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
        } else {
            nbt.reset();
        }
    }
    return *this;
}

TemplateEntityInfo& TemplateEntityInfo::operator=(TemplateEntityInfo&& other) noexcept {
    if (this != &other) {
        typeId = std::move(other.typeId);
        pos = std::move(other.pos);
        nbt = std::move(other.nbt);
    }
    return *this;
}

TemplateEntityInfo::~TemplateEntityInfo() = default;

// ============================================================================
// PlacementSettings
// ============================================================================

PlacementSettings::PlacementSettings()
    : m_rotation(Rotation::None)
    , m_mirror(Mirror::None)
    , m_boundingBox(nullptr)
{
}

PlacementSettings& PlacementSettings::setRotation(Rotation rotation) {
    m_rotation = rotation;
    return *this;
}

PlacementSettings& PlacementSettings::setMirror(Mirror mirror) {
    m_mirror = mirror;
    return *this;
}

PlacementSettings& PlacementSettings::setIgnoreEntities(bool ignore) {
    m_ignoreEntities = ignore;
    return *this;
}

PlacementSettings& PlacementSettings::setBoundingBox(const structure::StructureBoundingBox* bounds) {
    m_boundingBox = bounds;
    return *this;
}

// ============================================================================
// Template
// ============================================================================

Template::Template() : m_size(0, 0, 0)
{
}

Template::~Template() = default;

void Template::addBlock(const BlockInfo& blockInfo) {
    m_blocks.push_back(blockInfo);
}

void Template::addJigsawBlock(const TemplateJigsawBlockInfo& jigsawInfo) {
    m_jigsawBlocks.push_back(jigsawInfo);
}

void Template::addEntity(const TemplateEntityInfo& entityInfo) {
    m_entities.push_back(entityInfo);
}

structure::StructureBoundingBox Template::getBoundingBox(
    const PlacementSettings& settings,
    const BlockPos& pos) const
{
    if (m_blocks.empty() || (m_size.x == 0 && m_size.y == 0 && m_size.z == 0)) {
        return structure::StructureBoundingBox(pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
    }

    BlockPos transformedSize = transformBlockPos(m_size, settings.getMirror(), settings.getRotation(), BlockPos(0, 0, 0));

    return structure::StructureBoundingBox(
        pos.x, pos.y, pos.z,
        pos.x + transformedSize.x - 1,
        pos.y + transformedSize.y - 1,
        pos.z + transformedSize.z - 1
    );
}

bool Template::place(
    IWorldWriter& world,
    const BlockPos& pos,
    const PlacementSettings& settings,
    math::Random& rng,
    u32 flags) const
{
    if (m_blocks.empty()) {
        return true;  // 空模板，无需放置
    }

    // 获取边界框（可选检查）
    const auto* bounds = settings.getBoundingBox();

    // 放置所有方块
    for (const auto& block : m_blocks) {
        // 计算变换后的位置
        BlockPos transformedPos = transformBlockPos(
            block.pos,
            settings.getMirror(),
            settings.getRotation(),
            BlockPos(0, 0, 0)  // 相对于原点变换
        );

        // 加上目标位置偏移
        BlockPos worldPos = pos + transformedPos;

        // 检查边界框
        if (bounds) {
            if (worldPos.x < bounds->minX() || worldPos.x > bounds->maxX() ||
                worldPos.y < bounds->minY() || worldPos.y > bounds->maxY() ||
                worldPos.z < bounds->minZ() || worldPos.z > bounds->maxZ()) {
                continue;  // 跳过边界外的方块
            }
        }

        // 获取方块状态
        const BlockState* state = BlockRegistry::instance().getBlockState(block.blockStateId);
        if (!state) {
            continue;  // 跳过无效的方块状态
        }

        // 应用镜像和旋转变换到方块状态
        const BlockState* transformedState = state;

        // 先应用镜像
        if (settings.getMirror() != Mirror::None) {
            transformedState = &transformedState->getBlock().mirror(*transformedState, settings.getMirror());
        }

        // 再应用旋转
        if (settings.getRotation() != Rotation::None) {
            transformedState = &transformedState->getBlock().rotate(*transformedState, settings.getRotation());
        }

        // 放置方块
        world.setBlock(worldPos.x, worldPos.y, worldPos.z, transformedState, static_cast<i32>(flags));

        // 方块实体数据在区块反序列化阶段统一处理，此处仅负责方块状态放置
        (void)block.nbt;
    }

    // 结构模板中的实体数据由上层实体系统统一创建
    if (!settings.ignoreEntities()) {
        for (const auto& entityInfo : m_entities) {
            (void)entityInfo;
        }
    }

    return true;
}

BlockPos Template::transformBlockPos(
    const BlockPos& pos,
    Mirror mirror,
    Rotation rotation,
    const BlockPos& center)
{
    BlockPos result = pos;

    // 减去中心偏移
    result = BlockPos(result.x - center.x, result.y, result.z - center.z);

    // 应用镜像
    switch (mirror) {
        case Mirror::LeftRight:  // Z 轴镜像（左右）
            result = BlockPos(result.x, result.y, -result.z);
            break;
        case Mirror::FrontBack:  // X 轴镜像（前后）
            result = BlockPos(-result.x, result.y, result.z);
            break;
        default:
            break;
    }

    // 应用旋转
    switch (rotation) {
        case Rotation::Clockwise90:
            result = BlockPos(-result.z, result.y, result.x);
            break;
        case Rotation::Clockwise180:
            result = BlockPos(-result.x, result.y, -result.z);
            break;
        case Rotation::CounterClockwise90:
            result = BlockPos(result.z, result.y, -result.x);
            break;
        default:
            break;
    }

    // 加回中心偏移
    result = BlockPos(result.x + center.x, result.y, result.z + center.z);

    return result;
}

BlockPos Template::getTransformedPosition(
    const BlockPos& pos,
    Rotation rotation,
    const BlockPos& size)
{
    // 计算旋转后的位置（相对于模板中心）
    // 用于将模板内的坐标转换为世界坐标
    i32 x = pos.x;
    i32 z = pos.z;

    switch (rotation) {
        case Rotation::Clockwise90:
            return BlockPos(size.z - 1 - z, pos.y, x);
        case Rotation::Clockwise180:
            return BlockPos(size.x - 1 - x, pos.y, size.z - 1 - z);
        case Rotation::CounterClockwise90:
            return BlockPos(z, pos.y, size.x - 1 - x);
        default:
            return pos;
    }
}

// ============================================================================
// Processors
// ============================================================================

GravityStructureProcessor::GravityStructureProcessor(i32 heightmapType, i32 offset)
{
}

BlockIgnoreStructureProcessor::BlockIgnoreStructureProcessor(const std::vector<u32>& blocksToIgnore)
{
}

JigsawReplacementStructureProcessor::JigsawReplacementStructureProcessor()
{
}

void StructureProcessorList::addProcessor(std::unique_ptr<StructureProcessor> processor) {
    m_processors.push_back(std::move(processor));
}

namespace ProcessorLists {

static std::unique_ptr<StructureProcessorList> s_emptyList;

const StructureProcessorList& empty() {
    if (!s_emptyList) {
        s_emptyList = std::make_unique<StructureProcessorList>();
    }
    return *s_emptyList;
}

} // namespace ProcessorLists

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
