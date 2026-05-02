#pragma once

#include "../../../../core/Types.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../../util/nbt/Nbt.hpp"
#include "../../../../util/Direction.hpp"
#include "../../structure/StructureBoundingBox.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "RuleTest.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <unordered_set>

namespace mc {

class BlockState;
class IWorldWriter;

namespace world {
namespace gen {
namespace feature {
namespace template_ {

// 使用 Direction.hpp 中定义的 Rotation 和 Mirror 枚举
using mc::Rotation;
using mc::Mirror;

// 前向声明
class StructureProcessor;
class StructureProcessorList;

/**
 * @brief 模板方块信息
 */
struct BlockInfo {
    BlockPos pos;
    u32 blockStateId = 0;  // 使用 BlockState 的 ID 来避免完整类型定义
    std::unique_ptr<nbt::CompoundTag> nbt;

    BlockInfo();
    BlockInfo(const BlockPos& p, u32 stateId);
    BlockInfo(const BlockInfo& other);
    BlockInfo(BlockInfo&& other) noexcept;
    BlockInfo& operator=(const BlockInfo& other);
    BlockInfo& operator=(BlockInfo&& other) noexcept;
    ~BlockInfo();
};

/**
 * @brief 处理后的方块信息
 *
 * 参考 MC 1.16.5 Template.BlockInfo
 * 用于处理器链处理后返回的结果
 */
struct ProcessedBlockInfo {
    BlockPos pos;
    u32 blockStateId = 0;
    std::unique_ptr<nbt::CompoundTag> nbt;

    ProcessedBlockInfo() = default;
    ProcessedBlockInfo(const BlockPos& p, u32 stateId) : pos(p), blockStateId(stateId) {}
    ProcessedBlockInfo(const ProcessedBlockInfo& other);
    ProcessedBlockInfo(ProcessedBlockInfo&& other) noexcept;
    ProcessedBlockInfo& operator=(const ProcessedBlockInfo& other);
    ProcessedBlockInfo& operator=(ProcessedBlockInfo&& other) noexcept;
    ~ProcessedBlockInfo();

    /**
     * @brief 从 BlockInfo 创建 ProcessedBlockInfo
     * @param info 源方块信息
     * @return 处理后的方块信息
     */
    static ProcessedBlockInfo fromBlockInfo(const BlockInfo& info) {
        ProcessedBlockInfo result;
        result.pos = info.pos;
        result.blockStateId = info.blockStateId;
        if (info.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*info.nbt);
        }
        return result;
    }
};

/**
 * @brief 放置设置
 *
 * 参考 MC 1.16.5 PlacementSettings
 */
class PlacementSettings {
public:
    PlacementSettings();

    [[nodiscard]] Rotation getRotation() const { return m_rotation; }
    PlacementSettings& setRotation(Rotation rotation);

    [[nodiscard]] Mirror getMirror() const { return m_mirror; }
    PlacementSettings& setMirror(Mirror mirror);

    [[nodiscard]] bool ignoreEntities() const { return m_ignoreEntities; }
    PlacementSettings& setIgnoreEntities(bool ignore);

    [[nodiscard]] const structure::StructureBoundingBox* getBoundingBox() const { return m_boundingBox; }
    PlacementSettings& setBoundingBox(const structure::StructureBoundingBox* bounds);

    [[nodiscard]] const BlockPos& getCenterOffset() const { return m_centerOffset; }
    PlacementSettings& setCenterOffset(const BlockPos& offset);

    [[nodiscard]] u32 getBlockUpdateFlags() const { return m_blockUpdateFlags; }
    PlacementSettings& setBlockUpdateFlags(u32 flags);

    [[nodiscard]] bool keepLiquids() const { return m_keepLiquids; }
    PlacementSettings& setKeepLiquids(bool keep);

    [[nodiscard]] PlacementSettings copy() const;

