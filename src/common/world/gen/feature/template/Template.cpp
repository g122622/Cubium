#include "Template.hpp"
#include "RuleTest.hpp"
#include "../../../../world/IWorldWriter.hpp"
#include "../../../../world/block/BlockRegistry.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include <algorithm>
#include <unordered_map>

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
// ProcessedBlockInfo
// ============================================================================

ProcessedBlockInfo::ProcessedBlockInfo(const ProcessedBlockInfo& other)
    : pos(other.pos), blockStateId(other.blockStateId)
{
    if (other.nbt) {
        nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
    }
}

ProcessedBlockInfo::ProcessedBlockInfo(ProcessedBlockInfo&& other) noexcept
    : pos(std::move(other.pos))
    , blockStateId(other.blockStateId)
    , nbt(std::move(other.nbt))
{
}

ProcessedBlockInfo& ProcessedBlockInfo::operator=(const ProcessedBlockInfo& other) {
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

ProcessedBlockInfo& ProcessedBlockInfo::operator=(ProcessedBlockInfo&& other) noexcept {
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

std::optional<ProcessedBlockInfo> StructureProcessor::process(
    const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 默认实现：不修改，直接返回处理后的方块信息
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

// ============================================================================
// TemplateEntityInfo
// ============================================================================

TemplateEntityInfo::TemplateEntityInfo() : posx(0.0), posy(0.0), posz(0.0), blockPos()
{
}

TemplateEntityInfo::TemplateEntityInfo(const TemplateEntityInfo& other)
    : typeId(other.typeId)
    , posx(other.posx), posy(other.posy), posz(other.posz)
    , blockPos(other.blockPos)
{
    if (other.nbt) {
        nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
    }
}

TemplateEntityInfo::TemplateEntityInfo(TemplateEntityInfo&& other) noexcept
    : typeId(std::move(other.typeId))
    , posx(other.posx), posy(other.posy), posz(other.posz)
    , blockPos(std::move(other.blockPos))
    , nbt(std::move(other.nbt))
{
}

TemplateEntityInfo& TemplateEntityInfo::operator=(const TemplateEntityInfo& other) {
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

TemplateEntityInfo& TemplateEntityInfo::operator=(TemplateEntityInfo&& other) noexcept {
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

PlacementSettings& PlacementSettings::setCenterOffset(const BlockPos& offset) {
    m_centerOffset = offset;
    return *this;
}

PlacementSettings& PlacementSettings::setBlockUpdateFlags(u32 flags) {
    m_blockUpdateFlags = flags;
    return *this;
}

PlacementSettings& PlacementSettings::setKeepLiquids(bool keep) {
    m_keepLiquids = keep;
    return *this;
}

PlacementSettings PlacementSettings::copy() const {
    PlacementSettings result;
    result.m_rotation = m_rotation;
    result.m_mirror = m_mirror;
    result.m_ignoreEntities = m_ignoreEntities;
    result.m_keepLiquids = m_keepLiquids;
    result.m_boundingBox = m_boundingBox;
    result.m_centerOffset = m_centerOffset;
    result.m_blockUpdateFlags = m_blockUpdateFlags;
    result.m_processors = m_processors;
    return result;
}

PlacementSettings& PlacementSettings::setProcessors(const StructureProcessorList* processors) {
    m_processors = processors;
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

    return structure::StructureBoundingBox(
        pos.x, pos.y, pos.z,
        pos.x + sizeX - 1,
        pos.y + m_size.y - 1,  // Y尺寸不变
        pos.z + sizeZ - 1
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

    // MC 1.16.5: 首先处理方块信息（应用处理器链）
    std::vector<ProcessedBlockInfo> processedBlocks;
    processedBlocks.reserve(m_blocks.size());

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

        // 创建待处理的方块信息
        BlockInfo blockInfo(worldPos, block.blockStateId);
        if (block.nbt) {
            blockInfo.nbt = std::make_unique<nbt::CompoundTag>(*block.nbt);
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
                    // 创建原始方块信息（未变换的）
                    BlockInfo rawInfo(block.pos, block.blockStateId);
                    if (block.nbt) {
                        rawInfo.nbt = std::make_unique<nbt::CompoundTag>(*block.nbt);
                    }

                    auto result = processor->process(pos, worldPos, rawInfo, blockInfo, settings);
                    if (!result) {
                        shouldKeep = false;
                        break;  // 处理器返回空，跳过此方块
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
                continue;  // 跳过此方块
            }
        }

        processedBlocks.push_back(std::move(processedBlock));
    }

    // 放置所有处理后的方块
    for (const auto& processedBlock : processedBlocks) {
        // 检查边界框
        if (bounds) {
            if (processedBlock.pos.x < bounds->minX() || processedBlock.pos.x > bounds->maxX() ||
                processedBlock.pos.y < bounds->minY() || processedBlock.pos.y > bounds->maxY() ||
                processedBlock.pos.z < bounds->minZ() || processedBlock.pos.z > bounds->maxZ()) {
                continue;  // 跳过边界外的方块
            }
        }

        // 获取方块状态
        const BlockState* state = BlockRegistry::instance().getBlockState(processedBlock.blockStateId);
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
        world.setBlock(processedBlock.pos.x, processedBlock.pos.y, processedBlock.pos.z, transformedState, static_cast<i32>(flags));

        // 方块实体数据在区块反序列化阶段统一处理，此处仅负责方块状态放置
        // TODO: 处理方块实体 NBT 数据（更新位置坐标）
        (void)processedBlock.nbt;
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
    : m_heightmapType(heightmapType)
    , m_offset(offset)
{
}

std::optional<ProcessedBlockInfo> GravityStructureProcessor::process(
    const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // TODO: 需要访问世界高度图来获取地面高度
    // 当前实现：保持原位置，偏移offset
    // 完整实现需要IWorldReader访问高度图
    ProcessedBlockInfo result = ProcessedBlockInfo::fromBlockInfo(blockInfo);
    result.pos = BlockPos(blockInfo.pos.x, blockInfo.pos.y + m_offset, blockInfo.pos.z);
    return result;
}

BlockIgnoreStructureProcessor::BlockIgnoreStructureProcessor(const std::vector<u32>& blocksToIgnore)
    : m_blocksToIgnore(blocksToIgnore)
{
}

std::optional<ProcessedBlockInfo> BlockIgnoreStructureProcessor::process(
    const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 检查方块是否在忽略列表中
    for (u32 ignoreId : m_blocksToIgnore) {
        if (blockInfo.blockStateId == ignoreId) {
            return std::nullopt;  // 跳过此方块
        }
    }

    // 保留方块
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

JigsawReplacementStructureProcessor::JigsawReplacementStructureProcessor()
{
}

std::optional<ProcessedBlockInfo> JigsawReplacementStructureProcessor::process(
    const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // MC 1.16.5: JigsawReplacementStructureProcessor
    // 检查方块是否为 Jigsaw 方块
    // 如果是，读取 NBT 中的 final_state 字段并解析为新的方块状态

    const BlockState* state = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (!state) {
        ProcessedBlockInfo result;
        result.pos = blockInfo.pos;
        result.blockStateId = blockInfo.blockStateId;
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    // 检查是否是 Jigsaw 方块
    const Block& block = state->getBlock();
    ResourceLocation blockId = block.blockLocation();
    if (blockId.toString() != "minecraft:jigsaw") {
        // 不是 Jigsaw 方块，保持原样
        ProcessedBlockInfo result;
        result.pos = blockInfo.pos;
        result.blockStateId = blockInfo.blockStateId;
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
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

    const String& finalStateStr = dynamic_cast<const nbt::StringTag&>(*it->second).value;

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

u32 JigsawReplacementStructureProcessor::parseBlockStateString(const String& stateStr) {
    // 解析方块状态字符串
    // 格式: "minecraft:stone[axis=y,facing=north]" 或 "minecraft:stone"

    size_t bracketPos = stateStr.find('[');
    String blockName;
    std::unordered_map<String, String> properties;

    if (bracketPos == String::npos) {
        // 没有属性
        blockName = stateStr;
    } else {
        // 有属性
        blockName = stateStr.substr(0, bracketPos);
        String propsStr = stateStr.substr(bracketPos + 1);
        if (!propsStr.empty() && propsStr.back() == ']') {
            propsStr.pop_back();
        }

        // 解析属性
        size_t start = 0;
        size_t end = propsStr.find(',');
        while (start < propsStr.size()) {
            String prop = (end == String::npos) ? propsStr.substr(start) : propsStr.substr(start, end - start);
            size_t eqPos = prop.find('=');
            if (eqPos != String::npos) {
                String key = prop.substr(0, eqPos);
                String value = prop.substr(eqPos + 1);
                properties[key] = value;
            }
            if (end == String::npos) break;
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
                    const auto it = candidate->values().find(prop);
                    if (it == candidate->values().end() || it->second != index) {
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
{
}

std::optional<ProcessedBlockInfo> IntegrityProcessor::process(
    const BlockPos& /*seedPos*/,
    const BlockPos& pos,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 根据完整度概率决定是否保留方块
    // 使用位置哈希作为随机源，确保同一位置的方块在相同种子下行为一致
    u64 hash = math::hashBlockPos(pos.x, pos.y, pos.z);

    // 将哈希映射到 [0.0, 1.0) 范围
    f32 chance = static_cast<f32>((hash & 0xFFFFFFFF) % 10000) / 10000.0f;

    // MC 1.16.5: 如果随机值 >= 完整度，则跳过方块
    // 完整度 1.0 = 保留所有方块
    // 完整度 0.0 = 移除所有方块
    if (chance >= m_integrity) {
        return std::nullopt;  // 跳过此方块（模拟损坏）
    }

    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

// ============================================================================
// RuleStructureProcessor
// ============================================================================

RuleStructureProcessor::RuleStructureProcessor(std::vector<std::unique_ptr<RuleEntry>> rules)
    : m_rules(std::move(rules))
{
}

std::optional<ProcessedBlockInfo> RuleStructureProcessor::process(
    const BlockPos& seedPos,
    const BlockPos& /*pos*/,
    const BlockInfo& rawBlockInfo,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // MC 1.16.5: RuleStructureProcessor.func_230386_a_
    // 遍历所有规则，找到第一个匹配的规则
    // 创建确定性随机数生成器（基于位置）
    u64 hash = math::hashBlockPos(blockInfo.pos.x, blockInfo.pos.y, blockInfo.pos.z);
    math::Random rng(static_cast<u64>(hash));

    // 获取输入方块状态
    const BlockState* inputState = BlockRegistry::instance().getBlockState(rawBlockInfo.blockStateId);

    // 获取世界位置方块状态（需要通过 IWorldReader 访问，当前简化处理）
    // TODO: 需要访问世界来获取位置方块状态
    const BlockState* locationState = nullptr;

    for (const auto& rule : m_rules) {
        if (rule && rule->matches(
            inputState,
            locationState,
            rawBlockInfo.pos,
            blockInfo.pos,
            seedPos,
            rng))
        {
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

void StructureProcessorList::addProcessor(std::unique_ptr<StructureProcessor> processor) {
    m_processors.push_back(std::move(processor));
}

std::optional<ProcessedBlockInfo> StructureProcessorList::process(
    const BlockPos& seedPos,
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
