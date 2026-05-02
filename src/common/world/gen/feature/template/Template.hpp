#pragma once

#include "../../../../core/Types.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../../util/nbt/Nbt.hpp"
#include "../../../../util/Direction.hpp"
#include "../../structure/StructureBoundingBox.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <vector>
#include <memory>

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

    /**
     * @brief 获取中心偏移
     *
     * 参考 MC 1.16.5 PlacementSettings.func_215219_d
     * 用于旋转变换的中心点
     */
    [[nodiscard]] const BlockPos& getCenterOffset() const { return m_centerOffset; }
    PlacementSettings& setCenterOffset(const BlockPos& offset);

    /**
     * @brief 获取方块更新标志
     *
     * 参考 MC 1.16.5 PlacementSettings.func_215217_f
     * @return 放置方块时的更新标志
     */
    [[nodiscard]] u32 getBlockUpdateFlags() const { return m_blockUpdateFlags; }
    PlacementSettings& setBlockUpdateFlags(u32 flags);

    /**
     * @brief 检查是否保留液体
     *
     * 参考 MC 1.16.5 PlacementSettings.func_215218_i
     */
    [[nodiscard]] bool keepLiquids() const { return m_keepLiquids; }
    PlacementSettings& setKeepLiquids(bool keep);

    /**
     * @brief 复制放置设置
     *
     * 参考 MC 1.16.5 PlacementSettings.func_215216_h
     */
    [[nodiscard]] PlacementSettings copy() const;

private:
    Rotation m_rotation = Rotation::None;
    Mirror m_mirror = Mirror::None;
    bool m_ignoreEntities = false;
    bool m_keepLiquids = false;
    const structure::StructureBoundingBox* m_boundingBox = nullptr;
    BlockPos m_centerOffset = BlockPos(0, 0, 0);
    u32 m_blockUpdateFlags = 18;  // 默认标志：更新邻居和通知观察者
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
     * @param world 世界读取器（可选）
     * @param seedPos 种子位置
     * @param pos 当前放置位置
     * @param rawBlockInfo 原始方块信息（未变换）
     * @param blockInfo 变换后的方块信息
     * @param settings 放置设置
     * @param template_ 模板（可选）
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
     *
     * 参考 MC 1.16.5 IStructureProcessorType
     */
    [[nodiscard]] virtual u32 getProcessorType() const { return 0; }
};

/**
 * @brief 结构处理器列表
 */
class StructureProcessorList {
public:
    void addProcessor(std::unique_ptr<StructureProcessor> processor);

private:
    std::vector<std::unique_ptr<StructureProcessor>> m_processors;
};

/**
 * @brief Jigsaw 连接点信息（用于结构模板）
 */
struct TemplateJigsawBlockInfo {
    BlockPos pos;           ///< 连接点位置
    String name;            ///< 连接点名称 (如 "minecraft:bottom")
    String targetPool;      ///< 目标模板池
    String targetName;      ///< 目标连接点名称
    i32 jointType = 0;      ///< 连接类型: 0=rollable, 1=aligned

    TemplateJigsawBlockInfo() = default;
    TemplateJigsawBlockInfo(const BlockPos& p, const String& n, const String& pool, const String& tgt, i32 joint = 0)
        : pos(p), name(n), targetPool(pool), targetName(tgt), jointType(joint) {}
};

/**
 * @brief 实体信息
 */
struct TemplateEntityInfo {
    String typeId;          ///< 实体类型 ID
    BlockPos pos;           ///< 实体位置
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
 */
class GravityStructureProcessor : public StructureProcessor {
public:
    explicit GravityStructureProcessor(i32 heightmapType = 0, i32 offset = 0);
};

/**
 * @brief 方块忽略结构处理器
 */
class BlockIgnoreStructureProcessor : public StructureProcessor {
public:
    explicit BlockIgnoreStructureProcessor(const std::vector<u32>& blocksToIgnore = {});
};

/**
 * @brief Jigsaw 替换结构处理器
 */
class JigsawReplacementStructureProcessor : public StructureProcessor {
public:
    JigsawReplacementStructureProcessor();
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
