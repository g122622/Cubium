#pragma once

#include "../../../../core/Types.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../../util/nbt/Nbt.hpp"
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
 */
class PlacementSettings {
public:
    PlacementSettings();

    [[nodiscard]] i32 getRotation() const { return m_rotation; }
    PlacementSettings& setRotation(i32 rotation);

    [[nodiscard]] i32 getMirror() const { return m_mirror; }
    PlacementSettings& setMirror(i32 mirror);

    [[nodiscard]] bool ignoreEntities() const { return m_ignoreEntities; }
    PlacementSettings& setIgnoreEntities(bool ignore);

    [[nodiscard]] const structure::StructureBoundingBox* getBoundingBox() const { return m_boundingBox; }
    PlacementSettings& setBoundingBox(const structure::StructureBoundingBox* bounds);

private:
    i32 m_rotation = 0;
    i32 m_mirror = 0;  // 0=none, 1=x, 2=z
    bool m_ignoreEntities = false;
    const structure::StructureBoundingBox* m_boundingBox = nullptr;
};

/**
 * @brief 结构处理器接口
 */
class StructureProcessor {
public:
    virtual ~StructureProcessor() = default;
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
        i32 mirror,
        i32 rotation,
        const BlockPos& center);

    [[nodiscard]] static BlockPos getTransformedPosition(
        const BlockPos& pos,
        i32 rotation,
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
