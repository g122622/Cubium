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

#pragma once

#include "RuleTest.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {

class BlockState;
class Block;
class IWorldWriter;
class IWorld;

namespace world {
namespace gen {
namespace feature {
namespace template_ {

// 使用 Direction.hpp 中定义的 Rotation 和 Mirror 枚举
using mc::Mirror;
using mc::Rotation;

// 前向声明
class StructureProcessor;
class StructureProcessorList;

/**
 * @brief 模板方块信息
 */
struct BlockInfo {
    BlockPos pos;
    u32 blockStateId = 0; // 使用 BlockState 的 ID 来避免完整类型定义
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
 */
struct ProcessedBlockInfo {
    BlockPos pos;
    u32 blockStateId = 0;
    std::unique_ptr<nbt::CompoundTag> nbt;

    ProcessedBlockInfo() = default;
    ProcessedBlockInfo(const BlockPos& p, u32 stateId)
        : pos(p)
        , blockStateId(stateId)
    {}
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
    static ProcessedBlockInfo fromBlockInfo(const BlockInfo& info)
    {
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

    /**
     * @brief 获取世界读取器（用于 GravityStructureProcessor 等需要高度信息的处理器）
     */
    [[nodiscard]] const IWorld* getWorld() const { return m_world; }
    PlacementSettings& setWorld(const IWorld* world)
    {
        m_world = world;
        return *this;
    }

    /**
     * @brief 获取确定性随机数生成器
     * 如果设置了预设随机数，则返回预设的；否则基于位置种子创建
     *
     * @param pos 位置种子
     * @return 随机数生成器
     */
    [[nodiscard]] math::Random getRandom(const BlockPos& pos) const;

    /**
     * @brief 设置预设随机数生成器
     *
     * 当需要固定随机序列时使用
     */
    PlacementSettings& setRandom(math::Random* random)
    {
        m_random = random;
        return *this;
    }

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
    u32 m_blockUpdateFlags = 18; // 默认标志：更新邻居和通知观察者
    const StructureProcessorList* m_processors = nullptr;
    const IWorld* m_world = nullptr;  // 可选的世界读取器
    math::Random* m_random = nullptr; // 可选的预设随机数生成器
};

/**
 * @brief 结构处理器接口
 * 处理器可以修改或过滤模板中的方块
 */
class StructureProcessor {
public:
    virtual ~StructureProcessor() = default;

    /**
     * @brief 处理方块信息
     * @param seedPos 种子位置
     * @param pos 当前放置位置
     * @param rawBlockInfo 原始方块信息（未变换）
     * @param blockInfo 变换后的方块信息
     * @param settings 放置设置
     * @return 处理后的方块信息，或 nullopt 表示跳过此方块
     */
    [[nodiscard]] virtual std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings);

    /**
     * @brief 后处理阶段：在所有方块的 process() 调用完成后，对完整方块列表进行批量处理
     *
     * 此方法在所有方块逐一经过 process() 处理完毕后被调用，允许处理器对完整列表做
     * 跨方块批量操作（如 CappedStructureProcessor 限制替换次数）。
     * 默认实现直接返回 processedBlocks，不做任何修改。
     *
     * @param seedPos 结构放置的锚点位置
     * @param settings 放置设置
     * @param originalBlocks 原始方块信息列表（变换坐标前，对应模板内坐标和状态）
     * @param processedBlocks 经处理器链处理后的方块信息列表，可直接修改
     * @return 处理后的方块信息列表（通常返回修改后的 processedBlocks）
     */
    [[nodiscard]] virtual std::vector<ProcessedBlockInfo> finalizeProcessing(const BlockPos& seedPos,
        const PlacementSettings& settings,
        const std::vector<BlockInfo>& originalBlocks,
        std::vector<ProcessedBlockInfo> processedBlocks);

