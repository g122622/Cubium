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
 * IMPLIED, INCLUDING BUT BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "ArgumentType.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/CommandSource.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/command/coordinates/LocalCoordinates.hpp"
#include "common/command/coordinates/WorldCoordinate.hpp"
#include "common/command/coordinates/WorldCoordinates.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

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
        std::string lower = _toLower(name);

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
    static std::string _toLower(const std::string& str)
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

// ========== 坐标参数异常 ==========

/**
 * @brief 创建混合坐标类型异常
 *
 * 当在同一坐标中混合使用 ^ 和 ~ 或绝对坐标时抛出。
 * MC Java 不允许混合局部坐标与世界坐标。
 */
inline CommandException createMixedCoordinateTypeError()
{
    return CommandException(
        CommandErrorType::Unknown, "Mixed coordinate types: cannot mix ^ with ~ or absolute coordinates");
}

/**
 * @brief 方块位置参数类型
 *
 * 支持三种坐标格式：
 * - 绝对坐标：100 64 -200
 * - 相对坐标：~ ~ ~（~表示当前位置），~5 ~-2 ~10
 * - 局部坐标：^ ^ ^（相对于视线方向），^1 ^2 ^3
 *
 * 返回 Coordinates::Ptr，调用方需要在执行时调用 getPosition(source) 获取最终坐标。
 * 使用 getBlockPos(source) 获取取整后的方块坐标。
 */
class BlockPosArgumentType : public ArgumentType<Coordinates::Ptr> {
public:
    [[nodiscard]] Coordinates::Ptr parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();

        // 检查第一个字符判断坐标类型
        if (reader.canRead() && reader.peek() == LocalCoordinates::PREFIX) {
            // 局部坐标 ^ ^ ^
            return _parseLocalCoordinates(reader, start);
        } else {
            // 绝对/相对坐标
            return _parseWorldCoordinatesInt(reader, start);
        }
    }

    [[nodiscard]] std::string getTypeName() const override { return "block_pos"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0 0 0", "~ ~ ~", "^ ^ ^", "~1 ~-2 ~5"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<BlockPosArgumentType> blockPos() { return std::make_shared<BlockPosArgumentType>(); }

    // ========== 静态获取方法 ==========

    /**
     * @brief 获取方块坐标（取整后的 Vector3i）
     */
    template <typename S>
    static Vector3i getBlockPos(CommandContext<S>& context, const std::string& name, const S& source)
    {
        auto coords = context.template getArgument<Coordinates::Ptr>(name);
        Vector3d anchorPos = _getAnchorPosition(source);
        return coords->getBlockPos(anchorPos, source.rotation());
    }

    /**
     * @brief 获取世界坐标（Vector3d）
     */
    template <typename S>
    static Vector3d getPosition(CommandContext<S>& context, const std::string& name, const S& source)
    {
        auto coords = context.template getArgument<Coordinates::Ptr>(name);
        Vector3d anchorPos = _getAnchorPosition(source);
        return coords->getPosition(anchorPos, source.rotation());
    }

private:
    /**
     * @brief 解析局部坐标 ^left ^up ^forwards
     */
    [[nodiscard]] Coordinates::Ptr _parseLocalCoordinates(StringReader& reader, i32 start)
    {
        f64 left = _parseLocalDouble(reader, start);
        reader.skipWhitespace();
        f64 up = _parseLocalDouble(reader, start);
        reader.skipWhitespace();
        f64 forwards = _parseLocalDouble(reader, start);
        return std::make_shared<LocalCoordinates>(left, up, forwards);
    }

    /**
     * @brief 解析局部坐标的单个分量，必须以 ^ 开头
     */
    f64 _parseLocalDouble(StringReader& reader, i32 start)
    {
        if (!reader.canRead()) {
            throw CommandException(CommandErrorType::Unknown, "Expected local coordinate", reader.getCursor());
        }
        if (reader.peek() != LocalCoordinates::PREFIX) {
            reader.setCursor(start);
            throw createMixedCoordinateTypeError();
        }
        reader.skip();
        // ^ 后无数字时默认为 0.0
        if (!reader.canRead() || _isWhitespace(reader.peek())) {
            return 0.0;
        }
        return reader.readDouble();
    }

    /**
     * @brief 解析绝对/相对坐标（整数版本，用于方块坐标）
     */
    [[nodiscard]] Coordinates::Ptr _parseWorldCoordinatesInt(StringReader& reader, i32 start)
    {
        WorldCoordinate x = _parseCoordinateInt(reader, start);
        reader.skipWhitespace();
        WorldCoordinate y = _parseCoordinateInt(reader, start);
        reader.skipWhitespace();
        WorldCoordinate z = _parseCoordinateInt(reader, start);
        return std::make_shared<WorldCoordinates>(x, y, z);
    }

    /**
     * @brief 解析单个整数坐标分量
     */
    WorldCoordinate _parseCoordinateInt(StringReader& reader, i32 start)
    {
        bool relative = false;
        if (reader.canRead() && reader.peek() == '~') {
            relative = true;
            reader.skip();
        } else if (reader.canRead() && reader.peek() == LocalCoordinates::PREFIX) {
            // 在世界坐标中遇到 ^ 前缀，抛出混合类型错误
            reader.setCursor(start);
            throw createMixedCoordinateTypeError();
        }

        // 如果后面没有数字，返回 0（~ 或单独出现时）
        if (!reader.canRead() || _isWhitespace(reader.peek())) {
            return WorldCoordinate(relative, 0.0);
        }

        f64 value = reader.readDouble();
        return WorldCoordinate(relative, value);
    }

    static bool _isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
};

