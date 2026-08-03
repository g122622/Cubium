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

#include "Template.hpp"
#include "RuleTest.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/ILiquidContainer.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

// 使用 namespace
using fluid::FluidState;

// ============================================================================
// BlockAgeProcessor 常量
// ============================================================================

// 完整石砖方块被替换的概率（50%）
static constexpr f32 PROBABILITY_OF_REPLACING_FULL_BLOCK = 0.5f;

// 楼梯方块被替换的概率（50%）
static constexpr f32 PROBABILITY_OF_REPLACING_STAIRS = 0.5f;

// 黑曜石变哭泣黑曜石的概率（固定 15%，不受 mossiness 影响）
static constexpr f32 PROBABILITY_OF_REPLACING_OBSIDIAN = 0.15f;

// 非 mossiness 组候选数组大小
static constexpr size_t REPLACEMENT_OPTIONS_COUNT = 2;

// ============================================================================
// BlockInfo
// ============================================================================

BlockInfo::BlockInfo()
    : pos()
    , blockStateId(0)
    , nbt(nullptr)
{}

BlockInfo::BlockInfo(const BlockPos& p, u32 stateId)
    : pos(p)
    , blockStateId(stateId)
    , nbt(nullptr)
{}

BlockInfo::BlockInfo(const BlockInfo& other)
    : pos(other.pos)
    , blockStateId(other.blockStateId)
{
    if (other.nbt) {
        nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
    }
}

BlockInfo::BlockInfo(BlockInfo&& other) noexcept
    : pos(std::move(other.pos))
    , blockStateId(other.blockStateId)
    , nbt(std::move(other.nbt))
{}

