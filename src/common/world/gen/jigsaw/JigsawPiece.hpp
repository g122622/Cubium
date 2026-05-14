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

#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../structure/StructureBoundingBox.hpp"
#include "JigsawOrientation.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief Jigsaw 放置行为
 */
enum class JigsawPlacementBehaviour : u8 {
    TerrainMatching, ///< 匹配地形高度
    Rigid            ///< 固定位置
};

/**
 * @brief Jigsaw 连接类型
 *
 * 参考 MC 1.16.5 JigsawTileEntity.OrientationType
 * - ROLLABLE: 可以旋转连接，只需要facing相反
 * - ALIGNED: 对齐连接，facing相反且rotation必须相同
 */
enum class JigsawJointType : u8 {
    Rollable, ///< 可旋转连接
    Aligned   ///< 对齐连接
};

/**
 * @brief Jigsaw 连接点类型
 *
 * 参考 MC 1.16.5 的 JigsawBlock 名称：
 * - minecraft:bottom, minecraft:top
 * - minecraft:left, minecraft:right
 * - minecraft:front, minecraft:back
 * - 自定义名称如 village/center, village/street 等
 */
struct JigsawTarget {
    std::string name; ///< 连接点名称（如 "minecraft:bottom" 或 "village/center"）
    BlockPos offset;  ///< 相对于拼图块的偏移位置

    JigsawTarget() = default;
    JigsawTarget(const std::string& n, const BlockPos& o)
        : name(n)
        , offset(o)
    {}
};

/**
 * @brief Jigsaw 连接点信息
 *
 * 参考 MC 1.16.5 Template.BlockInfo 中的 Jigsaw 方块数据
 */
struct JigsawJoint {
    BlockPos sourcePos;     ///< 源位置（在拼图块内）
    std::string sourceName; ///< 源连接点名称（nbt中的"name"字段）
    std::string targetPool; ///< 目标模板池名称（nbt中的"pool"字段）
    std::string targetName; ///< 目标连接点名称（nbt中的"target"字段，可以是 "minecraft:empty" 表示终止）
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    JigsawJointType jointType = JigsawJointType::Rollable;      ///< 连接类型
    JigsawOrientation orientation = JigsawOrientation::NorthUp; ///< Jigsaw 方块朝向
    i32 sourceGroundY = 0;                                      ///< 源地面高度

    JigsawJoint() = default;
    JigsawJoint(const BlockPos& src,
        const std::string& srcName,
        const std::string& pool,
        const std::string& tgtName,
        JigsawPlacementBehaviour proj = JigsawPlacementBehaviour::Rigid)
        : sourcePos(src)
        , sourceName(srcName)
        , targetPool(pool)
        , targetName(tgtName)
        , projection(proj)
    {}
};

/**
 * @brief Jigsaw 拼图块基类
 *
 * 参考 MC 1.16.5: JigsawPiece
 */
class JigsawPiece {
public:
    virtual ~JigsawPiece() = default;
    virtual const std::string& getTypeName() const = 0;
    virtual std::unique_ptr<JigsawPiece> clone() const = 0;

    JigsawPlacementBehaviour getPlacementBehaviour() const { return m_placementBehaviour; }
    void setPlacementBehaviour(JigsawPlacementBehaviour behaviour) { m_placementBehaviour = behaviour; }

    virtual i32 getGroundLevelDelta() const { return m_groundLevelDelta; }
    void setGroundLevelDelta(i32 delta) { m_groundLevelDelta = delta; }

    const std::vector<JigsawJoint>& getJoints() const { return m_joints; }
    void addJoint(const JigsawJoint& joint) { m_joints.push_back(joint); }
    void clearJoints() { m_joints.clear(); }

    /**
     * @brief 获取打乱后的连接点列表
     *
     * MC 1.16.5: getJigsawBlocks 返回打乱顺序的连接点
     * 参考: SingleJigsawPiece.getJigsawBlocks() 第91-96行
     *
     * @param rng 随机数生成器
     * @return 打乱后的连接点列表
     */
    std::vector<JigsawJoint> getShuffledJoints(math::Random& rng) const
    {
        std::vector<JigsawJoint> shuffled = m_joints;
        rng.shuffle(shuffled);
        return shuffled;
    }

    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    virtual bool isEmpty() const { return false; }

    /**
     * @brief 获取拼图块大小
     */
    virtual BlockPos getSize() const { return BlockPos(1, 1, 1); }