/**
 * @brief 向量位置参数类型
 *
 * 与 BlockPosArgumentType 类似，但返回浮点坐标。
 * 支持 centerCorrect 选项：当为 true 时，绝对整数坐标会加 0.5 偏移到方块中心。
 *
 * centerCorrect 行为（与 MC Java 一致）：
 * - centerCorrect=true 且绝对整数坐标 → 加 0.5（如 10 → 10.5）
 * - centerCorrect=true 且绝对小数坐标 → 不加偏移（如 10.0 → 10.0）
 * - 相对坐标 → 不受 centerCorrect 影响
 * - 局部坐标 → 不受 centerCorrect 影响
 */
class Vec3ArgumentType : public ArgumentType<Coordinates::Ptr> {
public:
    explicit Vec3ArgumentType(bool centerCorrect = true)
        : m_centerCorrect(centerCorrect)
    {}

    [[nodiscard]] Coordinates::Ptr parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();

        // 检查第一个字符判断坐标类型
        if (reader.canRead() && reader.peek() == LocalCoordinates::PREFIX) {
            // 局部坐标 ^ ^ ^
            return _parseLocalCoordinates(reader, start);
        } else {
            // 绝对/相对坐标
            return _parseWorldCoordinatesDouble(reader, start);
        }
    }

    [[nodiscard]] std::string getTypeName() const override { return "vec3"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0 0 0", "~ ~ ~", "^ ^ ^", "~1.5 ~-0.5 ~5"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<Vec3ArgumentType> vec3() { return std::make_shared<Vec3ArgumentType>(true); }

    /**
     * @brief 创建不带中心偏移的 Vec3 参数（用于 /execute positioned 等场景）
     */
    static std::shared_ptr<Vec3ArgumentType> vec3NoCenter() { return std::make_shared<Vec3ArgumentType>(false); }

    // ========== 静态获取方法 ==========

    /**
     * @brief 获取世界坐标（Vector3d）
     */
    template <typename S>
    static Vector3d getVec3(CommandContext<S>& context, const std::string& name, const S& source)
    {
        auto coords = context.template getArgument<Coordinates::Ptr>(name);
        Vector3d anchorPos = _getAnchorPosition(source);
        return coords->getPosition(anchorPos, source.rotation());
    }

    /**
     * @brief 获取坐标对象（用于需要判断坐标类型的场景）
     */
    template <typename S>
    static Coordinates::Ptr getCoordinates(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<Coordinates::Ptr>(name);
    }

private:
    bool m_centerCorrect;

    /**
     * @brief 解析局部坐标 ^left ^up ^forwards
     */
    [[nodiscard]] Coordinates::Ptr _parseLocalCoordinates(StringReader& reader, i32 start)
    {
        f64 left = _parseLocalDouble(reader, start);
        reader.skipWhitespace();
        f64 up = _parseLocalDouble(reader, start);
        reader.skipWhitespace();
        f64 forwards = _parseLocalDouble(reader, start);
        return std::make_shared<LocalCoordinates>(left, up, forwards);
    }

    /**
     * @brief 解析局部坐标的单个分量，必须以 ^ 开头
     */
    f64 _parseLocalDouble(StringReader& reader, i32 start)
    {
        if (!reader.canRead()) {
            throw CommandException(CommandErrorType::Unknown, "Expected local coordinate", reader.getCursor());
        }
        if (reader.peek() != LocalCoordinates::PREFIX) {
            reader.setCursor(start);
            throw createMixedCoordinateTypeError();
        }
        reader.skip();
        // ^ 后无数字时默认为 0.0
        if (!reader.canRead() || _isWhitespace(reader.peek())) {
            return 0.0;
        }
        return reader.readDouble();
    }

    /**
     * @brief 解析绝对/相对坐标（浮点版本）
     */
    [[nodiscard]] Coordinates::Ptr _parseWorldCoordinatesDouble(StringReader& reader, i32 start)
    {
        WorldCoordinate x = _parseCoordinateDouble(reader, start);
        reader.skipWhitespace();
        WorldCoordinate y = _parseCoordinateDouble(reader, start);
        reader.skipWhitespace();
        WorldCoordinate z = _parseCoordinateDouble(reader, start);
        return std::make_shared<WorldCoordinates>(x, y, z);
    }

    /**
     * @brief 解析单个浮点坐标分量
     *
     * 支持 ~ 前缀（相对坐标）和 centerCorrect 逻辑。
     * 遇到 ^ 前缀会抛出混合类型错误。
     */
    WorldCoordinate _parseCoordinateDouble(StringReader& reader, i32 start)
    {
        bool relative = false;
        i32 cursorBeforeNumber = reader.getCursor();

        if (reader.canRead() && reader.peek() == '~') {
            relative = true;
            reader.skip();
        } else if (reader.canRead() && reader.peek() == LocalCoordinates::PREFIX) {
            // 在世界坐标中遇到 ^ 前缀，抛出混合类型错误
            reader.setCursor(start);
            throw createMixedCoordinateTypeError();
        }

        // 如果后面没有数字，返回 0（~ 或单独出现时）
        if (!reader.canRead() || _isWhitespace(reader.peek())) {
            return WorldCoordinate(relative, 0.0);
        }

        i32 cursorBeforeValue = reader.getCursor();
        f64 value = reader.readDouble();

        // centerCorrect: 绝对整数坐标加 0.5（方块中心偏移）
        // 条件：centerCorrect=true 且 非相对坐标 且 原始输入不含小数点
        if (m_centerCorrect && !relative) {
            // 检查原始输入是否不含小数点
            std::string_view input = reader.getString();
            bool hasDot = false;
            for (i32 i = cursorBeforeNumber; i < reader.getCursor(); ++i) {
                if (input[static_cast<size_t>(i)] == '.') {
                    hasDot = true;
                    break;
                }
            }
            if (!hasDot) {
                value += 0.5;
            }
        }

        return WorldCoordinate(relative, value);
    }

    static bool _isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
};