    /**
     * @brief 克隆处理器
     *
     * 用于在放置链路中组合多个处理器列表时深拷贝处理器。
     */
    [[nodiscard]] virtual std::unique_ptr<StructureProcessor> clone() const = 0;

    /**
     * @brief 获取处理器类型ID
     */
    [[nodiscard]] virtual u32 getProcessorType() const { return 0; }
};

/**
 * @brief 结构处理器列表
 */
class StructureProcessorList {
public:
    StructureProcessorList() = default;

    void addProcessor(std::unique_ptr<StructureProcessor> processor);
    [[nodiscard]] size_t size() const { return m_processors.size(); }
    [[nodiscard]] bool empty() const { return m_processors.empty(); }

    /**
     * @brief 深拷贝处理器列表
     *
     * 用于在放置链路中组合多个处理器列表时创建独立副本，
     * 避免修改原始注册表中的处理器。
     */
    [[nodiscard]] std::unique_ptr<StructureProcessorList> clone() const;

    /**
     * @brief 获取处理器列表的只读引用
     *
     * 用于遍历处理器进行克隆或查询。
     */
    [[nodiscard]] const std::vector<std::unique_ptr<StructureProcessor>>& getProcessors() const { return m_processors; }

    // 迭代器支持
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::const_iterator begin() const
    {
        return m_processors.begin();
    }
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::const_iterator end() const
    {
        return m_processors.end();
    }
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::iterator begin() { return m_processors.begin(); }
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::iterator end() { return m_processors.end(); }

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) const;

private:
    std::vector<std::unique_ptr<StructureProcessor>> m_processors;
};

/**
 * @brief Jigsaw 连接点信息（用于结构模板）
 *
 * 对应 MC 1.21 StructureTemplate.JigsawBlockInfo record。
 * selectionPriority/placementPriority 从 jigsaw 方块实体 NBT 读取
 * （"selection_priority"/"placement_priority"，默认 0）。
 */
struct TemplateJigsawBlockInfo {
    BlockPos pos;
    std::string name;
    std::string targetPool;
    std::string targetName;
    i32 jointType = 0;         // 0=rollable, 1=aligned
    u32 blockStateId = 0;      // 方块状态ID，用于读取 orientation 属性
    i32 placementPriority = 0; // 放置优先级（组装队列降序出队）
    i32 selectionPriority = 0; // 选择优先级（同一拼图块内降序排序）

    TemplateJigsawBlockInfo() = default;
    TemplateJigsawBlockInfo(const BlockPos& p,
        const std::string& n,
        const std::string& pool,
        const std::string& tgt,
        i32 joint = 0,
        u32 stateId = 0,
        i32 placementPrio = 0,
        i32 selectionPrio = 0)
        : pos(p)
        , name(n)
        , targetPool(pool)
        , targetName(tgt)
        , jointType(joint)
        , blockStateId(stateId)
        , placementPriority(placementPrio)
        , selectionPriority(selectionPrio)
    {}
};

/**
 * @brief 实体信息
 * 包含两个位置：
 * - pos: 精确位置（Double 列表），用于实体精确放置
 * - blockPos: 方块坐标（Int 列表），用于方块对齐
 */
struct TemplateEntityInfo {
    std::string typeId;
    f64 posx = 0.0;    // 精确位置 X
    f64 posy = 0.0;    // 精确位置 Y
    f64 posz = 0.0;    // 精确位置 Z
    BlockPos blockPos; // 方块坐标
    std::unique_ptr<nbt::CompoundTag> nbt;

    TemplateEntityInfo();
    TemplateEntityInfo(const TemplateEntityInfo& other);
    TemplateEntityInfo(TemplateEntityInfo&& other) noexcept;
    TemplateEntityInfo& operator=(const TemplateEntityInfo& other);
    TemplateEntityInfo& operator=(TemplateEntityInfo&& other) noexcept;
    ~TemplateEntityInfo();
};

/**
 * @brief 模板调色板
 * 存储一组方块信息，并提供按方块类型快速查找的缓存。
 * 一个模板可以有多个调色板，用于结构变体。
 */
class Palette {
public:
    Palette() = default;
    explicit Palette(std::vector<BlockInfo> blocks);

