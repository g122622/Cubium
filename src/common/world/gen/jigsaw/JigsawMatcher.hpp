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
#include "common/util/Direction.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include <optional>
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 连接点匹配器
 *
 * 负责匹配两个 Jigsaw 连接点（名称匹配 + 方向匹配）。
 * 所有方法均为静态工具方法。
 */
class JigsawMatcher {
public:
    /**
     * @brief 检查两个连接点是否可以匹配（仅检查名称）
     *
     * @param sourceTarget 源连接点的目标名称（target 字段）
     * @param targetName 目标连接点的名称（name 字段）
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

        // 目标名称必须匹配
        return sourceTarget == targetName;
    }

    /**
     * @brief 检查两个 Jigsaw 方向是否可以连接
     *
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
     * - 如果 facing 是水平方向，默认为 ALIGNED
     * - 如果 facing 是垂直方向，默认为 ROLLABLE
     *
     * @param orientation Jigsaw 方向
     * @return 默认连接类型
     */
    static JigsawJointType getDefaultJointType(JigsawOrientation orientation)
    {
        Direction facing = JigsawOrientations::getFacing(orientation);
        // 水平方向默认为 ALIGNED，垂直方向默认为 ROLLABLE
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
