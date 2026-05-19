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

#include "UnifiedResource.hpp"
#include <map>
#include <optional>
#include <vector>

namespace mc {
namespace resource {
namespace compat {
namespace unified {

/**
 * @brief 模型面朝向
 */
enum class Direction : u8 { Down, Up, North, South, West, East };

/**
 * @brief 将朝向转换为字符串
 */
[[nodiscard]] inline std::string directionToString(Direction dir)
{
    switch (dir) {
        case Direction::Down:
            return "down";
        case Direction::Up:
            return "up";
        case Direction::North:
            return "north";
        case Direction::South:
            return "south";
        case Direction::West:
            return "west";
        case Direction::East:
            return "east";
        default:
            return "down";
    }
}

/**
 * @brief 将字符串转换为朝向
 */
[[nodiscard]] inline Direction stringToDirection(const std::string& str)
{
    if (str == "up") return Direction::Up;
    if (str == "down") return Direction::Down;
    if (str == "north") return Direction::North;
    if (str == "south") return Direction::South;
    if (str == "west") return Direction::West;
    if (str == "east") return Direction::East;
    return Direction::Down;
}

/**
 * @brief 模型面 UV 坐标
 */
struct ModelFaceUV {
    f32 u1 = 0.0f;  ///< 最小 U (0-16)
    f32 v1 = 0.0f;  ///< 最小 V (0-16)
    f32 u2 = 16.0f; ///< 最大 U (0-16)
    f32 v2 = 16.0f; ///< 最大 V (0-16)
};

/**
 * @brief 模型面定义
 */
struct ModelFace {
    Direction direction = Direction::Down;
    std::string texture; ///< 纹理变量引用（例如 "#all"）
    ModelFaceUV uv;
    i32 rotation = 0;      ///< 面旋转角度（0、90、180、270 度）
    i32 tintIndex = -1;    ///< 生物群系着色索引（-1 = 无着色）
    bool cullFace = false; ///< 是否对相邻方块进行面剔除
};

/**
 * @brief 模型元素（立方体）
 */
struct ModelElement {
    f32 fromX = 0, fromY = 0, fromZ = 0;                               ///< 起始位置 (0-16)
    f32 toX = 16, toY = 16, toZ = 16;                                  ///< 结束位置 (0-16)
    std::vector<ModelFace> faces;                                      ///< 元素面
    i32 rotationAngle = 0;                                             ///< 旋转角度
    std::string rotationAxis = "y";                                    ///< 旋转轴 (x, y, z)
    f32 rotationOriginX = 8, rotationOriginY = 8, rotationOriginZ = 8; ///< 旋转原点
    bool shade = true;                                                 ///< 应用阴影
};

/**
 * @brief GUI 光照模式
 */
enum class GuiLight : u8 {
    Side, ///< 侧面光照（默认）
    Front ///< GUI 物品正面光照
};

/**
 * @brief 统一模型表示
 *
 * 模型被解析为版本无关的格式，
 * 可供渲染后端使用。
 */
struct UnifiedModel : public UnifiedResource {
    std::string parent;                          ///< 父模型引用
    std::map<std::string, std::string> textures; ///< 纹理变量
    std::vector<ModelElement> elements;          ///< 几何元素
    std::optional<GuiLight> guiLight;            ///< GUI 光照模式
    bool ambientOcclusion = true;                ///< 环境光遮蔽

    UnifiedModel() { type = ResourceType::Model; }

    /**
     * @brief 解析纹理引用
     *
     * @param textureRef 纹理引用（例如 "#all", "block/stone"）
     * @return 解析后的纹理路径
     */
    std::string resolveTexture(const std::string& textureRef) const
    {
        if (textureRef.empty()) {
            return "";
        }

        // 如果不是引用，原样返回
        if (textureRef[0] != '#') {
            return textureRef;
        }

        // 解析引用
        std::string varName = textureRef.substr(1);
        auto it = textures.find(varName);
        if (it != textures.end()) {
            // 如果结果也是引用，递归解析
            if (!it->second.empty() && it->second[0] == '#') {
                return resolveTexture(it->second);
            }
            return it->second;
        }

        return textureRef;
    }

    /**
     * @brief 检查模型是否有几何体
     */
    bool hasElements() const noexcept { return !elements.empty(); }

    /**
     * @brief 检查模型是否有父模型
     */
    bool hasParent() const noexcept { return !parent.empty(); }
};

} // namespace unified
} // namespace compat
} // namespace resource
} // namespace mc