    [[nodiscard]] const StructureProcessorList* getProcessors() const { return m_processors; }
    PlacementSettings& setProcessors(const StructureProcessorList* processors);

private:
    Rotation m_rotation = Rotation::None;
    Mirror m_mirror = Mirror::None;
    bool m_ignoreEntities = false;
    bool m_keepLiquids = false;
    const structure::StructureBoundingBox* m_boundingBox = nullptr;
    BlockPos m_centerOffset = BlockPos(0, 0, 0);
    u32 m_blockUpdateFlags = 18;  // 默认标志：更新邻居和通知观察者
    const StructureProcessorList* m_processors = nullptr;
};

/**
 * @brief 结构处理器接口
 *
 * 参考 MC 1.16.5 StructureProcessor
 * 处理器可以修改或过滤模板中的方块
 */
class StructureProcessor {
public:
    virtual ~StructureProcessor() = default;

    /**
     * @brief 处理方块信息
     *
     * 参考 MC 1.16.5 StructureProcessor.process
     * @param seedPos 种子位置
     * @param pos 当前放置位置
     * @param rawBlockInfo 原始方块信息（未变换）
     * @param blockInfo 变换后的方块信息
     * @param settings 放置设置
     * @return 处理后的方块信息，或 nullopt 表示跳过此方块
     */
    [[nodiscard]] virtual std::optional<ProcessedBlockInfo> process(
        const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings);

    /**
     * @brief 获取处理器类型ID
     */
    [[nodiscard]] virtual u32 getProcessorType() const { return 0; }
};

/**
 * @brief 结构处理器列表
 *
 * 参考 MC 1.16.5 StructureProcessorList
 */
class StructureProcessorList {
public:
    StructureProcessorList() = default;

    void addProcessor(std::unique_ptr<StructureProcessor> processor);
    [[nodiscard]] size_t size() const { return m_processors.size(); }
    [[nodiscard]] bool empty() const { return m_processors.empty(); }

    // 迭代器支持
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::const_iterator begin() const { return m_processors.begin(); }
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::const_iterator end() const { return m_processors.end(); }
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::iterator begin() { return m_processors.begin(); }
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::iterator end() { return m_processors.end(); }

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(
        const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) const;

private:
    std::vector<std::unique_ptr<StructureProcessor>> m_processors;
};

/**
 * @brief Jigsaw 连接点信息（用于结构模板）
 */
struct TemplateJigsawBlockInfo {
    BlockPos pos;
    String name;
    String targetPool;
    String targetName;
    i32 jointType = 0;  // 0=rollable, 1=aligned

    TemplateJigsawBlockInfo() = default;
    TemplateJigsawBlockInfo(const BlockPos& p, const String& n, const String& pool, const String& tgt, i32 joint = 0)
        : pos(p), name(n), targetPool(pool), targetName(tgt), jointType(joint) {}
};

/**
 * @brief 实体信息
 *
 * 参考 MC 1.16.5 Template.EntityInfo
 * 包含两个位置：
 * - pos: 精确位置（Double 列表），用于实体精确放置
 * - blockPos: 方块坐标（Int 列表），用于方块对齐
 */
struct TemplateEntityInfo {
    String typeId;
    f64 posx = 0.0;  // 精确位置 X
    f64 posy = 0.0;  // 精确位置 Y
    f64 posz = 0.0;  // 精确位置 Z
    BlockPos blockPos;  // 方块坐标
    std::unique_ptr<nbt::CompoundTag> nbt;

    TemplateEntityInfo();
    TemplateEntityInfo(const TemplateEntityInfo& other);
    TemplateEntityInfo(TemplateEntityInfo&& other) noexcept;
    TemplateEntityInfo& operator=(const TemplateEntityInfo& other);
    TemplateEntityInfo& operator=(TemplateEntityInfo&& other) noexcept;
    ~TemplateEntityInfo();
};

/**
 * @brief 结构模板
 */
class Template {
public:
    Template();
    ~Template();

    [[nodiscard]] const BlockPos& getSize() const { return m_size; }
    void setSize(const BlockPos& size) { m_size = size; }
    void addBlock(const BlockInfo& blockInfo);
    void addJigsawBlock(const TemplateJigsawBlockInfo& jigsawInfo);
    void addEntity(const TemplateEntityInfo& entityInfo);

    [[nodiscard]] const std::vector<BlockInfo>& getBlocks() const { return m_blocks; }
    [[nodiscard]] const std::vector<TemplateJigsawBlockInfo>& getJigsawBlocks() const { return m_jigsawBlocks; }
    [[nodiscard]] const std::vector<TemplateEntityInfo>& getEntities() const { return m_entities; }
    [[nodiscard]] size_t getBlockCount() const { return m_blocks.size(); }
    [[nodiscard]] size_t getJigsawBlockCount() const { return m_jigsawBlocks.size(); }