/**
 * @brief 二维向量位置参数类型（水平面 x, z 坐标）
 *
 * 解析两个以空格分隔的双精度浮点坐标分量（x 和 z），
 * 支持 ~ 相对坐标前缀，不支持 ^ 局部坐标。
 *
 * 与 Vec3ArgumentType 的区别：
 * - Vec3ArgumentType 解析三个分量 (x, y, z)，适用于三维坐标
 * - Vec2ArgumentType 只解析两个分量 (x, z)，适用于水平面坐标
 *
 * 用于 /spreadplayers 等只需要水平面坐标的命令。
 * MC 原版对应: Vec2Argument（返回 Coordinates 接口，内部构造 WorldCoordinates(x, relative(0), z)）
 */
class Vec2ArgumentType : public ArgumentType<Coordinates::Ptr> {
public:
    [[nodiscard]] Coordinates::Ptr parse(StringReader& reader) override
    {
        WorldCoordinate x = _parseCoordinate(reader);
        reader.skipWhitespace();
        WorldCoordinate z = _parseCoordinate(reader);
        // y 分量填充为相对 0（与 MC Java 一致）
        return std::make_shared<WorldCoordinates>(x, WorldCoordinate(true, 0.0), z);
    }

    [[nodiscard]] std::string getTypeName() const override { return "vec2"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override { return {"0 0", "~ ~", "0.1 -0.5", "~1 ~-2"}; }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<Vec2ArgumentType> vec2() { return std::make_shared<Vec2ArgumentType>(); }

    // ========== 静态获取方法 ==========

    /**
     * @brief 获取水平面坐标 (x, z)
     */
    template <typename S>
    static Vector2d getVec2(CommandContext<S>& context, const std::string& name, const S& source)
    {
        auto coords = context.template getArgument<Coordinates::Ptr>(name);
        Vector3d anchorPos = _getAnchorPosition(source);
        Vector3d pos = coords->getPosition(anchorPos, source.rotation());
        return Vector2d(pos.x, pos.z);
    }

private:
    WorldCoordinate _parseCoordinate(StringReader& reader)
    {
        bool relative = false;
        if (reader.canRead() && reader.peek() == '~') {
            relative = true;
            reader.skip();
        }
        // Vec2Argument 不支持 ^ 局部坐标（与 Vec3Argument 不同）
        // 遇到 ^ 抛出混合类型错误
        // 注意：MC Java 的 Vec2Argument 内部使用 WorldCoordinate.parseDouble，
        // 它会对 ^ 抛出 ERROR_MIXED_TYPE

        if (!reader.canRead() || _isWhitespace(reader.peek())) {
            return WorldCoordinate(relative, 0.0);
        }

        return WorldCoordinate(relative, reader.readDouble());
    }

    static bool _isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
};

/**
 * @brief 方块旋转参数类型（yaw, pitch）
 *
 * 支持 ~ 前缀表示相对角度偏移。
 * 返回 WorldCoordinates，其中 x 分量为 pitch，y 分量为 yaw。
 *
 * MC 原版对应: RotationArgument（返回 WorldCoordinates(pitch, yaw, relative(0))）
 */
class RotationArgumentType : public ArgumentType<Coordinates::Ptr> {
public:
    [[nodiscard]] Coordinates::Ptr parse(StringReader& reader) override
    {
        // 第一个参数是 yaw（对应 y 分量），第二个参数是 pitch（对应 x 分量）
        // MC Java 的 RotationArgument 构造 WorldCoordinates(yawCoord, pitchCoord, relative(0))
        // 即 WorldCoordinates.x = yaw, WorldCoordinates.y = pitch
        WorldCoordinate yawCoord = _parseAngle(reader);
        reader.skipWhitespace();
        WorldCoordinate pitchCoord = _parseAngle(reader);
        // z 分量填充为相对 0
        return std::make_shared<WorldCoordinates>(yawCoord, pitchCoord, WorldCoordinate(true, 0.0));
    }

