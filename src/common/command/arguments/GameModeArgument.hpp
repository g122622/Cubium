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

#include "ArgumentType.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/StringReader.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <algorithm>
#include <functional>
#include <memory>

namespace mc {

namespace command {

/**
 * @brief 游戏模式参数类型
 *
 * 解析游戏模式名称：
 * - survival, s, 0 -> Survival
 * - creative, c, 1 -> Creative
 * - adventure, a, 2 -> Adventure
 * - spectator, sp, 3 -> Spectator
 *
 * 注意：GameMode 枚举定义在 common/core/Types.hpp 中
 */
class GameModeArgumentType : public ArgumentType<GameMode> {
public:
    [[nodiscard]] GameMode parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();
        std::string name = reader.readUnquotedString();

        // 转换为小写进行比较
        std::string lower = toLower(name);

        if (lower == "survival" || lower == "s" || lower == "0") {
            return GameMode::Survival;
        } else if (lower == "creative" || lower == "c" || lower == "1") {
            return GameMode::Creative;
        } else if (lower == "adventure" || lower == "a" || lower == "2") {
            return GameMode::Adventure;
        } else if (lower == "spectator" || lower == "sp" || lower == "3") {
            return GameMode::Spectator;
        }

        reader.setCursor(start);
        throw CommandException(CommandErrorType::Unknown, "Invalid game mode: " + name, start);
    }

    [[nodiscard]] std::string getTypeName() const override { return "gamemode"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"survival", "creative", "adventure", "spectator"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<GameModeArgumentType> gameMode() { return std::make_shared<GameModeArgumentType>(); }

    // ========== 静态获取方法 ==========

    template <typename S>
    static GameMode getGameMode(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<GameMode>(name);
    }

    static std::string toString(GameMode mode)
    {
        switch (mode) {
            case GameMode::Survival:
                return "survival";
            case GameMode::Creative:
                return "creative";
            case GameMode::Adventure:
                return "adventure";
            case GameMode::Spectator:
                return "spectator";
            case GameMode::NotSet:
                return "not_set";
        }
        return "unknown";
    }

private:
    static std::string toLower(const std::string& str)
    {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }
        return result;
    }
};

/**
 * @brief 资源位置参数类型
 *
 * 解析资源位置，格式：namespace:path 或 path（默认命名空间）
 */
class ResourceLocationArgumentType : public ArgumentType<ResourceLocation> {
public:
    [[nodiscard]] ResourceLocation parse(StringReader& reader) override
    {
        std::string str = reader.readString();

        // 解析命名空间
        size_t colonPos = str.find(':');
        if (colonPos != std::string::npos) {
            std::string namespace_ = str.substr(0, colonPos);
            std::string path = str.substr(colonPos + 1);
            return ResourceLocation(namespace_, path);
        } else {
            // 默认命名空间
            return ResourceLocation("minecraft", str);
        }
    }

    [[nodiscard]] std::string getTypeName() const override { return "resource_location"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:stone", "stone", "minecraft:diamond_sword"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<ResourceLocationArgumentType> resourceLocation()
    {
        return std::make_shared<ResourceLocationArgumentType>();
    }

    // ========== 静态获取方法 ==========

    template <typename S>
    static ResourceLocation getResourceLocation(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<ResourceLocation>(name);
    }
};

/**
 * @brief 方块位置参数类型
 *
 * 支持三种坐标格式：
 * - 绝对坐标：100 64 -200
 * - 相对坐标：~ ~ ~（~表示当前位置）
 * - 局部坐标：^ ^ ^（^表示相对于视线方向）
 *
 * 参考 MC 的 BlockPosArgument
 */
class BlockPosArgumentType : public ArgumentType<Vector3i> {
public:
    [[nodiscard]] Vector3i parse(StringReader& reader) override
    {
        i32 x = parseCoordinate(reader, 'x');
        reader.skipWhitespace();
        i32 y = parseCoordinate(reader, 'y');
        reader.skipWhitespace();
        i32 z = parseCoordinate(reader, 'z');
        return Vector3i(x, y, z);
    }

    [[nodiscard]] std::string getTypeName() const override { return "block_pos"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0 0 0", "~ ~ ~", "^ ^ ^", "~1 ~-2 ~5"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<BlockPosArgumentType> blockPos() { return std::make_shared<BlockPosArgumentType>(); }

    template <typename S>
    static Vector3i getBlockPos(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<Vector3i>(name);
    }

private:
    /**
     * @brief 解析单个坐标分量
     */
    i32 parseCoordinate(StringReader& reader, [[maybe_unused]] char axis)
    {
        if (reader.canRead() && reader.peek() == '~') {
            reader.skip();
        } else if (reader.canRead() && reader.peek() == '^') {
            reader.skip();
        }

        // 如果后面没有数字，返回0（~ 或 ^ 单独出现）
        if (!reader.canRead() || isWhitespace(reader.peek())) {
            return 0;
        }

        // 解析数字部分
        i32 value = reader.readInt();
        return value;
    }

    static bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
};

/**
 * @brief 向量位置参数类型
 *
 * 与 BlockPosArgumentType 类似，但返回浮点坐标
 */
class Vec3ArgumentType : public ArgumentType<Vector3d> {
public:
    [[nodiscard]] Vector3d parse(StringReader& reader) override
    {
        f64 x = parseCoordinate(reader);
        reader.skipWhitespace();
        f64 y = parseCoordinate(reader);
        reader.skipWhitespace();
        f64 z = parseCoordinate(reader);
        return Vector3d(x, y, z);
    }

    [[nodiscard]] std::string getTypeName() const override { return "vec3"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0 0 0", "~ ~ ~", "^ ^ ^", "~1.5 ~-0.5 ~5"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<Vec3ArgumentType> vec3() { return std::make_shared<Vec3ArgumentType>(); }

    template <typename S>
    static Vector3d getVec3(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<Vector3d>(name);
    }

private:
    f64 parseCoordinate(StringReader& reader)
    {
        if (reader.canRead() && reader.peek() == '~') {
            reader.skip();
        } else if (reader.canRead() && reader.peek() == '^') {
            reader.skip(); // 局部坐标，暂时忽略
        }

        if (!reader.canRead() || isWhitespace(reader.peek())) {
            return 0.0;
        }

        return reader.readDouble();
    }

    static bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
};

/**
 * @brief 方块旋转参数类型（yaw, pitch）
 */
class RotationArgumentType : public ArgumentType<Vector2f> {
public:
    [[nodiscard]] Vector2f parse(StringReader& reader) override
    {
        f32 yaw = static_cast<f32>(parseAngle(reader));
        reader.skipWhitespace();
        f32 pitch = static_cast<f32>(parseAngle(reader));
        return Vector2f(yaw, pitch);
    }

    [[nodiscard]] std::string getTypeName() const override { return "rotation"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override { return {"0 0", "~ ~", "90 -45"}; }

    static std::shared_ptr<RotationArgumentType> rotation() { return std::make_shared<RotationArgumentType>(); }

    template <typename S>
    static Vector2f getRotation(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<Vector2f>(name);
    }

private:
    f64 parseAngle(StringReader& reader)
    {
        if (reader.canRead() && reader.peek() == '~') {
            reader.skip();
        }

        if (!reader.canRead() || isWhitespace(reader.peek())) {
            return 0.0;
        }

        return reader.readDouble();
    }

    static bool isWhitespace(char c) { return c == ' ' || c == '\t'; }
};

} // namespace command
} // namespace mc