BlockInfo& BlockInfo::operator=(const BlockInfo& other)
{
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

BlockInfo& BlockInfo::operator=(BlockInfo&& other) noexcept
{
    if (this != &other) {
        pos = std::move(other.pos);
        blockStateId = other.blockStateId;
        nbt = std::move(other.nbt);
    }
    return *this;
}

BlockInfo::~BlockInfo() = default;

// ============================================================================
// ProcessedBlockInfo
// ============================================================================

ProcessedBlockInfo::ProcessedBlockInfo(const ProcessedBlockInfo& other)
    : pos(other.pos)
    , blockStateId(other.blockStateId)
{
    if (other.nbt) {
        nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
    }
}

ProcessedBlockInfo::ProcessedBlockInfo(ProcessedBlockInfo&& other) noexcept
    : pos(std::move(other.pos))
    , blockStateId(other.blockStateId)
    , nbt(std::move(other.nbt))
{}

ProcessedBlockInfo& ProcessedBlockInfo::operator=(const ProcessedBlockInfo& other)
{
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

ProcessedBlockInfo& ProcessedBlockInfo::operator=(ProcessedBlockInfo&& other) noexcept
{
    if (this != &other) {
        pos = std::move(other.pos);
        blockStateId = other.blockStateId;
        nbt = std::move(other.nbt);
    }
    return *this;
}

ProcessedBlockInfo::~ProcessedBlockInfo() = default;

// ============================================================================
// StructureProcessor
// ============================================================================

std::optional<ProcessedBlockInfo> StructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 默认实现：不修改，直接返回处理后的方块信息
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

std::vector<ProcessedBlockInfo> StructureProcessor::finalizeProcessing(const BlockPos& /*seedPos*/,
    const PlacementSettings& /*settings*/,
    const std::vector<BlockInfo>& /*originalBlocks*/,
    std::vector<ProcessedBlockInfo> processedBlocks)
{
    // 默认实现：直接返回处理后的方块列表，不做任何修改
    return processedBlocks;
}

// ============================================================================
// TemplateEntityInfo
// ============================================================================

TemplateEntityInfo::TemplateEntityInfo()
    : posx(0.0)
    , posy(0.0)
    , posz(0.0)
    , blockPos()
{}

TemplateEntityInfo::TemplateEntityInfo(const TemplateEntityInfo& other)
    : typeId(other.typeId)
    , posx(other.posx)
    , posy(other.posy)
    , posz(other.posz)
    , blockPos(other.blockPos)
{
    if (other.nbt) {
        nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
    }
}

TemplateEntityInfo::TemplateEntityInfo(TemplateEntityInfo&& other) noexcept
    : typeId(std::move(other.typeId))
    , posx(other.posx)
    , posy(other.posy)
    , posz(other.posz)
    , blockPos(std::move(other.blockPos))
    , nbt(std::move(other.nbt))
{}

TemplateEntityInfo& TemplateEntityInfo::operator=(const TemplateEntityInfo& other)
{
    if (this != &other) {
        typeId = other.typeId;
        posx = other.posx;
        posy = other.posy;
        posz = other.posz;
        blockPos = other.blockPos;
        if (other.nbt) {
            nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
        } else {
            nbt.reset();
        }
    }
    return *this;
}

TemplateEntityInfo& TemplateEntityInfo::operator=(TemplateEntityInfo&& other) noexcept
{
    if (this != &other) {
        typeId = std::move(other.typeId);
        posx = other.posx;
        posy = other.posy;
        posz = other.posz;
        blockPos = std::move(other.blockPos);
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
    , m_centerOffset(0, 0, 0)
    , m_blockUpdateFlags(18)
{}

PlacementSettings& PlacementSettings::setRotation(Rotation rotation)
{
    m_rotation = rotation;
    return *this;
}

PlacementSettings& PlacementSettings::setMirror(Mirror mirror)
{
    m_mirror = mirror;
    return *this;
}

PlacementSettings& PlacementSettings::setIgnoreEntities(bool ignore)
{
    m_ignoreEntities = ignore;
    return *this;
}

PlacementSettings& PlacementSettings::setBoundingBox(const structure::StructureBoundingBox* bounds)
{
    m_boundingBox = bounds;
    return *this;
}

PlacementSettings& PlacementSettings::setCenterOffset(const BlockPos& offset)
{
    m_centerOffset = offset;
    return *this;
}

PlacementSettings& PlacementSettings::setBlockUpdateFlags(u32 flags)
{
    m_blockUpdateFlags = flags;
    return *this;
}

PlacementSettings& PlacementSettings::setKeepLiquids(bool keep)
{
    m_keepLiquids = keep;
    return *this;
}

math::Random PlacementSettings::getRandom(const BlockPos& pos) const
{
    // 如果设置了预设随机数，则返回副本；否则基于位置种子创建
    if (m_random) {
        return *m_random;
    }
    // 使用位置种子创建确定性随机数
    return math::Random(math::getPositionRandom(pos.x, pos.y, pos.z));
}

PlacementSettings PlacementSettings::copy() const
{
    PlacementSettings result;
    result.m_rotation = m_rotation;
    result.m_mirror = m_mirror;
    result.m_ignoreEntities = m_ignoreEntities;
    result.m_keepLiquids = m_keepLiquids;
    result.m_boundingBox = m_boundingBox;
    result.m_centerOffset = m_centerOffset;
    result.m_blockUpdateFlags = m_blockUpdateFlags;
    result.m_processors = m_processors;
    result.m_world = m_world;
    result.m_random = m_random;
    return result;
}

PlacementSettings& PlacementSettings::setProcessors(const StructureProcessorList* processors)
{
    m_processors = processors;
    return *this;
}

// ============================================================================
// Palette
// ============================================================================

Palette::Palette(std::vector<BlockInfo> blocks)
    : m_blocks(std::move(blocks))
{}

const std::vector<const BlockInfo*>& Palette::getBlocksByType(const Block& block) const
{
    // 检查缓存
    auto it = m_blockTypeCache.find(&block);
    if (it != m_blockTypeCache.end()) {
        return it->second;
    }

    // 构建缓存
    if (!m_cacheBuilt) {
        _buildCache();
    }

    it = m_blockTypeCache.find(&block);
    if (it != m_blockTypeCache.end()) {
        return it->second;
    }

    // 没有匹配的方块，返回空列表
    static const std::vector<const BlockInfo*> empty;
    return empty;
}

void Palette::_buildCache() const
{
    // 按方块类型缓存
    auto& registry = BlockRegistry::instance();

    for (const auto& blockInfo : m_blocks) {
        const BlockState* state = registry.getBlockState(blockInfo.blockStateId);
        if (state) {
            const Block* block = &state->getBlock();
            m_blockTypeCache[block].push_back(&blockInfo);
        }
    }
    m_cacheBuilt = true;
}

// ============================================================================
// Template
// ============================================================================

Template::Template()
    : m_size(0, 0, 0)
{}

Template::~Template() = default;

void Template::addPalette(Palette palette)
{
    m_palettes.push_back(std::move(palette));
}

const Palette* Template::getPalette(size_t index) const
{
    if (index < m_palettes.size()) {
        return &m_palettes[index];
    }
    return nullptr;
}

const Palette* Template::selectPalette(math::Random& rng) const
{
    if (m_palettes.empty()) {
        return nullptr;
    }
    // 随机选择一个调色板
    size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(m_palettes.size())));
    return &m_palettes[index];
}

const std::vector<BlockInfo>& Template::getBlocks() const
{
    // 兼容旧接口：返回第一个调色板的方块
    static const std::vector<BlockInfo> empty;
    if (m_palettes.empty()) {
        return empty;
    }
    return m_palettes[0].blocks();
}

size_t Template::getBlockCount() const
{
    size_t count = 0;
    for (const auto& palette : m_palettes) {
        count += palette.size();
    }
    return count;
}

void Template::addJigsawBlock(const TemplateJigsawBlockInfo& jigsawInfo)
{
    m_jigsawBlocks.push_back(jigsawInfo);
}

void Template::addEntity(const TemplateEntityInfo& entityInfo)
{
    m_entities.push_back(entityInfo);
}

structure::StructureBoundingBox Template::getBoundingBox(const PlacementSettings& settings, const BlockPos& pos) const
{
    // 即使没有方块，也使用模板尺寸
    if (m_size.x == 0 && m_size.y == 0 && m_size.z == 0) {
        return structure::StructureBoundingBox(pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
    }

    // 计算变换后的尺寸
    // 对于尺寸，旋转90/270度会交换X和Z
    i32 sizeX = m_size.x;
    i32 sizeZ = m_size.z;

    switch (settings.getRotation()) {
        case Rotation::Clockwise90:
        case Rotation::CounterClockwise90:
            // 90度或270度旋转交换X和Z尺寸
            std::swap(sizeX, sizeZ);
            break;
        default:
            break;
    }

    // 注意：镜像不影响尺寸，只影响位置

    return structure::StructureBoundingBox(pos.x,
        pos.y,
        pos.z,
        pos.x + sizeX - 1,
        pos.y + m_size.y - 1, // Y尺寸不变
        pos.z + sizeZ - 1);
}

bool Template::place(
    IWorldWriter& world, const BlockPos& pos, const PlacementSettings& settings, math::Random& rng, u32 flags) const
{
    // 选择调色板
    const Palette* selectedPalette = selectPalette(rng);
    if (!selectedPalette || selectedPalette->empty()) {
        // 没有调色板或调色板为空，检查旧格式的方块列表
        if (m_palettes.empty()) {
            return true; // 空模板，无需放置
        }
        selectedPalette = &m_palettes[0];
        if (selectedPalette->empty()) {
            return true;
        }
    }

    const std::vector<BlockInfo>& blocks = selectedPalette->blocks();

    // 获取边界框（可选检查）
    const auto* bounds = settings.getBoundingBox();

    // 首先处理方块信息（应用处理器链）
    std::vector<BlockInfo> originalBlocks;
    std::vector<ProcessedBlockInfo> processedBlocks;
    originalBlocks.reserve(blocks.size());
    processedBlocks.reserve(blocks.size());

    for (const auto& block : blocks) {
        // 计算变换后的位置
        BlockPos transformedPos = transformBlockPos(
            block.pos, settings.getMirror(), settings.getRotation(), BlockPos(0, 0, 0) // 相对于原点变换
        );

        // 加上目标位置偏移
        BlockPos worldPos = pos + transformedPos;

        // 创建待处理的方块信息
        BlockInfo blockInfo(worldPos, block.blockStateId);
        if (block.nbt) {
            blockInfo.nbt = std::make_unique<nbt::CompoundTag>(*block.nbt);
        }

        // 保存原始方块信息（用于 finalizeProcessing）
        BlockInfo rawInfo(block.pos, block.blockStateId);
        if (block.nbt) {
            rawInfo.nbt = std::make_unique<nbt::CompoundTag>(*block.nbt);
        }

        // 应用处理器链
        ProcessedBlockInfo processedBlock;
        processedBlock.pos = worldPos;
        processedBlock.blockStateId = blockInfo.blockStateId;
        if (blockInfo.nbt) {
            processedBlock.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }

        // 处理器链处理
        const StructureProcessorList* processors = settings.getProcessors();
        if (processors) {
            bool shouldKeep = true;
            for (const auto& processor : *processors) {
                if (processor) {
                    auto result = processor->process(pos, worldPos, rawInfo, blockInfo, settings);
                    if (!result) {
                        shouldKeep = false;
                        break; // 处理器返回空，跳过此方块
                    }
                    processedBlock = *result;
                    // 更新 blockInfo 供下一个处理器使用
                    blockInfo.pos = processedBlock.pos;
                    blockInfo.blockStateId = processedBlock.blockStateId;
                    if (processedBlock.nbt) {
                        blockInfo.nbt = std::make_unique<nbt::CompoundTag>(*processedBlock.nbt);
                    }
                }
            }

            if (!shouldKeep) {
                continue; // 跳过此方块
            }
        }

        originalBlocks.push_back(std::move(rawInfo));
        processedBlocks.push_back(std::move(processedBlock));
    }

    // 后处理阶段：对所有处理器调用 finalizeProcessing
    if (settings.getProcessors()) {
        for (const auto& processor : *settings.getProcessors()) {
            if (processor) {
                processedBlocks =
                    processor->finalizeProcessing(pos, settings, originalBlocks, std::move(processedBlocks));
            }
        }
        // 确保原始列表和处理后列表大小一致（finalizeProcessing 可能移除方块）
        if (processedBlocks.size() < originalBlocks.size()) {
            originalBlocks.resize(processedBlocks.size());
        }
    }

    // 方块排序逻辑：按 Y, X, Z 坐标排序，并分为三类
    // 1. 普通方块（无 NBT，有opaque碰撞箱）
    // 2. 其他方块（透明或变量透明度）
    // 3. 方块实体（有 NBT）
    // 最终顺序：普通方块 -> 其他方块 -> 方块实体

    // 分类方块
    std::vector<ProcessedBlockInfo> normalBlocks;      // 有opaque碰撞箱的方块
    std::vector<ProcessedBlockInfo> otherBlocks;       // 透明/变量透明度方块
    std::vector<ProcessedBlockInfo> blockEntityBlocks; // 有NBT的方块

    for (auto& block : processedBlocks) {
        const BlockState* state = BlockRegistry::instance().getBlockState(block.blockStateId);
        if (!state) {
            otherBlocks.push_back(std::move(block));
            continue;
        }

        if (block.nbt) {
            // 有 NBT 的方块最后放置
            blockEntityBlocks.push_back(std::move(block));
        } else if (state->hasOpaqueCollisionShape()) {
            // 有opaque碰撞箱的方块首先放置
            // 当前项目中所有方块默认 variableOpacity = false
            normalBlocks.push_back(std::move(block));
        } else {
            // 透明或变量透明度的方块放在中间
            otherBlocks.push_back(std::move(block));
        }
    }

    // 排序函数：按 Y, X, Z 坐标排序
    auto blockComparator = [](const ProcessedBlockInfo& a, const ProcessedBlockInfo& b) {
        if (a.pos.y != b.pos.y) return a.pos.y < b.pos.y;
        if (a.pos.x != b.pos.x) return a.pos.x < b.pos.x;
        return a.pos.z < b.pos.z;
    };

    std::sort(normalBlocks.begin(), normalBlocks.end(), blockComparator);
    std::sort(otherBlocks.begin(), otherBlocks.end(), blockComparator);
    std::sort(blockEntityBlocks.begin(), blockEntityBlocks.end(), blockComparator);

    // 合并方块列表：普通 -> 其他 -> 方块实体
    processedBlocks.clear();
    processedBlocks.reserve(normalBlocks.size() + otherBlocks.size() + blockEntityBlocks.size());
    for (auto& block : normalBlocks) {
        processedBlocks.push_back(std::move(block));
    }
    for (auto& block : otherBlocks) {
        processedBlocks.push_back(std::move(block));
    }
    for (auto& block : blockEntityBlocks) {
        processedBlocks.push_back(std::move(block));
    }

    // 放置所有处理后的方块
    for (const auto& processedBlock : processedBlocks) {
        // 检查边界框
        if (bounds) {
            if (processedBlock.pos.x < bounds->minX() || processedBlock.pos.x > bounds->maxX() ||
                processedBlock.pos.y < bounds->minY() || processedBlock.pos.y > bounds->maxY() ||
                processedBlock.pos.z < bounds->minZ() || processedBlock.pos.z > bounds->maxZ()) {
                continue; // 跳过边界外的方块
            }
        }

        // 获取方块状态
        const BlockState* state = BlockRegistry::instance().getBlockState(processedBlock.blockStateId);
        if (!state) {
            continue; // 跳过无效的方块状态
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
        world.setBlockState(processedBlock.pos.x,
            processedBlock.pos.y,
            processedBlock.pos.z,
            transformedState,
            static_cast<i32>(flags));

        // 方块实体数据由 placeInWorld 方法处理
        // 此处仅负责方块状态放置
        (void)processedBlock.nbt;
    }

    // 结构模板中的实体数据由 placeInWorld 方法处理
    (void)settings;

    return true;
}

bool Template::placeInWorld(
    IWorld& world, const BlockPos& pos, const PlacementSettings& settings, math::Random& rng, u32 flags) const
{
    // 选择调色板
    const Palette* selectedPalette = selectPalette(rng);
    if (!selectedPalette || selectedPalette->empty()) {
        if (m_palettes.empty()) {
            return true;
        }
        selectedPalette = &m_palettes[0];
        if (selectedPalette->empty()) {
            return true;
        }
    }

    const std::vector<BlockInfo>& blocks = selectedPalette->blocks();
    const auto* bounds = settings.getBoundingBox();

    // 处理方块信息
    std::vector<BlockInfo> originalBlocks;
    std::vector<ProcessedBlockInfo> processedBlocks;
    originalBlocks.reserve(blocks.size());
    processedBlocks.reserve(blocks.size());

    for (const auto& block : blocks) {
        BlockPos transformedPos =
            transformBlockPos(block.pos, settings.getMirror(), settings.getRotation(), BlockPos(0, 0, 0));
        BlockPos worldPos = pos + transformedPos;

        BlockInfo blockInfo(worldPos, block.blockStateId);
        if (block.nbt) {
            blockInfo.nbt = std::make_unique<nbt::CompoundTag>(*block.nbt);
        }

        // 保存原始方块信息（用于 finalizeProcessing）
        BlockInfo rawInfo(block.pos, block.blockStateId);
        if (block.nbt) {
            rawInfo.nbt = std::make_unique<nbt::CompoundTag>(*block.nbt);
        }

        ProcessedBlockInfo processedBlock;
        processedBlock.pos = worldPos;
        processedBlock.blockStateId = blockInfo.blockStateId;
        if (blockInfo.nbt) {
            processedBlock.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }

        // 应用处理器链
        const StructureProcessorList* processors = settings.getProcessors();
        if (processors) {
            bool shouldKeep = true;
            for (const auto& processor : *processors) {
                if (processor) {
                    auto result = processor->process(pos, worldPos, rawInfo, blockInfo, settings);
                    if (!result) {
                        shouldKeep = false;
                        break;
                    }
                    processedBlock = *result;
                    blockInfo.pos = processedBlock.pos;
                    blockInfo.blockStateId = processedBlock.blockStateId;
                    if (processedBlock.nbt) {
                        blockInfo.nbt = std::make_unique<nbt::CompoundTag>(*processedBlock.nbt);
                    }
                }
            }

            if (!shouldKeep) {
                continue;
            }
        }

        originalBlocks.push_back(std::move(rawInfo));
        processedBlocks.push_back(std::move(processedBlock));
    }

    // 后处理阶段：对所有处理器调用 finalizeProcessing
    if (settings.getProcessors()) {
        for (const auto& processor : *settings.getProcessors()) {
            if (processor) {
                processedBlocks =
                    processor->finalizeProcessing(pos, settings, originalBlocks, std::move(processedBlocks));
            }
        }
        // 确保原始列表和处理后列表大小一致（finalizeProcessing 可能移除方块）
        if (processedBlocks.size() < originalBlocks.size()) {
            originalBlocks.resize(processedBlocks.size());
        }
    }

    // 记录需要处理液体的位置
    std::vector<BlockPos> fluidUpdatePositions;

    // 放置所有处理后的方块
    for (const auto& processedBlock : processedBlocks) {
        // 检查边界框
        if (bounds) {
            if (processedBlock.pos.x < bounds->minX() || processedBlock.pos.x > bounds->maxX() ||
                processedBlock.pos.y < bounds->minY() || processedBlock.pos.y > bounds->maxY() ||
                processedBlock.pos.z < bounds->minZ() || processedBlock.pos.z > bounds->maxZ()) {
                continue;
            }
        }

        const BlockState* state = BlockRegistry::instance().getBlockState(processedBlock.blockStateId);
        if (!state) {
            continue;
        }

        // 应用镜像和旋转变换到方块状态
        const BlockState* transformedState = state;
        if (settings.getMirror() != Mirror::None) {
            transformedState = &transformedState->getBlock().mirror(*transformedState, settings.getMirror());
        }
        if (settings.getRotation() != Rotation::None) {
            transformedState = &transformedState->getBlock().rotate(*transformedState, settings.getRotation());
        }

        // 如果有 TileEntity NBT，先清除旧 TileEntity
        if (processedBlock.nbt) {
            BlockEntity* existingEntity = world.getBlockEntity(processedBlock.pos);
            if (existingEntity) {
                // 清除旧 TileEntity
                world.removeBlockEntity(processedBlock.pos);
            }
        }

        // 获取当前位置的流体状态（如果需要保留液体）
        const FluidState* fluidState = nullptr;
        if (settings.keepLiquids()) {
            fluidState = world.getFluidState(processedBlock.pos);
        }

        // 放置方块
        bool placed = world.setBlockState(processedBlock.pos.x,
            processedBlock.pos.y,
            processedBlock.pos.z,
            transformedState,
            static_cast<i32>(flags));

        if (!placed) {
            continue;
        }

        // 处理 TileEntity NBT
        if (processedBlock.nbt) {
            BlockEntity* tileEntity = world.getBlockEntity(processedBlock.pos);
            if (tileEntity) {
                // 更新位置坐标
                processedBlock.nbt->put("x", processedBlock.pos.x);
                processedBlock.nbt->put("y", processedBlock.pos.y);
                processedBlock.nbt->put("z", processedBlock.pos.z);

                // 为战利品容器注入随机 LootTableSeed。
                // LootableContainerBlockEntity 是 RandomizableContainer 的等价类
                // （ChestEntity/BarrelEntity/ShulkerBoxEntity/DispenserBlockEntity 等）。
                // 仅注入 LootTableSeed，LootTable 键已存在于模板 NBT 中。
                // BrushableBlockEntity（可疑沙）不是 LootableContainerBlockEntity，
                // 其 LootTableSeed 由结构生成器（如 DesertPyramidStructure）通过
                // setLootTable() 直接设置，此处不干预。
                if (dynamic_cast<blockentity::LootableContainerBlockEntity*>(tileEntity) != nullptr) {
                    processedBlock.nbt->put("LootTableSeed", rng.nextLong());
                }

                // 加载 NBT 数据到方块实体
                tileEntity->loadFromNBT(*processedBlock.nbt);
            }
        }

        // 处理液体填充
        if (fluidState && !fluidState->isEmpty()) {
            Block& block = transformedState->getBlockMutable();
            ILiquidContainer* liquidContainer = dynamic_cast<ILiquidContainer*>(&block);
            if (liquidContainer &&
                liquidContainer->canContainFluid(
                    world, processedBlock.pos, *transformedState, fluidState->getFluid())) {
                liquidContainer->receiveFluid(world, processedBlock.pos, *transformedState, *fluidState);
                // 如果不是源头，记录位置以便后续处理
                if (!fluidState->isSource()) {
                    fluidUpdatePositions.push_back(processedBlock.pos);
                }
            }
        }
    }

    // 液体传播处理
    // 如果方块被放置在非源流体位置，需要处理流体传播
    if (!fluidUpdatePositions.empty() && settings.keepLiquids()) {
        // 完整实现：追踪相邻流体并填充容器
        static const Direction directions[] = {
            Direction::Up, Direction::North, Direction::East, Direction::South, Direction::West};

        bool changed = true;
        while (changed && !fluidUpdatePositions.empty()) {
            changed = false;
            auto it = fluidUpdatePositions.begin();
            while (it != fluidUpdatePositions.end()) {
                const BlockPos& fluidPos = *it;
                BlockPos bestPos = fluidPos;
                const FluidState* bestFluid = world.getFluidState(fluidPos);

                // 查找相邻的最高流体
                for (const Direction& dir : directions) {
                    if (!bestFluid || bestFluid->isSource()) break;

                    BlockPos adjacentPos = fluidPos.offset(dir);
                    const FluidState* adjacentFluid = world.getFluidState(adjacentPos);
                    if (!adjacentFluid || adjacentFluid->isEmpty()) continue;

                    // 比较流体高度：更高的流体或源流体优先
                    if (adjacentFluid->getActualHeight(world, adjacentPos) >
                            bestFluid->getActualHeight(world, bestPos) ||
                        (adjacentFluid->isSource() && !bestFluid->isSource())) {
                        bestFluid = adjacentFluid;
                        bestPos = adjacentPos;
                    }
                }

                // 如果找到源流体，填充容器
                if (bestFluid && bestFluid->isSource()) {
                    const BlockState* blockState = world.getBlockState(fluidPos);
                    if (blockState) {
                        Block& block = blockState->getBlockMutable();
                        ILiquidContainer* liquidContainer = dynamic_cast<ILiquidContainer*>(&block);
                        if (liquidContainer &&
                            liquidContainer->canContainFluid(world, fluidPos, *blockState, bestFluid->getFluid())) {
                            liquidContainer->receiveFluid(world, fluidPos, *blockState, *bestFluid);
                            changed = true;
                            it = fluidUpdatePositions.erase(it);
                            continue;
                        }
                    }
                }

                ++it;
            }
        }
    }

    // 形状更新：对所有放置的方块根据邻居状态更新其形状
    // 如栅栏连接、楼梯朝向、红石线路形状等需要根据邻居方块确定状态的方块
    for (const auto& processedBlock : processedBlocks) {
        const BlockState* currentState = world.getBlockState(processedBlock.pos);
        if (currentState == nullptr || currentState->isAir()) {
            continue;
        }
        BlockState updated = Block::updateFromNeighbourShapes(*currentState, world, processedBlock.pos);
        if (updated != *currentState) {
            world.setBlockState(processedBlock.pos, &updated, 276);
        }
    }

    // 处理实体
    if (!settings.ignoreEntities() && !m_entities.empty()) {
        for (const auto& entityInfo : m_entities) {
            // 变换实体方块位置（用于边界框检查）
            BlockPos transformedBlockPos =
                transformBlockPos(entityInfo.blockPos, settings.getMirror(), settings.getRotation(), BlockPos(0, 0, 0));
            BlockPos entityBlockPos = pos + transformedBlockPos;

            // 检查边界框
            if (bounds) {
                if (entityBlockPos.x < bounds->minX() || entityBlockPos.x > bounds->maxX() ||
                    entityBlockPos.y < bounds->minY() || entityBlockPos.y > bounds->maxY() ||
                    entityBlockPos.z < bounds->minZ() || entityBlockPos.z > bounds->maxZ()) {
                    continue;
                }
            }

            // 变换实体精确位置（f64），对应 MC 1.21.11 StructureTemplate#transform(Vec3, ...)
            // 实体使用 block-corner 坐标系：镜像用 1.0 - coord，旋转含 +1 偏移
            math::Vector3d templatePos(entityInfo.posx, entityInfo.posy, entityInfo.posz);
            math::Vector3d transformedPos =
                transformEntityPos(templatePos, settings.getMirror(), settings.getRotation(), BlockPos(0, 0, 0));

            // 加上世界偏移
            f64 entityX = transformedPos.x + static_cast<f64>(pos.x);
            f64 entityY = transformedPos.y + static_cast<f64>(pos.y);
            f64 entityZ = transformedPos.z + static_cast<f64>(pos.z);

            // 创建实体
            if (!entityInfo.typeId.empty()) {
                // 解析实体类型
                const entity::EntityType* entityType = entity::EntityRegistry::instance().getType(entityInfo.typeId);
                if (entityType) {
                    auto entity = entityType->create(&world);
                    if (entity) {
                        // 加载 NBT 数据到实体（对应 MC 1.21.11 EntityType.create(NBT)）
                        // NBT 包含实体的完整状态（健康、装备、年龄、自定义数据等），
                        // 随后我们用变换后的位置/朝向覆盖 NBT 中的 Pos/Rotation。
                        bool nbtLoaded = false;
                        if (entityInfo.nbt) {
                            auto result = entity->readFromNBT(*entityInfo.nbt);
                            if (result.failed()) {
                                spdlog::warn("[Template] Failed to load entity NBT for type '{}': {}",
                                    entityInfo.typeId,
                                    result.error().toString());
                            } else {
                                nbtLoaded = true;
                            }
                        }

                        // 计算变换后的朝向（对应 MC 1.21.11 Entity#rotate + Entity#mirror）
                        // f = rotate(yaw, rotation) + (mirror(yaw, mirror) - yaw)
                        // 其中 rotate(yaw) = yaw + 90/180/270/0
                        // mirror(yaw) = -yaw（FrontBack）或 180 - yaw（LeftRight）或 yaw（None）
                        // 因此 mirror(yaw) - yaw 在 FrontBack 下为 -2*yaw，LeftRight 下为 180 - 2*yaw
                        f32 yaw = math::wrapDegrees(entity->yaw());
                        f32 pitch = entity->pitch();

                        // mirror(yaw)
                        f32 mirroredYaw = yaw;
                        switch (settings.getMirror()) {
                            case Mirror::FrontBack:
                                mirroredYaw = -yaw;
                                break;
                            case Mirror::LeftRight:
                                mirroredYaw = 180.0f - yaw;
                                break;
                            default:
                                break;
                        }

                        // rotate(yaw)
                        f32 rotatedYaw = yaw;
                        switch (settings.getRotation()) {
                            case Rotation::Clockwise90:
                                rotatedYaw = yaw + 90.0f;
                                break;
                            case Rotation::Clockwise180:
                                rotatedYaw = yaw + 180.0f;
                                break;
                            case Rotation::CounterClockwise90:
                                rotatedYaw = yaw + 270.0f;
                                break;
                            default:
                                break;
                        }

                        // f = rotate(yaw) + (mirror(yaw) - yaw)
                        f32 finalYaw = math::wrapDegrees(rotatedYaw + (mirroredYaw - yaw));

                        // 用变换后的位置和朝向覆盖 NBT 中读取的值
                        // 对应 MC 1.21.11 Entity#snapTo(vec31.x, vec31.y, vec31.z, f, getXRot())
                        entity->setPosition(
                            static_cast<f32>(entityX), static_cast<f32>(entityY), static_cast<f32>(entityZ));
                        entity->setRotation(finalYaw, pitch);

                        // 同步身体与头部朝向到结构旋转后的 yaw
                        // 对应 MC 1.21.11 StructureTemplate#placeEntities 中：
                        //   p_454648_.setYBodyRot(f);
                        //   p_454648_.setYHeadRot(f);
                        // Entity 基类的 setYBodyRot/setYHeadRot 默认空实现，
                        // LivingEntity 重写后会分别写入 m_renderYawOffset 与 m_rotationYawHead，
                        // 因此对任意实体类型调用都安全（非 LivingEntity 时为 no-op）。
                        // readFromNBT 已将 body/head 同步为 NBT 中的原始 yaw，
                        // 此处覆盖为结构旋转后的 finalYaw，让首帧身体/头部朝向正确。
                        entity->setYBodyRot(finalYaw);
                        entity->setYHeadRot(finalYaw);

                        // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
                        // 对应 MC 1.21.11 Mob#finalizeSpawn(ServerLevelAccessor, DifficultyInstance, SpawnReason)
                        auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
                        if (mobEntity != nullptr) {
                            BlockPos difficultyPos(static_cast<i32>(std::floor(entityX)),
                                static_cast<i32>(entityY),
                                static_cast<i32>(std::floor(entityZ)));
                            entity::combat::DifficultyInstance difficultyInstance =
                                entity::combat::DifficultyInstance::at(world, difficultyPos);
                            mobEntity->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Structure);
                        }

                        // 生成实体
                        world.spawnEntity(std::move(entity));
                        (void)nbtLoaded; // 避免未使用变量警告
                    }
                } else {
                    spdlog::warn("[Template] Unknown entity type '{}' in structure template", entityInfo.typeId);
                }
            }
        }
    }

    return true;
}

BlockPos Template::transformBlockPos(const BlockPos& pos, Mirror mirror, Rotation rotation, const BlockPos& center)
{
    BlockPos result = pos;

    // 减去中心偏移
    result = BlockPos(result.x - center.x, result.y, result.z - center.z);

    // 应用镜像
    switch (mirror) {
        case Mirror::LeftRight: // Z 轴镜像（左右）
            result = BlockPos(result.x, result.y, -result.z);
            break;
        case Mirror::FrontBack: // X 轴镜像（前后）
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

math::Vector3d Template::transformEntityPos(
    const math::Vector3d& pos, Mirror mirror, Rotation rotation, const BlockPos& pivot)
{
    // 对应 MC 1.21.11 StructureTemplate#transform(Vec3, Mirror, Rotation, BlockPos)
    // 实体使用浮点坐标，镜像使用 `1.0 - coord`（block-corner 坐标系），
    // 旋转公式包含 +1 偏移以保持子方块对齐。
    f64 d0 = pos.x;
    f64 d1 = pos.y;
    f64 d2 = pos.z;
    bool flag = true;

    switch (mirror) {
        case Mirror::LeftRight:
            d2 = 1.0 - d2;
            break;
        case Mirror::FrontBack:
            d0 = 1.0 - d0;
            break;
        default:
            flag = false;
            break;
    }

    const i32 i = pivot.x;
    const i32 j = pivot.z;

    switch (rotation) {
        case Rotation::CounterClockwise90:
            return math::Vector3d(static_cast<f64>(i - j) + d2, d1, static_cast<f64>(i + j + 1) - d0);
        case Rotation::Clockwise90:
            return math::Vector3d(static_cast<f64>(i + j + 1) - d2, d1, static_cast<f64>(j - i) + d0);
        case Rotation::Clockwise180:
            return math::Vector3d(static_cast<f64>(i + i + 1) - d0, d1, static_cast<f64>(j + j + 1) - d2);
        case Rotation::None:
        default:
            return flag ? math::Vector3d(d0, d1, d2) : pos;
    }
}

BlockPos Template::getTransformedPosition(const BlockPos& pos, Rotation rotation, const BlockPos& size)
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
    : m_heightmapType(heightmapType)
    , m_offset(offset)
{}

std::optional<ProcessedBlockInfo> GravityStructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings)
{
    // GravityStructureProcessor 根据高度图调整 Y 坐标
    // 如果有世界访问，则获取地面高度；否则使用简化实现
    const IWorld* world = settings.getWorld();

    ProcessedBlockInfo result = ProcessedBlockInfo::fromBlockInfo(blockInfo);

    if (world) {
        // 完整实现：使用高度图获取地面高度
        i32 surfaceY = world->getHeight(blockInfo.pos.x, blockInfo.pos.z);
        result.pos = BlockPos(blockInfo.pos.x, surfaceY + m_offset, blockInfo.pos.z);
    } else {
        // 简化实现：仅应用偏移量
        result.pos = BlockPos(blockInfo.pos.x, blockInfo.pos.y + m_offset, blockInfo.pos.z);
    }

    return result;
}

BlockIgnoreStructureProcessor::BlockIgnoreStructureProcessor(const std::vector<u32>& blocksToIgnore)
    : m_blocksToIgnore(blocksToIgnore.begin(), blocksToIgnore.end())
{}

std::optional<ProcessedBlockInfo> BlockIgnoreStructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 使用 unordered_set 进行 O(1) 查找
    if (m_blocksToIgnore.count(blockInfo.blockStateId) > 0) {
        return std::nullopt; // 跳过此方块
    }

    // 保留方块
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

JigsawReplacementStructureProcessor::JigsawReplacementStructureProcessor() {}

std::optional<ProcessedBlockInfo> JigsawReplacementStructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // JigsawReplacementStructureProcessor
    // 检查方块是否为 Jigsaw 方块
    // 如果是，读取 NBT 中的 final_state 字段并解析为新的方块状态

    const BlockState* state = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (!state) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 检查是否是 Jigsaw 方块
    const Block& block = state->getBlock();
    ResourceLocation blockId = block.blockLocation();
    if (blockId.toString() != "minecraft:jigsaw") {
        // 不是 Jigsaw 方块，保持原样
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 是 Jigsaw 方块，读取 final_state
    if (!blockInfo.nbt) {
        // 没有 NBT，返回空气（跳过）
        return std::nullopt;
    }

    auto it = blockInfo.nbt->value.find("final_state");
    if (it == blockInfo.nbt->value.end() || !it->second) {
        // 没有 final_state，返回空气（跳过）
        return std::nullopt;
    }

    if (it->second->id() != nbt::TagId::String) {
        return std::nullopt;
    }

    const std::string& finalStateStr = dynamic_cast<const nbt::StringTag&>(*it->second).value;

    // 解析 final_state 字符串
    // 格式: "minecraft:stone[properties]" 或 "minecraft:stone"
    u32 newStateId = parseBlockStateString(finalStateStr);

    // 检查是否是 structure_void
    const BlockState* newState = BlockRegistry::instance().getBlockState(newStateId);
    if (newState) {
        ResourceLocation newBlockId = newState->getBlock().blockLocation();
        if (newBlockId.toString() == "minecraft:structure_void") {
            // structure_void 表示跳过此方块
            return std::nullopt;
        }
    }

    // 返回新的方块状态（无 NBT）
    ProcessedBlockInfo result;
    result.pos = blockInfo.pos;
    result.blockStateId = newStateId;
    // Jigsaw 方块被替换后不保留 NBT
    return result;
}

u32 JigsawReplacementStructureProcessor::parseBlockStateString(const std::string& stateStr)
{
    // 解析方块状态字符串
    // 格式: "minecraft:stone[axis=y,facing=north]" 或 "minecraft:stone"

    size_t bracketPos = stateStr.find('[');
    std::string blockName;
    std::unordered_map<std::string, std::string> properties;

    if (bracketPos == std::string::npos) {
        // 没有属性
        blockName = stateStr;
    } else {
        // 有属性
        blockName = stateStr.substr(0, bracketPos);
        std::string propsStr = stateStr.substr(bracketPos + 1);
        if (!propsStr.empty() && propsStr.back() == ']') {
            propsStr.pop_back();
        }

        // 解析属性
        size_t start = 0;
        size_t end = propsStr.find(',');
        while (start < propsStr.size()) {
            std::string prop =
                (end == std::string::npos) ? propsStr.substr(start) : propsStr.substr(start, end - start);
            size_t eqPos = prop.find('=');
            if (eqPos != std::string::npos) {
                std::string key = prop.substr(0, eqPos);
                std::string value = prop.substr(eqPos + 1);
                properties[key] = value;
            }
            if (end == std::string::npos) break;
            start = end + 1;
            end = propsStr.find(',', start);
        }
    }

    // 获取方块
    auto& registry = BlockRegistry::instance();
    Block* block = registry.getBlock(ResourceLocation(blockName));
    if (!block) {
        return 0; // 空气
    }

    // 获取默认状态
    const BlockState* state = &block->defaultState();

    // 应用属性
    if (!properties.empty()) {
        const auto& container = block->stateContainer();
        std::unordered_map<const IProperty*, size_t> wanted;

        for (const auto& [key, value] : properties) {
            const IProperty* prop = container.getProperty(key);
            if (!prop) continue;

            auto parsedValue = prop->parseValue(value);
            if (!parsedValue) continue;

            wanted[prop] = *parsedValue;
        }

        // 查找匹配的状态
        if (!wanted.empty()) {
            for (const auto& candidate : container.validStates()) {
                if (!candidate) continue;

                bool matches = true;
                for (const auto& [prop, index] : wanted) {
                    const auto valueIndex = candidate->getValueIndex(*prop);
                    if (!valueIndex.has_value() || *valueIndex != index) {
                        matches = false;
                        break;
                    }
                }

                if (matches) {
                    state = candidate.get();
                    break;
                }
            }
        }
    }

    return state ? state->stateId() : 0;
}

IntegrityProcessor::IntegrityProcessor(f32 integrity)
    : m_integrity(integrity)
{}

std::optional<ProcessedBlockInfo> IntegrityProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 使用位置种子创建确定性随机数生成器
    // 关键：使用变换后的世界坐标 (blockInfo.pos)，而非模板内坐标
    u64 seed = math::getPositionRandom(blockInfo.pos.x, blockInfo.pos.y, blockInfo.pos.z);
    math::Random rng(seed);

    // 完整度 >= 1.0：保留所有方块
    // 随机判断是否保留方块：nextFloat() 返回 [0.0, 1.0)，所以 <= integrity 的概率正好是 integrity
    if (m_integrity >= 1.0f) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 随机判断是否保留方块
    // nextFloat() 返回 [0.0, 1.0)，所以 <= integrity 的概率正好是 integrity
    if (rng.nextFloat() <= m_integrity) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 移除方块
    return std::nullopt;
}

// ============================================================================
// RuleStructureProcessor
// ============================================================================

RuleStructureProcessor::RuleStructureProcessor(std::vector<std::unique_ptr<RuleEntry>> rules)
    : m_rules(std::move(rules))
{}

std::optional<ProcessedBlockInfo> RuleStructureProcessor::process(const BlockPos& seedPos,
    const BlockPos& /*pos*/,
    const BlockInfo& rawBlockInfo,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings)
{
    // 遍历所有规则，找到第一个匹配的规则
    // 使用位置种子创建确定性随机数生成器
    u64 seed = math::getPositionRandom(blockInfo.pos.x, blockInfo.pos.y, blockInfo.pos.z);
    math::Random rng(seed);

    // 获取输入方块状态
    const BlockState* inputState = BlockRegistry::instance().getBlockState(rawBlockInfo.blockStateId);

    // 获取世界位置方块状态（通过 PlacementSettings 中的世界访问）。
    // 生产路径 world 总是非空；world 缺失时（仅单元测试）回退到输入方块状态，
    // 使 location 谓词仍可在引用风格 test(const BlockState&) 下求值。
    const BlockState* locationState = inputState;
    const IWorld* world = settings.getWorld();
    if (world) {
        locationState = world->getBlockState(blockInfo.pos);
    }

    for (const auto& rule : m_rules) {
        if (rule && rule->matches(inputState, locationState, rawBlockInfo.pos, blockInfo.pos, seedPos, rng)) {
            // 找到匹配的规则，返回输出方块状态
            ProcessedBlockInfo result;
            result.pos = blockInfo.pos;
            result.blockStateId = rule->outputStateId();
            // 不复制 NBT（规则输出不保留原 NBT）
            return result;
        }
    }

    // 没有规则匹配，保持原样
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

std::unique_ptr<StructureProcessor> RuleStructureProcessor::clone() const
{
    std::vector<std::unique_ptr<RuleEntry>> clonedRules;
    clonedRules.reserve(m_rules.size());
    for (const auto& rule : m_rules) {
        if (rule) {
            clonedRules.push_back(rule->clone());
        }
    }
    return std::make_unique<RuleStructureProcessor>(std::move(clonedRules));
}

// ============================================================================
// NopStructureProcessor
// ============================================================================

std::optional<ProcessedBlockInfo> NopStructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 直接返回原始方块信息
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

// ============================================================================
// LavaSubmergingProcessor
// ============================================================================

std::optional<ProcessedBlockInfo> LavaSubmergingProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings)
{
    // LavaSubmergingProcessor
    // 如果当前位置是岩浆，且模板方块不透明，则替换为岩浆
    const IWorld* world = settings.getWorld();
    if (!world) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 获取当前位置的方块状态
    const BlockState* worldState = world->getBlockState(blockInfo.pos);
    if (!worldState) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 检查当前位置是否是岩浆
    // 方块ID: 岩浆 (flowing_lava = 10, lava = 11)
    u32 worldBlockId = worldState->blockId();
    if (worldBlockId != 10 && worldBlockId != 11) {
        // 不是岩浆，保持原样
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 获取模板方块状态
    const BlockState* templateState = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (!templateState) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 检查模板方块是否不透明
    // 如果不透明，则让岩浆保留；如果透明，则放置模板方块
    bool isOpaque = templateState->isOpaque();

    if (isOpaque) {
        // 不透明方块，岩浆应该被替换为该方块
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 透明方块（如栅栏、楼梯等），让岩浆保留
    // 返回岩浆状态
    ProcessedBlockInfo result;
    result.pos = blockInfo.pos;
    // 使用静止岩浆 (ID = 11)
    result.blockStateId = 11; // lava
    return result;
}

// ============================================================================
// BlockAgeProcessor
// ============================================================================

BlockAgeProcessor::BlockAgeProcessor(f32 mossiness)
    : m_mossiness(mossiness)
{}

std::optional<ProcessedBlockInfo> BlockAgeProcessor::process(const BlockPos& seedPos,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 随机将石砖相关方块替换为苔藓化或裂变版本
    const BlockState* state = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (!state) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    const Block& block = state->getBlock();

    // 使用确定性随机（基于位置）
    u64 hash = math::hashBlockPos(blockInfo.pos.x, blockInfo.pos.y, blockInfo.pos.z);
    math::Random rng(static_cast<u64>(hash) ^ static_cast<u64>(seedPos.x * 31 + seedPos.z * 17));

    const BlockState* newState = nullptr;

    // 石砖类完整方块（石砖、石头、錾刻石砖）
    if ((VanillaBlocks::STONE_BRICKS && &block == VanillaBlocks::STONE_BRICKS) ||
        (VanillaBlocks::STONE && &block == VanillaBlocks::STONE) ||
        (VanillaBlocks::CHISELED_STONE_BRICKS && &block == VanillaBlocks::CHISELED_STONE_BRICKS)) {
        newState = _maybeReplaceFullStoneBlock(rng);
    }
    // 楼梯方块（使用标签匹配，保留 facing/half/shape/waterlogged 属性）
    else if (BlockTags::STAIRS().contains(block)) {
        newState = _maybeReplaceStairs(*state, rng);
    }
    // 台阶方块（使用标签匹配，保留 type/waterlogged 属性）
    else if (BlockTags::SLABS().contains(block)) {
        newState = _maybeReplaceSlab(*state, rng);
    }
    // 墙壁方块（使用标签匹配，保留 up/north/south/east/west/waterlogged 属性）
    else if (BlockTags::WALLS().contains(block)) {
        newState = _maybeReplaceWall(*state, rng);
    }
    // 黑曜石
    else if (VanillaBlocks::OBSIDIAN && &block == VanillaBlocks::OBSIDIAN) {
        newState = _maybeReplaceObsidian(rng);
    }

    if (newState) {
        ProcessedBlockInfo result;
        result.pos = blockInfo.pos;
        result.blockStateId = newState->stateId();
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    // 未被替换，返回原始方块
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

const BlockState* BlockAgeProcessor::_maybeReplaceFullStoneBlock(math::Random& rng)
{
    // 50% 概率不替换
    if (rng.nextFloat() >= PROBABILITY_OF_REPLACING_FULL_BLOCK) {
        return nullptr;
    }

    // 非 mossiness 组候选：裂纹石砖 或 随机朝向的石砖楼梯
    const BlockState* nonMossyOptions[] = {
        VanillaBlocks::CRACKED_STONE_BRICKS ? &VanillaBlocks::CRACKED_STONE_BRICKS->defaultState() : nullptr,
        VanillaBlocks::STONE_BRICK_STAIRS ? &_getRandomFacingStairs(rng, *VanillaBlocks::STONE_BRICK_STAIRS) : nullptr};

    // mossiness 组候选：苔藓石砖 或 随机朝向的苔藓石砖楼梯
    const BlockState* mossyOptions[] = {
        VanillaBlocks::MOSSY_STONE_BRICKS ? &VanillaBlocks::MOSSY_STONE_BRICKS->defaultState() : nullptr,
        VanillaBlocks::MOSSY_STONE_BRICK_STAIRS ? &_getRandomFacingStairs(rng, *VanillaBlocks::MOSSY_STONE_BRICK_STAIRS)
                                                : nullptr};

    return _getRandomBlock(rng, nonMossyOptions, mossyOptions);
}

const BlockState* BlockAgeProcessor::_maybeReplaceStairs(const BlockState& state, math::Random& rng)
{
    // 50% 概率不替换
    if (rng.nextFloat() >= PROBABILITY_OF_REPLACING_STAIRS) {
        return nullptr;
    }

    // mossiness 组候选：苔藓石砖楼梯（保留原属性）或 苔藓石砖台阶（默认状态）
    const BlockState* mossyOptions[] = {VanillaBlocks::MOSSY_STONE_BRICK_STAIRS
            ? &VanillaBlocks::MOSSY_STONE_BRICK_STAIRS->defaultState().withPropertiesOf(state)
            : nullptr,
        VanillaBlocks::MOSSY_STONE_BRICK_SLAB ? &VanillaBlocks::MOSSY_STONE_BRICK_SLAB->defaultState() : nullptr};

    // 非 mossiness 组候选：石台阶 或 石砖台阶（默认状态）
    const BlockState* nonMossyOptions[] = {
        VanillaBlocks::STONE_SLAB ? &VanillaBlocks::STONE_SLAB->defaultState() : nullptr,
        VanillaBlocks::STONE_BRICK_SLAB ? &VanillaBlocks::STONE_BRICK_SLAB->defaultState() : nullptr};

    return _getRandomBlock(rng, nonMossyOptions, mossyOptions);
}

const BlockState* BlockAgeProcessor::_maybeReplaceSlab(const BlockState& state, math::Random& rng)
{
    // mossiness 概率替换为苔藓石砖台阶，保留原属性
    if (rng.nextFloat() < m_mossiness && VanillaBlocks::MOSSY_STONE_BRICK_SLAB) {
        return &VanillaBlocks::MOSSY_STONE_BRICK_SLAB->defaultState().withPropertiesOf(state);
    }
    return nullptr;
}

const BlockState* BlockAgeProcessor::_maybeReplaceWall(const BlockState& state, math::Random& rng)
{
    // mossiness 概率替换为苔藓石砖墙，保留原属性
    if (rng.nextFloat() < m_mossiness && VanillaBlocks::MOSSY_STONE_BRICK_WALL) {
        return &VanillaBlocks::MOSSY_STONE_BRICK_WALL->defaultState().withPropertiesOf(state);
    }
    return nullptr;
}

const BlockState* BlockAgeProcessor::_maybeReplaceObsidian(math::Random& rng)
{
    // 固定 15% 概率替换为哭泣黑曜石
    if (rng.nextFloat() < PROBABILITY_OF_REPLACING_OBSIDIAN && VanillaBlocks::CRYING_OBSIDIAN) {
        return &VanillaBlocks::CRYING_OBSIDIAN->defaultState();
    }
    return nullptr;
}

const BlockState& BlockAgeProcessor::_getRandomFacingStairs(math::Random& rng, const Block& stairsBlock)
{
    // 生成随机朝向的楼梯状态：随机水平朝向 + 随机上半/下半
    const BlockState& defaultState = stairsBlock.defaultState();
    const BlockState* result = &defaultState;

    // 设置随机水平朝向
    static constexpr Direction horizontalDirs[] = {
        Direction::North, Direction::South, Direction::East, Direction::West};
    Direction facing = horizontalDirs[rng.nextInt(4)];
    if (defaultState.hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        result = &defaultState.with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }

    // 设置随机上半/下半
    if (result->hasProperty(BlockStateProperties::HALF())) {
        // Half 枚举值：Bottom(0), Top(1)
        auto half = static_cast<BlockStateProperties::Half>(rng.nextInt(2));
        result = &result->with(BlockStateProperties::HALF(), half);
    }

    return *result;
}

const BlockState* BlockAgeProcessor::_getRandomBlock(
    math::Random& rng, const BlockState* const nonMossy[], const BlockState* const mossy[])
{
    // mossiness 概率选择 mossy 组，否则选择 non-mossy 组
    if (rng.nextFloat() < m_mossiness) {
        return _pickRandomNonNull(rng, mossy, 2);
    }
    return _pickRandomNonNull(rng, nonMossy, 2);
}

const BlockState* BlockAgeProcessor::_pickRandomNonNull(
    math::Random& rng, const BlockState* const options[], size_t count)
{
    // 从选项数组中随机选取一个非空元素
    size_t nonNullCount = 0;
    for (size_t i = 0; i < count; ++i) {
        if (options[i] != nullptr) {
            ++nonNullCount;
        }
    }
    if (nonNullCount == 0) {
        return nullptr;
    }
    size_t targetIndex = static_cast<size_t>(rng.nextInt(static_cast<i32>(nonNullCount)));
    size_t currentIndex = 0;
    for (size_t i = 0; i < count; ++i) {
        if (options[i] != nullptr) {
            if (currentIndex == targetIndex) {
                return options[i];
            }
            ++currentIndex;
        }
    }
    return nullptr;
}

// ============================================================================
// BlackstoneReplacementProcessor
// ============================================================================

BlackstoneReplacementProcessor::BlackstoneReplacementProcessor()
{
    // 黑石替换映射：将普通石质方块替换为黑石变体，用于堡垒遗迹

    auto& registry = BlockRegistry::instance();

    // 辅助lambda：根据名称获取方块ID
    auto getBlockId = [&registry](const char* name) -> u32 {
        ResourceLocation loc(name);
        Block* block = registry.getBlock(loc);
        if (block) {
            return block->blockId();
        }
        return 0; // 空气/未找到
    };

    // 基础方块替换映射
    u32 cobblestone = getBlockId("minecraft:cobblestone");
    u32 blackstone = getBlockId("minecraft:blackstone");
    u32 mossyCobblestone = getBlockId("minecraft:mossy_cobblestone");
    u32 stone = getBlockId("minecraft:stone");
    u32 polishedBlackstone = getBlockId("minecraft:polished_blackstone");
    u32 stoneBricks = getBlockId("minecraft:stone_bricks");
    u32 polishedBlackstoneBricks = getBlockId("minecraft:polished_blackstone_bricks");
    u32 mossyStoneBricks = getBlockId("minecraft:mossy_stone_bricks");
    u32 crackedStoneBricks = getBlockId("minecraft:cracked_stone_bricks");
    u32 crackedPolishedBlackstoneBricks = getBlockId("minecraft:cracked_polished_blackstone_bricks");
    u32 chiseledStoneBricks = getBlockId("minecraft:chiseled_stone_bricks");
    u32 chiseledPolishedBlackstone = getBlockId("minecraft:chiseled_polished_blackstone");
    u32 ironBars = getBlockId("minecraft:iron_bars");
    u32 chain = getBlockId("minecraft:iron_chain");

    // 基础方块替换
    if (cobblestone && blackstone) {
        m_replacements[cobblestone] = blackstone;
    }
    if (mossyCobblestone && blackstone) {
        m_replacements[mossyCobblestone] = blackstone;
    }
    if (stone && polishedBlackstone) {
        m_replacements[stone] = polishedBlackstone;
    }
    if (stoneBricks && polishedBlackstoneBricks) {
        m_replacements[stoneBricks] = polishedBlackstoneBricks;
    }
    if (mossyStoneBricks && polishedBlackstoneBricks) {
        m_replacements[mossyStoneBricks] = polishedBlackstoneBricks;
    }
    if (crackedStoneBricks && crackedPolishedBlackstoneBricks) {
        m_replacements[crackedStoneBricks] = crackedPolishedBlackstoneBricks;
    }
    if (chiseledStoneBricks && chiseledPolishedBlackstone) {
        m_replacements[chiseledStoneBricks] = chiseledPolishedBlackstone;
    }
    if (ironBars && chain) {
        m_replacements[ironBars] = chain;
    }

    // 楼梯替换
    u32 cobblestoneStairs = getBlockId("minecraft:cobblestone_stairs");
    u32 blackstoneStairs = getBlockId("minecraft:blackstone_stairs");
    u32 mossyCobblestoneStairs = getBlockId("minecraft:mossy_cobblestone_stairs");
    u32 stoneStairs = getBlockId("minecraft:stone_stairs");
    u32 polishedBlackstoneStairs = getBlockId("minecraft:polished_blackstone_stairs");
    u32 stoneBrickStairs = getBlockId("minecraft:stone_brick_stairs");
    u32 polishedBlackstoneBrickStairs = getBlockId("minecraft:polished_blackstone_brick_stairs");
    u32 mossyStoneBrickStairs = getBlockId("minecraft:mossy_stone_brick_stairs");

    if (cobblestoneStairs && blackstoneStairs) {
        m_replacements[cobblestoneStairs] = blackstoneStairs;
    }
    if (mossyCobblestoneStairs && blackstoneStairs) {
        m_replacements[mossyCobblestoneStairs] = blackstoneStairs;
    }
    if (stoneStairs && polishedBlackstoneStairs) {
        m_replacements[stoneStairs] = polishedBlackstoneStairs;
    }
    if (stoneBrickStairs && polishedBlackstoneBrickStairs) {
        m_replacements[stoneBrickStairs] = polishedBlackstoneBrickStairs;
    }
    if (mossyStoneBrickStairs && polishedBlackstoneBrickStairs) {
        m_replacements[mossyStoneBrickStairs] = polishedBlackstoneBrickStairs;
    }

    // 台阶替换
    u32 cobblestoneSlab = getBlockId("minecraft:cobblestone_slab");
    u32 blackstoneSlab = getBlockId("minecraft:blackstone_slab");
    u32 mossyCobblestoneSlab = getBlockId("minecraft:mossy_cobblestone_slab");
    u32 smoothStoneSlab = getBlockId("minecraft:smooth_stone_slab");
    u32 stoneSlab = getBlockId("minecraft:stone_slab");
    u32 polishedBlackstoneSlab = getBlockId("minecraft:polished_blackstone_slab");
    u32 stoneBrickSlab = getBlockId("minecraft:stone_brick_slab");
    u32 polishedBlackstoneBrickSlab = getBlockId("minecraft:polished_blackstone_brick_slab");
    u32 mossyStoneBrickSlab = getBlockId("minecraft:mossy_stone_brick_slab");

    if (cobblestoneSlab && blackstoneSlab) {
        m_replacements[cobblestoneSlab] = blackstoneSlab;
    }
    if (mossyCobblestoneSlab && blackstoneSlab) {
        m_replacements[mossyCobblestoneSlab] = blackstoneSlab;
    }
    if (smoothStoneSlab && polishedBlackstoneSlab) {
        m_replacements[smoothStoneSlab] = polishedBlackstoneSlab;
    }
    if (stoneSlab && polishedBlackstoneSlab) {
        m_replacements[stoneSlab] = polishedBlackstoneSlab;
    }
    if (stoneBrickSlab && polishedBlackstoneBrickSlab) {
        m_replacements[stoneBrickSlab] = polishedBlackstoneBrickSlab;
    }
    if (mossyStoneBrickSlab && polishedBlackstoneBrickSlab) {
        m_replacements[mossyStoneBrickSlab] = polishedBlackstoneBrickSlab;
    }

    // 墙替换
    u32 cobblestoneWall = getBlockId("minecraft:cobblestone_wall");
    u32 blackstoneWall = getBlockId("minecraft:blackstone_wall");
    u32 mossyCobblestoneWall = getBlockId("minecraft:mossy_cobblestone_wall");
    u32 stoneBrickWall = getBlockId("minecraft:stone_brick_wall");
    u32 polishedBlackstoneBrickWall = getBlockId("minecraft:polished_blackstone_brick_wall");
    u32 mossyStoneBrickWall = getBlockId("minecraft:mossy_stone_brick_wall");

    if (cobblestoneWall && blackstoneWall) {
        m_replacements[cobblestoneWall] = blackstoneWall;
    }
    if (mossyCobblestoneWall && blackstoneWall) {
        m_replacements[mossyCobblestoneWall] = blackstoneWall;
    }
    if (stoneBrickWall && polishedBlackstoneBrickWall) {
        m_replacements[stoneBrickWall] = polishedBlackstoneBrickWall;
    }
    if (mossyStoneBrickWall && polishedBlackstoneBrickWall) {
        m_replacements[mossyStoneBrickWall] = polishedBlackstoneBrickWall;
    }
}

std::optional<ProcessedBlockInfo> BlackstoneReplacementProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{

    const BlockState* state = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (!state) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    u32 blockId = state->blockId();

    // 查找替换映射
    auto it = m_replacements.find(blockId);
    if (it != m_replacements.end()) {
        ProcessedBlockInfo result;
        result.pos = blockInfo.pos;

        // 获取目标方块
        Block* targetBlock = BlockRegistry::instance().getBlock(it->second);
        if (targetBlock) {
            // 获取目标方块的默认状态
            const BlockState* targetState = &targetBlock->defaultState();
            const Block& sourceBlock = state->getBlock();
            const auto& sourceContainer = sourceBlock.stateContainer();
            const auto& targetContainer = targetBlock->stateContainer();

            // 保持兼容的方块状态属性
            // 参考 BlackStoneReplacementProcessor.java 第54-65行

            // 尝试复制 FACING 属性（用于楼梯、墙等）
            const IProperty* facingProp = sourceContainer.getProperty("facing");
            const IProperty* targetFacingProp = targetContainer.getProperty("facing");
            if (facingProp && targetFacingProp) {
                auto valueIndex = state->getValueIndex(*facingProp);
                if (valueIndex.has_value()) {
                    // 尝试在目标方块上设置相同属性值
                    size_t sourceIndex = *valueIndex;
                    std::string valueStr = facingProp->valueToString(sourceIndex);
                    auto parsedValue = targetFacingProp->parseValue(valueStr);
                    if (parsedValue) {
                        // 遍历目标方块的所有状态，找到具有目标属性值的状态
                        for (const auto& candidate : targetContainer.validStates()) {
                            if (!candidate) continue;
                            auto targetValueIndex = candidate->getValueIndex(*targetFacingProp);
                            if (targetValueIndex.has_value() && *targetValueIndex == *parsedValue) {
                                targetState = candidate.get();
                                break;
                            }
                        }
                    }
                }
            }

            // 尝试复制 HALF 属性（用于楼梯）
            const IProperty* halfProp = sourceContainer.getProperty("half");
            const IProperty* targetHalfProp = targetContainer.getProperty("half");
            if (halfProp && targetHalfProp && targetState) {
                auto valueIndex = state->getValueIndex(*halfProp);
                if (valueIndex.has_value()) {
                    size_t sourceIndex = *valueIndex;
                    std::string valueStr = halfProp->valueToString(sourceIndex);
                    auto parsedValue = targetHalfProp->parseValue(valueStr);
                    if (parsedValue) {
                        for (const auto& candidate : targetContainer.validStates()) {
                            if (!candidate) continue;
                            // 检查是否保持 FACING 值
                            auto targetFacingValueIndex =
                                targetFacingProp ? candidate->getValueIndex(*targetFacingProp) : std::nullopt;
                            auto sourceFacingValueIndex =
                                targetFacingProp ? targetState->getValueIndex(*targetFacingProp) : std::nullopt;
                            bool facingMatches = (targetFacingProp == nullptr) ||
                                (targetFacingValueIndex.has_value() && sourceFacingValueIndex.has_value() &&
                                    *targetFacingValueIndex == *sourceFacingValueIndex);
                            if (!facingMatches) continue;

                            auto targetValueIndex = candidate->getValueIndex(*targetHalfProp);
                            if (targetValueIndex.has_value() && *targetValueIndex == *parsedValue) {
                                targetState = candidate.get();
                                break;
                            }
                        }
                    }
                }
            }

            // 尝试复制 TYPE 属性（用于台阶 - top/bottom/double）
            const IProperty* typeProp = sourceContainer.getProperty("type");
            const IProperty* targetTypeProp = targetContainer.getProperty("type");
            if (typeProp && targetTypeProp && targetState) {
                auto valueIndex = state->getValueIndex(*typeProp);
                if (valueIndex.has_value()) {
                    size_t sourceIndex = *valueIndex;
                    std::string valueStr = typeProp->valueToString(sourceIndex);
                    auto parsedValue = targetTypeProp->parseValue(valueStr);
                    if (parsedValue) {
                        for (const auto& candidate : targetContainer.validStates()) {
                            if (!candidate) continue;
                            auto targetValueIndex = candidate->getValueIndex(*targetTypeProp);
                            if (targetValueIndex.has_value() && *targetValueIndex == *parsedValue) {
                                targetState = candidate.get();
                                break;
                            }
                        }
                    }
                }
            }

            result.blockStateId = targetState ? targetState->stateId() : targetBlock->defaultState().stateId();
        } else {
            result.blockStateId = blockInfo.blockStateId;
        }

        // 复制 NBT（如果有）
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }

        return result;
    }

    // 没有找到替换映射，保持原样
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

void StructureProcessorList::addProcessor(std::unique_ptr<StructureProcessor> processor)
{
    m_processors.push_back(std::move(processor));
}

std::optional<ProcessedBlockInfo> StructureProcessorList::process(const BlockPos& seedPos,
    const BlockPos& pos,
    const BlockInfo& rawBlockInfo,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings) const
{
    // 如果没有处理器，直接返回原始信息
    if (m_processors.empty()) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 按顺序处理
    ProcessedBlockInfo current = ProcessedBlockInfo::fromBlockInfo(blockInfo);

    for (const auto& processor : m_processors) {
        if (!processor) continue;

        // 创建临时BlockInfo用于处理
        BlockInfo currentInfo;
        currentInfo.pos = current.pos;
        currentInfo.blockStateId = current.blockStateId;
        if (current.nbt) {
            currentInfo.nbt = std::make_unique<nbt::CompoundTag>(*current.nbt);
        }

        auto result = processor->process(seedPos, pos, rawBlockInfo, currentInfo, settings);
        if (!result) {
            // 处理器返回nullopt，跳过此方块
            return std::nullopt;
        }
        current = std::move(*result);
    }

    return current;
}

std::unique_ptr<StructureProcessorList> StructureProcessorList::clone() const
{
    auto list = std::make_unique<StructureProcessorList>();
    for (const auto& proc : m_processors) {
        if (proc) {
            list->addProcessor(proc->clone());
        }
    }
    return list;
}

namespace ProcessorLists {

static std::unique_ptr<StructureProcessorList> s_emptyList;

const StructureProcessorList& empty()
{
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