    /**
     * @brief 从模板加载 Jigsaw 方块信息
     *
     * 参考 MC 1.16.5: JigsawPiece.getJigsawBlocks
     * 加载模板并提取所有 Jigsaw 方块作为连接点
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
    i32 m_groundLevelDelta = 1; // MC 1.16.5 默认值为 1
    std::vector<JigsawJoint> m_joints;
    std::string m_name;
};

/**
 * @brief 空拼图块
 */
class EmptyJigsawPiece : public JigsawPiece {
public:
    static EmptyJigsawPiece& instance();

    const std::string& getTypeName() const override { return s_typeName; }
    std::unique_ptr<JigsawPiece> clone() const override;
    bool isEmpty() const override { return true; }
    BlockPos getSize() const override { return BlockPos(0, 0, 0); }

    // MC 1.16.5: EmptyJigsawPiece 是单例，但 clone() 需要返回有效指针
    // 参考: EmptyJigsawPiece.java - INSTANCE 单例
    EmptyJigsawPiece()
        : JigsawPiece(JigsawPlacementBehaviour::Rigid)
    {}

private:
    static std::string s_typeName;
};

/**
 * @brief 单模板拼图块
 */
class SingleJigsawPiece : public JigsawPiece {
public:
    explicit SingleJigsawPiece(
        const std::string& templateName, JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid);

    const std::string& getTypeName() const override { return s_typeName; }
    const std::string& getTemplateName() const { return m_templateName; }
    std::unique_ptr<JigsawPiece> clone() const override
    {
        auto piece = std::make_unique<SingleJigsawPiece>(m_templateName, getPlacementBehaviour());
        piece->setGroundLevelDelta(getGroundLevelDelta());
        for (const auto& joint : m_joints) {
            piece->addJoint(joint);
        }
        return piece;
    }

    BlockPos getSize() const override { return m_size; }
    void setSize(const BlockPos& size) { m_size = size; }

private:
    std::string m_templateName;
    BlockPos m_size;
    static std::string s_typeName;
};

/**
 * @brief 列表拼图块（包含多个子块）
 */
class ListJigsawPiece : public JigsawPiece {
public:
    explicit ListJigsawPiece(JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid);

    const std::string& getTypeName() const override { return s_typeName; }
    std::unique_ptr<JigsawPiece> clone() const override;

    void addPiece(std::unique_ptr<JigsawPiece> piece);
    const std::vector<std::unique_ptr<JigsawPiece>>& getPieces() const { return m_pieces; }
    size_t getPieceCount() const { return m_pieces.size(); }

private:
    std::vector<std::unique_ptr<JigsawPiece>> m_pieces;
    static std::string s_typeName;
};

/**
 * @brief 连接点匹配器
 *
 * 负责匹配两个 Jigsaw 连接点
 * 参考 MC 1.16.5: JigsawBlock.func_220171_a
 */
class JigsawMatcher {
public:
    /**
     * @brief 检查两个连接点是否可以匹配（仅检查名称）
     *
     * @param sourceTarget 源连接点的目标名称（target字段）
     * @param targetName 目标连接点的名称（name字段）
     * @return 是否可以连接
     */
    static bool canMatchByName(const std::string& sourceTarget, const std::string& targetName)
    {
        // 空连接点永远不匹配
        if (sourceTarget.empty() || targetName.empty()) {
            return false;
        }

        // minecraft:empty 表示终止点
        if (sourceTarget == "minecraft:empty" || targetName == "minecraft:empty") {
            return false;
        }

        // 目标名称必须匹配（MC中是target.target == source.name）
        return sourceTarget == targetName;
    }

    /**
     * @brief 检查两个 Jigsaw 方向是否可以连接
     *
     * 参考 MC 1.16.5: JigsawBlock.func_220171_a
     * 连接条件：
     * 1. facing 方向必须相反（面对面）
     * 2. 如果是 rollable 类型，则只需 facing 相反
     * 3. 如果是 aligned 类型，rotation 方向也必须相同
     *
     * @param sourceOrientation 源 Jigsaw 方向
     * @param targetOrientation 目标 Jigsaw 方向
     * @param sourceJointType 源连接类型（rollable 或 aligned）
     * @return 是否可以连接
     */
    static bool canConnectOrientation(
        JigsawOrientation sourceOrientation, JigsawOrientation targetOrientation, JigsawJointType sourceJointType)
    {
        Direction sourceFacing = JigsawOrientations::getFacing(sourceOrientation);
        Direction targetFacing = JigsawOrientations::getFacing(targetOrientation);

        // 条件1: facing 必须相反（面对面）
        if (Directions::opposite(sourceFacing) != targetFacing) {
            return false;
        }

        // 如果是 rollable 类型，只需 facing 相反
        if (sourceJointType == JigsawJointType::Rollable) {
            return true;
        }

        // aligned 类型：rotation 方向也必须相同
        Direction sourceRotation = JigsawOrientations::getRotation(sourceOrientation);
        Direction targetRotation = JigsawOrientations::getRotation(targetOrientation);
        return sourceRotation == targetRotation;
    }