    /**
     * @brief 获取所有方块信息
     */
    [[nodiscard]] const std::vector<BlockInfo>& blocks() const { return m_blocks; }

    /**
     * @brief 获取指定方块类型的所有方块信息
     * @param block 方块类型
     * @return 匹配的方块信息列表
     */
    [[nodiscard]] const std::vector<const BlockInfo*>& getBlocksByType(const Block& block) const;

    /**
     * @brief 获取方块数量
     */
    [[nodiscard]] size_t size() const { return m_blocks.size(); }

    /**
     * @brief 是否为空
     */
    [[nodiscard]] bool empty() const { return m_blocks.empty(); }

private:
    std::vector<BlockInfo> m_blocks;
    mutable std::unordered_map<const Block*, std::vector<const BlockInfo*>> m_blockTypeCache;
    mutable bool m_cacheBuilt = false;

    void _buildCache() const;
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

    /**
     * @brief 添加调色板
     */
    void addPalette(Palette palette);

    /**
     * @brief 获取调色板数量
     */
    [[nodiscard]] size_t getPaletteCount() const { return m_palettes.size(); }

    /**
     * @brief 获取指定索引的调色板
     */
    [[nodiscard]] const Palette* getPalette(size_t index) const;

    /**
     * @brief 选择一个调色板
     * @param rng 随机数生成器
     * @return 选中的调色板，如果没有调色板返回 nullptr
     */
    [[nodiscard]] const Palette* selectPalette(math::Random& rng) const;

    /**
     * @brief 获取第一个调色板的方块（兼容旧接口）
     * @deprecated 使用 getPalette() 或 selectPalette() 代替
     */
    [[nodiscard]] const std::vector<BlockInfo>& getBlocks() const;

    void addJigsawBlock(const TemplateJigsawBlockInfo& jigsawInfo);
    void addEntity(const TemplateEntityInfo& entityInfo);

    [[nodiscard]] const std::vector<TemplateJigsawBlockInfo>& getJigsawBlocks() const { return m_jigsawBlocks; }
    [[nodiscard]] const std::vector<TemplateEntityInfo>& getEntities() const { return m_entities; }
    [[nodiscard]] size_t getBlockCount() const;
    [[nodiscard]] size_t getJigsawBlockCount() const { return m_jigsawBlocks.size(); }

    [[nodiscard]] structure::StructureBoundingBox getBoundingBox(
        const PlacementSettings& settings, const BlockPos& pos) const;

    /**
     * @brief 放置模板到世界（基础版本）
     *
     * 仅放置方块，不处理液体、TileEntity 和实体。
     * 适用于不需要完整世界访问的场景。
     *
     * @param world 世界写入器
     * @param pos 放置位置
     * @param settings 放置设置
     * @param rng 随机数生成器
     * @param flags 方块更新标志
     * @return 是否成功放置
     */
    bool place(IWorldWriter& world,
        const BlockPos& pos,
        const PlacementSettings& settings,
        math::Random& rng,
        u32 flags = 18) const;

    /**
     * @brief 放置模板到世界（完整版本）
     * 完整实现包括：
     * - 方块放置
     * - 液体处理（水、岩浆填充容器）
     * - TileEntity NBT 更新（位置坐标、战利品表种子）
     * - 实体创建（生物、物品等）
     *
     * @param world 世界接口（需要完整的读写访问）
     * @param pos 放置位置
     * @param settings 放置设置
     * @param rng 随机数生成器
     * @param flags 方块更新标志
     * @return 是否成功放置
     */
    bool placeInWorld(
        IWorld& world, const BlockPos& pos, const PlacementSettings& settings, math::Random& rng, u32 flags = 18) const;

