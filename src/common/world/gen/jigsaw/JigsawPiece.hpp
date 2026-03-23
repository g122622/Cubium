#pragma once

#include "../../../core/Types.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../structure/StructureBoundingBox.hpp"
#include <memory>
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
    TerrainMatching,  ///< 匹配地形高度
    Rigid             ///< 固定位置
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
    String name;         ///< 连接点名称（如 "minecraft:bottom" 或 "village/center"）
    BlockPos offset;     ///< 相对于拼图块的偏移位置

    JigsawTarget() = default;
    JigsawTarget(const String& n, const BlockPos& o) : name(n), offset(o) {}
};

/**
 * @brief Jigsaw 连接点信息
 */
struct JigsawJoint {
    BlockPos sourcePos;                 ///< 源位置（在拼图块内）
    String sourceName;                  ///< 源连接点名称
    String targetPool;                  ///< 目标模板池名称
    String targetName;                  ///< 目标连接点名称（可以是 "minecraft:empty" 表示终止）
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    i32 sourceGroundY = 0;              ///< 源地面高度

    JigsawJoint() = default;
    JigsawJoint(const BlockPos& src, const String& srcName, const String& pool, const String& tgtName,
                JigsawPlacementBehaviour proj = JigsawPlacementBehaviour::Rigid)
        : sourcePos(src), sourceName(srcName), targetPool(pool), targetName(tgtName), projection(proj) {}
};

/**
 * @brief Jigsaw 拼图块基类
 */
class JigsawPiece {
public:
    virtual ~JigsawPiece() = default;
    virtual const String& getTypeName() const = 0;
    virtual std::unique_ptr<JigsawPiece> clone() const = 0;

    JigsawPlacementBehaviour getPlacementBehaviour() const { return m_placementBehaviour; }
    void setPlacementBehaviour(JigsawPlacementBehaviour behaviour) { m_placementBehaviour = behaviour; }

    virtual i32 getGroundLevelDelta() const { return m_groundLevelDelta; }
    void setGroundLevelDelta(i32 delta) { m_groundLevelDelta = delta; }

    const std::vector<JigsawJoint>& getJoints() const { return m_joints; }
    void addJoint(const JigsawJoint& joint) { m_joints.push_back(joint); }
    void clearJoints() { m_joints.clear(); }

    const String& getName() const { return m_name; }
    void setName(const String& name) { m_name = name; }

    virtual bool isEmpty() const { return false; }

    /**
     * @brief 获取拼图块大小
     */
    virtual BlockPos getSize() const { return BlockPos(1, 1, 1); }

protected:
    explicit JigsawPiece(JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid)
        : m_placementBehaviour(behaviour) {}

    JigsawPlacementBehaviour m_placementBehaviour = JigsawPlacementBehaviour::Rigid;
    i32 m_groundLevelDelta = 0;
    std::vector<JigsawJoint> m_joints;
    String m_name;
};

/**
 * @brief 空拼图块
 */
class EmptyJigsawPiece : public JigsawPiece {
public:
    static EmptyJigsawPiece& instance();

    const String& getTypeName() const override { return s_typeName; }
    std::unique_ptr<JigsawPiece> clone() const override;
    bool isEmpty() const override { return true; }
    BlockPos getSize() const override { return BlockPos(0, 0, 0); }

private:
    EmptyJigsawPiece() : JigsawPiece(JigsawPlacementBehaviour::Rigid) {}
    static String s_typeName;
};

/**
 * @brief 单模板拼图块
 */
class SingleJigsawPiece : public JigsawPiece {
public:
    explicit SingleJigsawPiece(const String& templateName,
                               JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid);

    const String& getTypeName() const override { return s_typeName; }
    const String& getTemplateName() const { return m_templateName; }
    std::unique_ptr<JigsawPiece> clone() const override {
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
    String m_templateName;
    BlockPos m_size;
    static String s_typeName;
};

/**
 * @brief 列表拼图块（包含多个子块）
 */
class ListJigsawPiece : public JigsawPiece {
public:
    explicit ListJigsawPiece(JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid);

    const String& getTypeName() const override { return s_typeName; }
    std::unique_ptr<JigsawPiece> clone() const override;

    void addPiece(std::unique_ptr<JigsawPiece> piece);
    const std::vector<std::unique_ptr<JigsawPiece>>& getPieces() const { return m_pieces; }
    size_t getPieceCount() const { return m_pieces.size(); }

private:
    std::vector<std::unique_ptr<JigsawPiece>> m_pieces;
    static String s_typeName;
};

/**
 * @brief 连接点匹配器
 *
 * 负责匹配两个 Jigsaw 连接点
 */
class JigsawMatcher {
public:
    /**
     * @brief 检查两个连接点是否可以匹配
     *
     * @param sourceName 源连接点名称
     * @param targetName 目标连接点名称
     * @return 是否可以连接
     */
    static bool canMatch(const String& sourceName, const String& targetName) {
        // 空连接点永远不匹配
        if (sourceName.empty() || targetName.empty()) {
            return false;
        }

        // minecraft:empty 表示终止点
        if (sourceName == "minecraft:empty" || targetName == "minecraft:empty") {
            return false;
        }

        // 相同名称的连接点可以匹配
        if (sourceName == targetName) {
            return true;
        }

        // 对称连接点匹配（如 bottom/top, left/right, front/back）
        if (isOpposite(sourceName, targetName)) {
            return true;
        }

        return false;
    }

    /**
     * @brief 检查两个名称是否是对称的
     */
    static bool isOpposite(const String& name1, const String& name2) {
        // 标准 Minecraft 对称连接点
        if ((name1 == "minecraft:bottom" && name2 == "minecraft:top") ||
            (name1 == "minecraft:top" && name2 == "minecraft:bottom")) {
            return true;
        }
        if ((name1 == "minecraft:left" && name2 == "minecraft:right") ||
            (name1 == "minecraft:right" && name2 == "minecraft:left")) {
            return true;
        }
        if ((name1 == "minecraft:front" && name2 == "minecraft:back") ||
            (name1 == "minecraft:back" && name2 == "minecraft:front")) {
            return true;
        }

        return false;
    }

    /**
     * @brief 获取旋转后的连接点名称
     *
     * @param name 原始名称
     * @param rotation 旋转角度 (0, 90, 180, 270)
     * @return 旋转后的名称
     */
    static String rotateName(const String& name, i32 rotation) {
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
                case 90: return "minecraft:right";
                case 180: return "minecraft:back";
                case 270: return "minecraft:left";
                default: return name;
            }
        }
        if (name == "minecraft:right") {
            switch (rotation) {
                case 90: return "minecraft:back";
                case 180: return "minecraft:left";
                case 270: return "minecraft:front";
                default: return name;
            }
        }
        if (name == "minecraft:back") {
            switch (rotation) {
                case 90: return "minecraft:left";
                case 180: return "minecraft:front";
                case 270: return "minecraft:right";
                default: return name;
            }
        }
        if (name == "minecraft:left") {
            switch (rotation) {
                case 90: return "minecraft:front";
                case 180: return "minecraft:right";
                case 270: return "minecraft:back";
                default: return name;
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
