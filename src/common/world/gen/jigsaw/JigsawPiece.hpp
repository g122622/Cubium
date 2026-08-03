/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "JigsawTypes.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {

class IWorldWriter;
class IChunkGenerator;

namespace world {
namespace chunk {
class ChunkPrimer;
}

namespace gen {
namespace structure {
class StructureBoundingBox;
}

namespace feature {
namespace template_ {
class TemplateManager;
} // namespace template_
} // namespace feature

namespace jigsaw {

struct PlacedPiece;

/**
 * @brief Jigsaw 拼图块基类
 *
 * 拼图块是 Jigsaw 组装系统的基本单元。每个拼图块对应一个结构模板（SingleJigsawPiece）、
 * 一个地物（FeatureJigsawPiece）、一个子块列表（ListJigsawPiece）或空占位（EmptyJigsawPiece）。
 *
 * 子类通过 place() 多态分发实现各自的放置逻辑，处理器链构造封装在子类内部。
 */
class JigsawPiece {
public:
    virtual ~JigsawPiece() = default;
    virtual const std::string& getTypeName() const = 0;
    virtual std::unique_ptr<JigsawPiece> clone() const = 0;

    /**
     * @brief 放置拼图块到世界
     *
     * 子类实现各自的放置逻辑：
     * - SingleJigsawPiece/LegacySingleJigsawPiece：加载模板并应用处理器链放置
     * - ListJigsawPiece：递归放置子块
     * - FeatureJigsawPiece：调用配置化地物的 place()（需要 chunk 和 generator）
     * - EmptyJigsawPiece：空操作
     *
     * @param world 世界写入器
     * @param placed 已放置的拼图块信息（位置、旋转、边界框等）
     * @param templateManager 模板管理器（用于加载结构模板）
     * @param rng 随机数生成器
     * @param bounds 方块放置裁剪边界；无需裁剪时传入 nullptr
     * @param chunk 区块数据（FeatureJigsawPiece 放置配置化地物时需要，可为 nullptr）
     * @param generator 区块生成器（FeatureJigsawPiece 放置配置化地物时需要，可为 nullptr）
     */
    virtual void place(IWorldWriter& world,
        const PlacedPiece& placed,
        class feature::template_::TemplateManager& templateManager,
        math::Random& rng,
        const structure::StructureBoundingBox* bounds,
        world::chunk::ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) = 0;

    JigsawPlacementBehaviour getPlacementBehaviour() const { return m_placementBehaviour; }
    void setPlacementBehaviour(JigsawPlacementBehaviour behaviour) { m_placementBehaviour = behaviour; }

    virtual i32 getGroundLevelDelta() const { return m_groundLevelDelta; }
    void setGroundLevelDelta(i32 delta) { m_groundLevelDelta = delta; }

    const std::vector<JigsawJoint>& getJoints() const { return m_joints; }
    void addJoint(const JigsawJoint& joint) { m_joints.push_back(joint); }
    void clearJoints() { m_joints.clear(); }

    /**
     * @brief 获取按 selectionPriority 降序稳定排序后的连接点列表
     *
     * 先打乱连接点顺序（Fisher-Yates），再按 selectionPriority 降序稳定排序，
     * 使高优先级连接点先被处理。对应 MC 1.21 的 SinglePoolElement.getShuffledJigsawBlocks()。
     *
     * @param rng 随机数生成器
     * @return 排序后的连接点列表
     */
    std::vector<JigsawJoint> getShuffledJoints(math::Random& rng) const;

    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    virtual bool isEmpty() const { return false; }

    /**
     * @brief 是否为 legacy 类型拼图块
     *
     * Legacy 拼图块使用 STRUCTURE_AND_AIR 忽略策略（忽略空气方块），
     * 而标准拼图块只忽略 STRUCTURE_BLOCK。
     */
    virtual bool isLegacy() const { return false; }

    /**
     * @brief 获取拼图块大小
     */
    virtual BlockPos getSize() const { return BlockPos(1, 1, 1); }

    /**
     * @brief 从模板加载 Jigsaw 方块信息
     *
     * 加载模板并提取所有 Jigsaw 方块作为连接点。
     *
     * @param templateName 模板名称（资源位置）
     * @param joints 输出的连接点列表
     * @param size 输出的模板大小
     * @return 是否成功加载
     */
    virtual bool loadJointsFromTemplate(
        const std::string& templateName, std::vector<JigsawJoint>& joints, BlockPos& size);

protected:
    explicit JigsawPiece(JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid)
        : m_placementBehaviour(behaviour)
    {}

    JigsawPlacementBehaviour m_placementBehaviour = JigsawPlacementBehaviour::Rigid;
    i32 m_groundLevelDelta = 1; // 默认值为 1
    std::vector<JigsawJoint> m_joints;
    std::string m_name;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