    /**
     * @brief 完整检查两个连接点是否可以匹配
     *
     * @param sourceTarget 源连接点的目标名称
     * @param targetName 目标连接点的名称
     * @param sourceOrientation 源 Jigsaw 方向
     * @param targetOrientation 目标 Jigsaw 方向
     * @param sourceJointType 源连接类型
     * @return 是否可以连接
     */
    static bool canMatch(const std::string& sourceTarget,
        const std::string& targetName,
        JigsawOrientation sourceOrientation,
        JigsawOrientation targetOrientation,
        JigsawJointType sourceJointType)
    {
        // 检查名称匹配
        if (!canMatchByName(sourceTarget, targetName)) {
            return false;
        }

        // 检查方向匹配
        return canConnectOrientation(sourceOrientation, targetOrientation, sourceJointType);
    }

    /**
     * @brief 从连接点方向确定默认连接类型
     *
     * 参考 MC 1.16.5: JigsawTileEntity.OrientationType.func_235673_a_
     * - 如果 facing 是水平方向，默认为 ALIGNED
     * - 如果 facing 是垂直方向，默认为 ROLLABLE
     *
     * @param orientation Jigsaw 方向
     * @return 默认连接类型
     */
    static JigsawJointType getDefaultJointType(JigsawOrientation orientation)
    {
        Direction facing = JigsawOrientations::getFacing(orientation);
        // MC 1.16.5: direction.getAxis().isHorizontal() ? ALIGNED : ROLLABLE
        if (facing == Direction::North || facing == Direction::South || facing == Direction::East ||
            facing == Direction::West) {
            return JigsawJointType::Aligned;
        }
        return JigsawJointType::Rollable;
    }

    /**
     * @brief 从字符串获取连接类型
     *
     * @param jointStr 连接类型字符串 ("rollable" 或 "aligned")
     * @return 连接类型，如果无效则返回 nullopt
     */
    static std::optional<JigsawJointType> jointTypeFromString(const std::string& jointStr)
    {
        if (jointStr == "rollable") {
            return JigsawJointType::Rollable;
        } else if (jointStr == "aligned") {
            return JigsawJointType::Aligned;
        }
        return std::nullopt;
    }

    /**
     * @brief 连接类型转字符串
     */
    static std::string jointTypeToString(JigsawJointType type)
    {
        return type == JigsawJointType::Rollable ? "rollable" : "aligned";
    }

    /**
     * @brief 获取旋转后的连接点名称
     *
     * @param name 原始名称
     * @param rotation 旋转角度 (0, 90, 180, 270)
     * @return 旋转后的名称
     */
    static std::string rotateName(const std::string& name, i32 rotation)
    {
        if (rotation == 0 || name.empty()) {
            return name;
        }

        // 标准 Minecraft 方向连接点
        if (name == "minecraft:top" || name == "minecraft:bottom") {
            // top/bottom 不受水平旋转影响
            return name;
        }

        // front/back/left/right 受旋转影响
        if (name == "minecraft:front") {
            switch (rotation) {
                case 90:
                    return "minecraft:right";
                case 180:
                    return "minecraft:back";
                case 270:
                    return "minecraft:left";
                default:
                    return name;
            }
        }
        if (name == "minecraft:right") {
            switch (rotation) {
                case 90:
                    return "minecraft:back";
                case 180:
                    return "minecraft:left";
                case 270:
                    return "minecraft:front";
                default:
                    return name;
            }
        }
        if (name == "minecraft:back") {
            switch (rotation) {
                case 90:
                    return "minecraft:left";
                case 180:
                    return "minecraft:front";
                case 270:
                    return "minecraft:right";
                default:
                    return name;
            }
        }
        if (name == "minecraft:left") {
            switch (rotation) {
                case 90:
                    return "minecraft:front";
                case 180:
                    return "minecraft:right";
                case 270:
                    return "minecraft:back";
                default:
                    return name;
            }
        }

        // 非标准名称保持不变
        return name;
    }
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