    [[nodiscard]] structure::StructureBoundingBox getBoundingBox(
        const PlacementSettings& settings,
        const BlockPos& pos) const;

    bool place(
        IWorldWriter& world,
        const BlockPos& pos,
        const PlacementSettings& settings,
        math::Random& rng,
        u32 flags = 18) const;

    [[nodiscard]] static BlockPos transformBlockPos(
        const BlockPos& pos,
        Mirror mirror,
        Rotation rotation,
        const BlockPos& center);

    [[nodiscard]] static BlockPos getTransformedPosition(
        const BlockPos& pos,
        Rotation rotation,
        const BlockPos& size);

private:
    BlockPos m_size;
    std::vector<BlockInfo> m_blocks;
    std::vector<TemplateJigsawBlockInfo> m_jigsawBlocks;
    std::vector<TemplateEntityInfo> m_entities;
};

/**
 * @brief 重力结构处理器
 *
 * 参考 MC 1.16.5 GravityStructureProcessor
 */
class GravityStructureProcessor : public StructureProcessor {
public:
    explicit GravityStructureProcessor(i32 heightmapType = 0, i32 offset = 0);

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(
        const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] i32 heightmapType() const { return m_heightmapType; }
    [[nodiscard]] i32 offset() const { return m_offset; }

private:
    i32 m_heightmapType;
    i32 m_offset;
};

/**
 * @brief 方块忽略结构处理器
 *
 * 参考 MC 1.16.5 BlockIgnoreStructureProcessor
 */
class BlockIgnoreStructureProcessor : public StructureProcessor {
public:
    explicit BlockIgnoreStructureProcessor(const std::vector<u32>& blocksToIgnore = {});

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(
        const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] const std::unordered_set<u32>& blocksToIgnore() const { return m_blocksToIgnore; }

private:
    std::unordered_set<u32> m_blocksToIgnore;
};

/**
 * @brief Jigsaw 替换结构处理器
 *
 * 参考 MC 1.16.5 JigsawReplacementStructureProcessor
 * 当遇到 Jigsaw 方块时，读取其 final_state NBT 字段，
 * 解析并替换为对应的方块状态。
 * 如果 final_state 是 structure_void，则跳过此方块。
 */
class JigsawReplacementStructureProcessor : public StructureProcessor {
public:
    JigsawReplacementStructureProcessor();

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(
        const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

private:
    /**
     * @brief 解析方块状态字符串
     * @param stateStr 方块状态字符串，如 "minecraft:stone[axis=y]"
     * @return 方块状态 ID，失败返回 0（空气）
     */
    [[nodiscard]] static u32 parseBlockStateString(const String& stateStr);
};

/**
 * @brief 完整度结构处理器
 *
 * 参考 MC 1.16.5 IntegrityProcessor
 */
class IntegrityProcessor : public StructureProcessor {
public:
    explicit IntegrityProcessor(f32 integrity);

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(
        const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

private:
    f32 m_integrity;
};

/**
 * @brief 规则结构处理器
 *
 * 参考 MC 1.16.5 RuleStructureProcessor
 * 根据一组规则来替换方块，每条规则包含：
 * - inputPredicate: 测试输入方块（模板中的方块）
 * - locationPredicate: 测试位置方块（世界中已有的方块）
 * - posPredicate: 测试位置条件
 * - outputState: 匹配时的输出方块状态
 */
class RuleStructureProcessor : public StructureProcessor {
public:
    /**
     * @brief 构造规则处理器
     * @param rules 规则列表
     */
    explicit RuleStructureProcessor(std::vector<std::unique_ptr<RuleEntry>> rules);

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(
        const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] const std::vector<std::unique_ptr<RuleEntry>>& rules() const { return m_rules; }

private:
    std::vector<std::unique_ptr<RuleEntry>> m_rules;
};

/**
 * @brief 预定义处理器列表
 */
namespace ProcessorLists {
    [[nodiscard]] const StructureProcessorList& empty();
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