    [[nodiscard]] static BlockPos transformBlockPos(
        const BlockPos& pos, Mirror mirror, Rotation rotation, const BlockPos& center);

    /**
     * @brief 变换实体精确位置（f64）
     *
     * 对应 MC 1.21.11 StructureTemplate#transform(Vec3, Mirror, Rotation, BlockPos)。
     * 实体位置使用浮点坐标，因此镜像使用 `1.0 - coord`（而非方块位置的 `-coord`），
     * 旋转公式也包含 `+1` 偏移以保持子方块对齐（block-corner 坐标系）。
     *
     * 与 transformBlockPos 的区别：
     * - transformBlockPos：方块坐标（整数），镜像 `-coord`，旋转无 +1 偏移
     * - transformEntityPos：实体坐标（f64），镜像 `1.0 - coord`，旋转含 +1 偏移
     *
     * @param pos 模板内实体精确位置（f64）
     * @param mirror 镜像
     * @param rotation 旋转
     * @param pivot 旋转中心（通常为 BlockPos(0,0,0)）
     * @return 变换后的精确位置（f64，尚未加上世界偏移）
     */
    [[nodiscard]] static math::Vector3d transformEntityPos(
        const math::Vector3d& pos, Mirror mirror, Rotation rotation, const BlockPos& pivot);

    [[nodiscard]] static BlockPos getTransformedPosition(const BlockPos& pos, Rotation rotation, const BlockPos& size);

private:
    BlockPos m_size;
    std::vector<Palette> m_palettes; // 支持多调色板
    std::vector<TemplateJigsawBlockInfo> m_jigsawBlocks;
    std::vector<TemplateEntityInfo> m_entities;
};

/**
 * @brief 重力结构处理器
 */
class GravityStructureProcessor : public StructureProcessor {
public:
    explicit GravityStructureProcessor(i32 heightmapType = 0, i32 offset = 0);

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<GravityStructureProcessor>(m_heightmapType, m_offset);
    }

    [[nodiscard]] i32 heightmapType() const { return m_heightmapType; }
    [[nodiscard]] i32 offset() const { return m_offset; }

private:
    i32 m_heightmapType;
    i32 m_offset;
};

/**
 * @brief 方块忽略结构处理器
 */
class BlockIgnoreStructureProcessor : public StructureProcessor {
public:
    explicit BlockIgnoreStructureProcessor(const std::vector<u32>& blocksToIgnore = {});

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<BlockIgnoreStructureProcessor>(
            std::vector<u32>(m_blocksToIgnore.begin(), m_blocksToIgnore.end()));
    }

    [[nodiscard]] const std::unordered_set<u32>& blocksToIgnore() const { return m_blocksToIgnore; }

private:
    std::unordered_set<u32> m_blocksToIgnore;
};

/**
 * @brief Jigsaw 替换结构处理器
 * 当遇到 Jigsaw 方块时，读取其 final_state NBT 字段，
 * 解析并替换为对应的方块状态。
 * 如果 final_state 是 structure_void，则跳过此方块。
 */
class JigsawReplacementStructureProcessor : public StructureProcessor {
public:
    JigsawReplacementStructureProcessor();

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<JigsawReplacementStructureProcessor>();
    }

private:
    /**
     * @brief 解析方块状态字符串
     * @param stateStr 方块状态字符串，如 "minecraft:stone[axis=y]"
     * @return 方块状态 ID，失败返回 0（空气）
     */
    [[nodiscard]] static u32 parseBlockStateString(const std::string& stateStr);
};

/**
 * @brief 完整度结构处理器
 */
class IntegrityProcessor : public StructureProcessor {
public:
    explicit IntegrityProcessor(f32 integrity);

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<IntegrityProcessor>(m_integrity);
    }

private:
    f32 m_integrity;
};