    [[nodiscard]] std::string getTypeName() const override { return "rotation"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override { return {"0 0", "~ ~", "90 -45"}; }

    static std::shared_ptr<RotationArgumentType> rotation() { return std::make_shared<RotationArgumentType>(); }

    /**
     * @brief 获取旋转角 (yaw, pitch)
     */
    template <typename S>
    static Vector2f getRotation(CommandContext<S>& context, const std::string& name, const S& source)
    {
        auto coords = context.template getArgument<Coordinates::Ptr>(name);
        return coords->getRotation(source.rotation());
    }

private:
    WorldCoordinate _parseAngle(StringReader& reader)
    {
        bool relative = false;
        if (reader.canRead() && reader.peek() == '~') {
            relative = true;
            reader.skip();
        }

        if (!reader.canRead() || _isWhitespace(reader.peek())) {
            return WorldCoordinate(relative, 0.0);
        }

        return WorldCoordinate(relative, reader.readDouble());
    }

    static bool _isWhitespace(char c) { return c == ' ' || c == '\t'; }
};

/**
 * @brief 根据命令源的锚点类型计算锚点位置
 *
 * Feet 锚点使用命令源位置，Eyes 锚点使用位置 + 实体眼睛高度。
 * 如果没有关联实体，退回到使用命令源位置。
 */
template <typename S>
Vector3d _getAnchorPosition(const S& source)
{
    if (source.anchor() == EntityAnchorType::Eyes && source.entity() != nullptr) {
        const Vector3d& pos = source.position();
        return Vector3d(pos.x, pos.y + static_cast<f64>(source.entity()->eyeHeight()), pos.z);
    }
    return source.position();
}

} // namespace command
} // namespace mc