/**
 * @brief 规则结构处理器
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

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override;

    [[nodiscard]] const std::vector<std::unique_ptr<RuleEntry>>& rules() const { return m_rules; }

private:
    std::vector<std::unique_ptr<RuleEntry>> m_rules;
};

/**
 * @brief 空操作结构处理器
 * 直接返回原始方块信息，不做任何修改。
 * 主要用于测试或作为占位符。
 */
class NopStructureProcessor : public StructureProcessor {
public:
    NopStructureProcessor() = default;

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<NopStructureProcessor>();
    }
};

/**
 * @brief 岩浆淹没结构处理器
 * 当结构放置在岩浆中时，如果方块是非固体（非不透明），则将其替换为岩浆。
 * 用于堡垒遗迹等在下界岩浆海中生成的结构。
 */
class LavaSubmergingProcessor : public StructureProcessor {
public:
    LavaSubmergingProcessor() = default;

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<LavaSubmergingProcessor>();
    }
};

/**
 * @brief 方块老化结构处理器（苔藓化处理器）
 * 根据苔藓概率随机将石砖相关方块替换为苔藓化或裂变版本。
 * 用于村庄等结构的老化效果。
 */
class BlockAgeProcessor : public StructureProcessor {
public:
    /**
     * @brief 构造方块老化处理器
     * @param mossiness 苔藓化概率 (0.0 - 1.0)
     */
    explicit BlockAgeProcessor(f32 mossiness);

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<BlockAgeProcessor>(m_mossiness);
    }

    [[nodiscard]] f32 mossiness() const { return m_mossiness; }

private:
    f32 m_mossiness;

    /// 尝试替换完整石砖方块（石砖、石头、錾刻石砖）
    /// 可能替换为裂纹石砖、苔藓石砖、随机朝向石砖楼梯或苔藓石砖楼梯
    [[nodiscard]] const BlockState* _maybeReplaceFullStoneBlock(math::Random& rng);

    /// 尝试替换楼梯方块，使用 withPropertiesOf 保留原朝向/半部分属性
    [[nodiscard]] const BlockState* _maybeReplaceStairs(const BlockState& state, math::Random& rng);

    /// 尝试替换台阶方块，使用 withPropertiesOf 保留原类型属性
    [[nodiscard]] const BlockState* _maybeReplaceSlab(const BlockState& state, math::Random& rng);

    /// 尝试替换墙壁方块，使用 withPropertiesOf 保留原连接属性
    [[nodiscard]] const BlockState* _maybeReplaceWall(const BlockState& state, math::Random& rng);

    /// 尝试替换黑曜石为哭泣黑曜石
    [[nodiscard]] static const BlockState* _maybeReplaceObsidian(math::Random& rng);

    /// 生成随机朝向的楼梯状态（随机水平朝向 + 随机上半/下半）
    [[nodiscard]] static const BlockState& _getRandomFacingStairs(math::Random& rng, const Block& stairsBlock);

    /// 根据 mossiness 概率从两组候选中随机选择
    [[nodiscard]] const BlockState* _getRandomBlock(
        math::Random& rng, const BlockState* const nonMossy[], const BlockState* const mossy[]);

    /// 从选项数组中随机选取一个非空元素
    [[nodiscard]] static const BlockState* _pickRandomNonNull(
        math::Random& rng, const BlockState* const options[], size_t count);
};

/**
 * @brief 黑石替换结构处理器
 * 将普通石质方块替换为黑石变体，用于堡垒遗迹的结构生成。
 * 替换映射：
 * - 圆石 -> 黑石
 * - 石头 -> 磨制黑石
 * - 石砖 -> 磨制黑石砖
 * - 等...
 */
class BlackstoneReplacementProcessor : public StructureProcessor {
public:
    BlackstoneReplacementProcessor();

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<BlackstoneReplacementProcessor>();
    }

private:
    // 方块替换映射表：输入方块ID -> 输出方块ID
    std::unordered_map<u32, u32> m_replacements;
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
