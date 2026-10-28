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
// _getAnchorPosition<S> 在模板体内调用 source.entity()->eyeHeight()，需要 Entity 完整定义。
// 此前靠调用点的传递性 include 偶然成立，一旦某个调用点的包含链变化即编译失败，故显式引入。
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"
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
    /**
     * @brief 解析游戏模式名（原版 GameModeArgument 语义）
     *
     * 只认 `survival` / `creative` / `adventure` / `spectator` 四个全名——1.13 起
     * `s`/`c`/`a`/`sp` 与数字简写都已从原版移除，`/gamemode s` 在原版里是语法错误。
     * 大小写也不同：原版按名字精确匹配，不转小写。不可识别时游标停在已读到的位置
     * （读完了整个单词），与 brigadier 的报错位置一致。
     */
    [[nodiscard]] GameMode parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();
        std::string name = reader.readUnquotedString();

        if (name == "survival") {
            return GameMode::Survival;
        }
        if (name == "creative") {
            return GameMode::Creative;
        }
        if (name == "adventure") {
            return GameMode::Adventure;
        }
        if (name == "spectator") {
            return GameMode::Spectator;
        }

        // 与 brigadier 一致：报错位置是「读完整个单词」处，游标本身停留在起点。
        const i32 end = start + static_cast<i32>(name.size());
        reader.setCursor(start);
        throw CommandException(CommandErrorType::Unknown, "Unknown game mode: " + name, end);
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "gamemode";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"survival", "creative", "adventure", "spectator"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<GameModeArgumentType> gameMode()
    {
        return std::make_shared<GameModeArgumentType>();
    }

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
/**
 * @brief 按原版坐标语法读取一个数字（{@code WorldCoordinate.parseDouble(reader, false)}）
 *
 * 原版实现（用原版 jar 反编译 + 实测确认）：
 *
 * <pre>
 *   末尾已读尽          → SyntaxException「Expected a coordinate」，游标停在起点
 *   下一个字符是空格    → 0.0，游标不动（` 5s` 因此是 0，且单位读成空串）
 *   其它                → readDouble()：可选 '-'、数字、至多一个小数点，遇字母/`+` 即停
 * </pre>
 *
 * 所以 `1e3` 读成 1（`e3` 留在原地当单位），`0x10` 读成 0（`x10` 留在原地）。
 * 所有数值参数（坐标、时间）都走这一个读取器。
 *
 * @param reader 读取器
 * @param out 读到的值
 * @return 是否读到；游标已读尽时返回 false（调用方此时应整体失败）
 */
inline bool readWorldCoordinateDouble(StringReader& reader, f64& out)
{
    if (reader.atEnd()) {
        return false;
    }
    if (reader.peek() == ' ') {
        out = 0.0;
        return true;
    }
    out = reader.readDouble();
    return true;
}

/**
 * @brief 按命令语法读取资源位置（原版 {@code Identifier.read(StringReader)}）
 *
 * 贪婪读取 `[a-z0-9_.-]`、`/` 与单个 `:`,遇到第二个 `:` 报错、遇到其它字符立刻停。
 * 命令里所有「方块 / 物品 / 实体 / 维度 / 结构」的 id 都走这一个语法——
 * 之前各处用 `readString()`（会吃掉引号与大写），与原版在 `"stone"`、`Minecraft:Stone`
 * 这类输入上分叉。
 *
 * @param reader 读取器
 * @return 解析出的 id（无命名空间时默认 minecraft）
 * @throws CommandException 遇到第二个 `:` 时
 */
inline ResourceLocation readResourceLocationId(StringReader& reader)
{
    const i32 start = reader.getCursor();
    std::string text;
    while (reader.canRead()) {
        const char c = reader.peek();
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == '/') {
            text.push_back(c);
        } else if (c == ':' && text.find(':') == std::string::npos) {
            text.push_back(c);
        } else if (c == ':') {
            // 第二个冒号：原版直接报错（SyntaxException in command argument parser）。
            reader.setCursor(start);
            throw CommandException(CommandErrorType::Unknown, "Invalid resource location: " + text, start);
        } else {
            break;
        }
        reader.skip();
    }

    const size_t colonPos = text.find(':');
    if (colonPos != std::string::npos) {
        return ResourceLocation(text.substr(0, colonPos), text.substr(colonPos + 1));
    }
    // 空 id 是允许的（原版读出 `minecraft:`），调用方随后会在注册表里查不到而报错。
    return ResourceLocation("minecraft", text);
}

class ResourceLocationArgumentType : public ArgumentType<ResourceLocation> {
public:
    ResourceLocationArgumentType() = default;

    /// 解析资源位置（见 {@link readResourceLocationId}）。
    [[nodiscard]] ResourceLocation parse(StringReader& reader) override
    {
        return readResourceLocationId(reader);
    }

    /// 原版命令树里的注册名是 `resource_location`（只要求语法合法的 id）。
    /// 绑定到注册表的单值资源用 {@link ResourceArgumentType}（注册名 `resource`）。
    [[nodiscard]] std::string getTypeName() const override
    {
        return "resource_location";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:stone", "stone", "minecraft:diamond_sword"};
    }

    /// 原版 ResourceLocationArgument 没有内置建议：注册命令时由各命令自己决定挂不挂
    /// {@code SharedSuggestionProvider.suggestResource(...)}（/bossbar、/playsound 挂了，
    /// /data、/execute 下的没挂），所以这里恒为 false；挂了提供器的节点走
    /// {@code CommandNode::hasCustomSuggestions()}。
    [[nodiscard]] bool hasBuiltinSuggestions() const noexcept override
    {
        return false;
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
 * @brief 粒子类型参数（原版 {@code ResourceArgument.particle()}，命令树里叫 {@code resource}）。
 */
class ParticleResourceArgumentType : public ArgumentType<ResourceLocation> {
public:
    [[nodiscard]] ResourceLocation parse(StringReader& reader) override
    {
        return readResourceLocationId(reader);
    }

    // 原版注册名就是 particle（ParticleArgument 的注册表项），不是 resource。
    [[nodiscard]] std::string getTypeName() const override
    {
        return "particle";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:flame", "flame", "minecraft:heart"};
    }

    static std::shared_ptr<ParticleResourceArgumentType> particle()
    {
        return std::make_shared<ParticleResourceArgumentType>();
    }
};

/**
 * @brief 聊天组件参数（原版 {@code ComponentArgument}，命令树里叫 {@code component}）
 *
 * 输入一段 JSON（支持 SNBT 的组件写法由调用方另行处理）。这里只读整段剩余文本：
 * 组件是最后一个参数，原版子解析器同样会读掉后续全部内容。
 */
struct ComponentText {
    std::string text;
};

/// 参数节点持有的组件值：原版 ArgumentCommandNode 要求可默认构造的值类型，用指针最省事。
using ComponentTextPtr = std::shared_ptr<ComponentText>;

class ComponentArgumentType : public ArgumentType<ComponentTextPtr> {
public:
    [[nodiscard]] ComponentTextPtr parse(StringReader& reader) override
    {
        // 组件是命令链末端：原版 MessageArgument 用「剩余原文」构建组件，读走整段；
        // 但开头是引号时仍按引号串读取（`kick @a "some reason"` 的值不含引号）。
        std::string text = reader.canRead() && reader.peek() == '"' ? reader.readQuotedString() : reader.getRemaining();
        reader.setCursor(reader.getTotalLength());
        return std::make_shared<ComponentText>(ComponentText{std::move(text)});
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "component";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"{\"text\":\"hello\"}", "\"hello\"", "[{\"text\":\"a\"}]"};
    }

    static std::shared_ptr<ComponentArgumentType> component()
    {
        return std::make_shared<ComponentArgumentType>();
    }
};

/**
 * @brief 玩家档案参数（原版 {@code GameProfileArgument}，命令树里叫 {@code game_profile}）
 *
 * 原版 GameProfileArgument.getGameProfiles 先走 EntitySelectorParser 的 SELECTOR 解析器，
 * 因此 `@s` / `@a` 这类实体选择器同样合法（/team join <team> @s 即依赖这一点），名字列表
 * 只是它的另一条分支。这里把原始文本原样交回调用方，由 PlayerResolver::gameProfileSelector
 * 区分为「实体选择器」还是「名字」。
 */
class GameProfileArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string result;
        // 实体选择器以 '@' 开头时可能含方括号与空格（`@a[distance=..5]`），须整体读取。
        if (reader.canRead() && reader.peek() == '@') {
            while (reader.canRead() && !reader.isWhitespace(reader.peek())) {
                result.push_back(reader.peek());
                reader.skip();
            }
            return result;
        }
        while (reader.canRead()) {
            const char c = reader.peek();
            if (reader.isWhitespace(c)) {
                break;
            }
            if (c == ',') {
                // 原版允许多个玩家名逗号分隔，中间允许空格。
                result.push_back(c);
                reader.skip();
                reader.skipWhitespace();
                continue;
            }
            result.push_back(c);
            reader.skip();
        }
        if (result.empty()) {
            throw CommandException(
                CommandErrorType::Unknown, "Expected player name at position " + std::to_string(start), start);
        }
        return result;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "game_profile";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"Steve", "Alex,Steve"};
    }

    static std::shared_ptr<GameProfileArgumentType> gameProfile()
    {
        return std::make_shared<GameProfileArgumentType>();
    }
};

/**
 * @brief 聊天消息参数（原版 {@code MessageArgument}，命令树里叫 {@code message}）
 *
 * 与 {@link ComponentArgumentType} 读法相同（整段剩余文本），区别只在类型名：/say、/msg、
 * /teammsg 在协议里是 `message`，/tellraw、/title、/kick 的 reason 是 `component`。
 */
class MessageComponentArgumentType : public ArgumentType<ComponentTextPtr> {
public:
    [[nodiscard]] ComponentTextPtr parse(StringReader& reader) override
    {
        std::string text = reader.canRead() && reader.peek() == '"' ? reader.readQuotedString() : reader.getRemaining();
        reader.setCursor(reader.getTotalLength());
        return std::make_shared<ComponentText>(ComponentText{std::move(text)});
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "message";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"hello", "{\"text\":\"hi\"}"};
    }

    static std::shared_ptr<MessageComponentArgumentType> message()
    {
        return std::make_shared<MessageComponentArgumentType>();
    }
};

/**
 * @brief 队伍名参数（原版 {@code TeamArgument}，命令树里叫 {@code team}）
 */
class TeamNameArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string result;
        while (reader.canRead()) {
            const char c = reader.peek();
            // 队伍名里不能有空白；其余字符（含中文、括号等）原版都直接收下。
            if (reader.isWhitespace(c)) {
                break;
            }
            result.push_back(c);
            reader.skip();
        }
        if (result.empty()) {
            throw CommandException(
                CommandErrorType::Unknown, "Expected team name at position " + std::to_string(start), start);
        }
        return result;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "team";
    }

    // 原版 TeamArgument 没有自定义 ArgumentTypeInfo，注册在 ArgumentTypeInfos 里用的是
    // SingletonArgumentInfo.contextFree，网络 payload 为空（不写任何字节）。
    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"red", "team-1"};
    }

    static std::shared_ptr<TeamNameArgumentType> team()
    {
        return std::make_shared<TeamNameArgumentType>();
    }
};

/**
 * @brief 记分持有者名参数（原版 {@code ScoreHolderArgument}，命令树里叫 {@code score_holder}）
 *
 * 与 {@link GameProfileArgumentType} 同样收「名字」而不是选择器，读法一致。
 */
class ScoreHolderArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string result;
        while (reader.canRead()) {
            const char c = reader.peek();
            if (reader.isWhitespace(c)) {
                break;
            }
            if (c == ',') {
                result.push_back(c);
                reader.skip();
                reader.skipWhitespace();
                continue;
            }
            result.push_back(c);
            reader.skip();
        }
        if (result.empty()) {
            throw CommandException(
                CommandErrorType::Unknown, "Expected score holder at position " + std::to_string(start), start);
        }
        return result;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "score_holder";
    }

    /// 网络序列化：原版 ScoreHolderArgument.Info 写 byte flags(bit0=multiple)。
    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"multiple", m_multiple}};
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"Steve", "*"};
    }

    /// 原版 ScoreHolderArgument 没有内置建议。单目标（/execute if score、/scoreboard players get）
    /// 命令行里没挂提供器；多目标（/scoreboard players set、/team join 等）在注册时由命令自己挂
    /// {@code suggestScoreHolders()}——那属于 {@code CommandNode::hasCustomSuggestions()}。
    [[nodiscard]] bool hasBuiltinSuggestions() const noexcept override
    {
        return m_multiple;
    }

    static std::shared_ptr<ScoreHolderArgumentType> scoreHolder(bool multiple = false)
    {
        auto result = std::make_shared<ScoreHolderArgumentType>();
        result->m_multiple = multiple;
        return result;
    }

private:
    /// 原版 ScoreHolderArgument.Info 的 multiple 标志（bit0）：true 表示可一次给多个持有者。
    /// 它参与网络序列化，命令树里 `/scoreboard players *` 一类节点据此区分单/多目标。
    bool m_multiple = false;
};

/**
 * @brief 记分项名参数（原版 {@code ObjectiveArgument}，命令树里叫 {@code objective}）
 */
class ObjectiveNameArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string result;
        while (reader.canRead() && !reader.isWhitespace(reader.peek())) {
            result.push_back(reader.peek());
            reader.skip();
        }
        if (result.empty()) {
            throw CommandException(
                CommandErrorType::Unknown, "Expected objective name at position " + std::to_string(start), start);
        }
        return result;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "objective";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"obj", "my_objective"};
    }

    static std::shared_ptr<ObjectiveNameArgumentType> objective()
    {
        return std::make_shared<ObjectiveNameArgumentType>();
    }
};

/**
 * @brief 坐标轴组合参数（原版 {@code SwizzleArgument}，命令树里叫 {@code swizzle}）
 *
 * 只接受不重复的 x/y/z 组合：x、z、y、xz、xy、yz、xyz。
 */
class SwizzleArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string result;
        while (reader.canRead()) {
            const char c = reader.peek();
            if (c != 'x' && c != 'y' && c != 'z') {
                break;
            }
            if (result.find(c) != std::string::npos) {
                reader.setCursor(start);
                throw CommandException(CommandErrorType::Unknown, "Duplicate axis", start);
            }
            result.push_back(c);
            reader.skip();
        }
        if (result.empty() || result.size() > 3) {
            reader.setCursor(start);
            throw CommandException(
                CommandErrorType::Unknown, "Expected swizzle at position " + std::to_string(start), start);
        }
        return result;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "swizzle";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"x", "xy", "xyz"};
    }

    static std::shared_ptr<SwizzleArgumentType> swizzle()
    {
        return std::make_shared<SwizzleArgumentType>();
    }
};

/**
 * @brief 锚点参数（原版 {@code EntityAnchorArgument}，命令树里叫 {@code entity_anchor}）
 *
 * 取值为 eyes / feet，原版按小写精确匹配。
 */
class EntityAnchorArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string result;
        while (reader.canRead()) {
            const char c = reader.peek();
            if (!((c >= 'a' && c <= 'z') || c == '_')) {
                break;
            }
            result.push_back(c);
            reader.skip();
        }
        if (result != "eyes" && result != "feet") {
            reader.setCursor(start);
            throw CommandException(
                CommandErrorType::Unknown, "Expected 'eyes' or 'feet' at position " + std::to_string(start), start);
        }
        return result;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "entity_anchor";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"eyes", "feet"};
    }

    static std::shared_ptr<EntityAnchorArgumentType> entityAnchor()
    {
        return std::make_shared<EntityAnchorArgumentType>();
    }
};

/**
 * @brief 记分板显示槽位参数（原版 setdisplay 的 slot；命令树里该位置是字面量子节点）。
 */
class DisplaySlotArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string current;
        while (reader.canRead() && !reader.isWhitespace(reader.peek())) {
            current.push_back(reader.peek());
            reader.skip();
        }
        if (current.empty() || !isKnownDisplaySlot(current)) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::Unknown, "Unknown display slot '" + current + "'", start);
        }
        return current;
    }

    /// 原版 setdisplay 的槽位参数注册名是 scoreboard_slot。
    [[nodiscard]] std::string getTypeName() const override
    {
        return "scoreboard_slot";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"list", "sidebar", "below_name", "sidebar.team.red"};
    }

    static std::shared_ptr<DisplaySlotArgumentType> displaySlot()
    {
        return std::make_shared<DisplaySlotArgumentType>();
    }

    /// 槽位名 → DisplaySlot；不认识的名字映射到 List，调用方应先用 isKnownDisplaySlot 校验。
    static scoreboard::DisplaySlot toDisplaySlot(const std::string& name)
    {
        if (name == "below_name") {
            return scoreboard::DisplaySlot::BelowName;
        }
        if (name == "sidebar" || name.rfind("sidebar.team.", 0) == 0) {
            return scoreboard::DisplaySlot::Sidebar;
        }
        return scoreboard::DisplaySlot::List;
    }

    static bool isKnownDisplaySlot(const std::string& name)
    {
        return name == "list" || name == "sidebar" || name == "below_name" || name.rfind("sidebar.team.", 0) == 0;
    }
};

/**
 * @brief 动作槽位参数（原版 {@code OperationArgument} 的另一形态，命令树里叫 {@code operation}）
 *
 * 原版 ObjectiveCriteriaArgument 注册的是 objective_criteria，而 scoreboard 的判据在
 * 1.21 里改用 OperationArgument（命令树类型名 operation，不建议）。这里单独表达。
 */
class OperationArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        return readWord(reader);
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "operation";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"dummy"};
    }

    static std::string readWord(StringReader& reader)
    {
        const i32 start = reader.getCursor();
        std::string word = reader.readUnquotedString();
        if (word.empty()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::StringExpected, "Expected word", start);
        }
        return word;
    }
};

/**
 * @brief 记分板判据参数（原版 {@code ObjectiveCriteriaArgument}，命令树里叫 {@code objective_criteria}）
 */
class ObjectiveCriteriaArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string result;
        while (reader.canRead()) {
            const char c = reader.peek();
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == ':' || c == '-')) {
                break;
            }
            result.push_back(c);
            reader.skip();
        }
        if (result.empty()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::Unknown, "Unknown criteria '" + result + "'", start);
        }
        return result;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "objective_criteria";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"dummy", "deathCount", "minecraft:player_kill_count"};
    }

    static std::shared_ptr<ObjectiveCriteriaArgumentType> objectiveCriteria()
    {
        return std::make_shared<ObjectiveCriteriaArgumentType>();
    }
};

/**
 * @brief 记分板运算参数（原版 {@code OperationArgument}：=、+=、-=、*=、/=、%=、<、>、><）
 */
class ScoreOperationArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        static const char* const kOperations[] = {"+=", "-=", "*=", "/=", "%=", "><", "<", ">", "="};
        const i32 start = reader.getCursor();
        for (const char* op : kOperations) {
            const size_t len = std::char_traits<char>::length(op);
            if (reader.canRead(static_cast<i32>(len)) &&
                reader.getString().substr(static_cast<size_t>(static_cast<size_t>(start)), len) == op) {
                reader.skip(static_cast<i32>(len));
                return op;
            }
        }
        throw CommandException(
            CommandErrorType::Unknown, "Expected operation at position " + std::to_string(start), start);
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "operation";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"=", "+=", "><"};
    }

    static std::shared_ptr<ScoreOperationArgumentType> operation()
    {
        return std::make_shared<ScoreOperationArgumentType>();
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

    [[nodiscard]] std::string getTypeName() const override
    {
        return "block_pos";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0 0 0", "~ ~ ~", "^ ^ ^", "~1 ~-2 ~5"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<BlockPosArgumentType> blockPos()
    {
        return std::make_shared<BlockPosArgumentType>();
    }

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

    static bool _isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
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

    [[nodiscard]] std::string getTypeName() const override
    {
        return "vec3";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0 0 0", "~ ~ ~", "^ ^ ^", "~1.5 ~-0.5 ~5"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<Vec3ArgumentType> vec3()
    {
        return std::make_shared<Vec3ArgumentType>(true);
    }

    /**
     * @brief 创建不带中心偏移的 Vec3 参数（用于 /execute positioned 等场景）
     */
    static std::shared_ptr<Vec3ArgumentType> vec3NoCenter()
    {
        return std::make_shared<Vec3ArgumentType>(false);
    }

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

    static bool _isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
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

    [[nodiscard]] std::string getTypeName() const override
    {
        return "vec2";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0 0", "~ ~", "0.1 -0.5", "~1 ~-2"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<Vec2ArgumentType> vec2()
    {
        return std::make_shared<Vec2ArgumentType>();
    }

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

    static bool _isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
};

/**
 * @brief 列坐标参数（原版 {@code ColumnPosArgument}，命令树里叫 {@code column_pos}）
 *
 * 两个方块坐标分量（x/z），y 分量固定为 0；/forceload 用它把坐标取整到区块。
 * 返回与 BlockPos 相同的 Coordinates::Ptr，读取用 getColumnPos。
 */
class ColumnPosArgumentType : public ArgumentType<Coordinates::Ptr> {
public:
    [[nodiscard]] Coordinates::Ptr parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        WorldCoordinate x = _parseCoordinate(reader, start);
        reader.skipWhitespace();
        WorldCoordinate z = _parseCoordinate(reader, start);
        return std::make_shared<WorldCoordinates>(x, WorldCoordinate(true, 0.0), z);
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "column_pos";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0 0", "~ ~", "~1 ~-2"};
    }

    static std::shared_ptr<ColumnPosArgumentType> columnPos()
    {
        return std::make_shared<ColumnPosArgumentType>();
    }

    template <typename S>
    static Vector3i getColumnPos(CommandContext<S>& context, const std::string& name, const S& source)
    {
        auto coords = context.template getArgument<Coordinates::Ptr>(name);
        return coords->getBlockPos(_getAnchorPosition(source), source.rotation());
    }

private:
    static WorldCoordinate _parseCoordinate(StringReader& reader, i32 start)
    {
        bool relative = false;
        if (reader.canRead() && reader.peek() == '~') {
            relative = true;
            reader.skip();
        } else if (reader.canRead() && reader.peek() == LocalCoordinates::PREFIX) {
            reader.setCursor(start);
            throw createMixedCoordinateTypeError();
        }
        if (!reader.canRead() || _isWhitespace(reader.peek())) {
            return WorldCoordinate(relative, 0.0);
        }
        return WorldCoordinate(relative, reader.readDouble());
    }

    static bool _isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
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
        WorldCoordinate yawCoord = _parseAngle(reader);
        reader.skipWhitespace();
        WorldCoordinate pitchCoord = _parseAngle(reader);
        return std::make_shared<WorldCoordinates>(yawCoord, pitchCoord, WorldCoordinate(true, 0.0));
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "rotation";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0 0", "~ ~", "0.1 -0.5"};
    }

    static std::shared_ptr<RotationArgumentType> rotation()
    {
        return std::make_shared<RotationArgumentType>();
    }

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

    static bool _isWhitespace(char c)
    {
        return c == ' ' || c == '\t';
    }
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

// ========== 关键字参数 ==========

/**
 * @brief 单一取值的枚举型参数（高度图、颜色、对话框 id、战利品谓词……）
 *
 * 这些参数在原版里各有一个 `ArgumentTypeInfo`（类型名与网络 id 必须与之一致），但取值
 * 只是一个词或一个资源位置。统一在这里声明类型名与「是否允许 `#` 前缀、是否要求资源
 * 位置语法」，语法细节由调用方按原版词表解释。
 */
class KeywordArgumentType : public ArgumentType<std::string> {
public:
    enum class Syntax {
        Word,         // 未加引号的词（如 heightmap 的 world_surface）
        ResourceId,   // 资源位置（如 dialog 的 id）
        ResourceOrTag // `#tag` 或词（如 loot_predicate）
    };

    KeywordArgumentType(std::string typeName, Syntax syntax)
        : m_typeName(std::move(typeName))
        , m_syntax(syntax)
    {}

    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        switch (m_syntax) {
            case Syntax::Word: {
                const i32 start = reader.getCursor();
                std::string result;
                while (reader.canRead() && !reader.isWhitespace(reader.peek())) {
                    result.push_back(reader.peek());
                    reader.skip();
                }
                if (result.empty()) {
                    reader.setCursor(start);
                    throw CommandException(CommandErrorType::StringExpected,
                        "Expected " + m_typeName + " at position " + std::to_string(start),
                        start);
                }
                return result;
            }
            case Syntax::ResourceId:
                return readResourceLocationId(reader).toString();
            case Syntax::ResourceOrTag: {
                // `#tag`（标签）或单个资源：与 ResourceOrTagArgument 同一套语法。
                bool tag = false;
                if (reader.canRead() && reader.peek() == '#') {
                    tag = true;
                    reader.skip();
                }
                const std::string id = readResourceLocationId(reader).toString();
                return tag ? "#" + id : id;
            }
        }
        return {};
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return m_typeName;
    }

    /// 资源类关键字（`dialog`、`loot_predicate`……）在网络上要写所属注册表键。
    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        if (m_syntax == Syntax::Word) {
            return nlohmann::json::object();
        }
        return nlohmann::json{{"registry", m_typeName}};
    }

    static std::shared_ptr<KeywordArgumentType> word(std::string typeName)
    {
        return std::make_shared<KeywordArgumentType>(std::move(typeName), Syntax::Word);
    }

    static std::shared_ptr<KeywordArgumentType> resourceId(std::string typeName)
    {
        return std::make_shared<KeywordArgumentType>(std::move(typeName), Syntax::ResourceId);
    }

    static std::shared_ptr<KeywordArgumentType> resourceOrTag(std::string typeName)
    {
        return std::make_shared<KeywordArgumentType>(std::move(typeName), Syntax::ResourceOrTag);
    }

private:
    std::string m_typeName;
    Syntax m_syntax;
};

/**
 * @brief `style` 参数类型（原版 {@code StyleArgument}，命令树里叫 {@code style}）
 */
class StyleKeywordArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        const std::string word = reader.readUnquotedString();
        if (word.empty()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::StringExpected, "Expected style", start);
        }
        return word;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "style";
    }

    /// 原版 StyleArgument 的 properties 是 Variant<String>：内置样式名写字符串，否则写资源位置。
    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"values",
            std::vector<std::string>{"bold", "italic", "underlined", "strikethrough", "obfuscated", "reset"}}};
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"{color:red}", "{}"};
    }
};

/**
 * @brief 方块位置参数，但对外报告成另一个类型名（原版 {@code heightmap} / {@code item_slots}）
 *
 * 原版这两个参数各自有一个 ArgumentTypeInfo（`heightmap`、`item_slots`）：语法是一个方块
 * 位置；后续允许的子节点（`heightmap` 要求再跟一个高度图名、`item_slots` 只是一个位置）
 * 由调用方按原版形状挂接。这里复用 {@link BlockPosArgumentType} 的坐标解析，只替换类型名，
 * 保证命令树里该节点的类型与网络 id 与原版一致。
 */
class PositionArgumentAliasType : public ArgumentType<Coordinates::Ptr> {
public:
    explicit PositionArgumentAliasType(std::string typeName)
        : m_typeName(std::move(typeName))
    {}

    [[nodiscard]] Coordinates::Ptr parse(StringReader& reader) override
    {
        BlockPosArgumentType positions;
        return positions.parse(reader);
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return m_typeName;
    }

    static std::shared_ptr<PositionArgumentAliasType> of(std::string typeName)
    {
        return std::make_shared<PositionArgumentAliasType>(std::move(typeName));
    }

private:
    std::string m_typeName;
};

/**
 * @brief 数值范围参数（原版 {@code int_range} / {@code float_range}）
 *
 * 语法与原版 AttributeArgument 共用的一套范围写法：`min..max`、`..max`、`min..`。
 * 命令树只关心类型名与网络 id，值本身在执行时再按原版语义求值。
 */
class NumericRangeArgumentType : public ArgumentType<std::string> {
public:
    explicit NumericRangeArgumentType(std::string typeName)
        : m_typeName(std::move(typeName))
    {}

    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        const std::string text = reader.readUnquotedString();
        if (text.empty()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::FloatExpected, "Expected range", start);
        }
        return text;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return m_typeName;
    }

    static std::shared_ptr<NumericRangeArgumentType> of(std::string typeName)
    {
        return std::make_shared<NumericRangeArgumentType>(std::move(typeName));
    }

private:
    std::string m_typeName;
};

// ========== NBT 路径 ==========

/**
 * @brief NBT 路径参数，但对外报告成 {@code nbt_selector}（/execute if ... 的条件）
 *
 * 与 {@link NbtPathArgumentType} 同一套语法，只是命令树里的类型名不同。
 */
class NbtSelectorArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string text;
        while (reader.canRead() && !reader.isWhitespace(reader.peek())) {
            text.push_back(reader.peek());
            reader.skip();
        }
        if (text.empty()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::InvalidNbtPath, "Expected nbt_selector", start);
        }
        return text;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "nbt_selector";
    }

    static std::shared_ptr<NbtSelectorArgumentType> nbtSelector()
    {
        return std::make_shared<NbtSelectorArgumentType>();
    }
};

/**
 * @brief 物品槽位列表参数（原版 {@code item_slots}）
 *
 * 语法：`container.0`、`weapon.mainhand`、`*`、`*[a,b]`（列表槽位）。这里只校验语法形状，
 * 槽位解释交给调用方（原版 SlotRanges）。
 */
class ItemSlotsArgumentType : public ArgumentType<std::string> {
public:
    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        std::string text;
        if (reader.canRead() && reader.peek() == '*') {
            text.push_back(reader.read());
            if (reader.canRead() && reader.peek() == '[') {
                i32 depth = 0;
                while (reader.canRead()) {
                    const char c = reader.read();
                    text.push_back(c);
                    if (c == '[') {
                        ++depth;
                    } else if (c == ']') {
                        --depth;
                        if (depth == 0) {
                            break;
                        }
                    }
                }
            }
        } else {
            while (reader.canRead()) {
                const char c = reader.peek();
                if (reader.isWhitespace(c) || c == '[' || c == ']') {
                    break;
                }
                text.push_back(c);
                reader.skip();
            }
        }
        if (text.empty()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::StringExpected, "Expected item slots", start);
        }
        return text;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "item_slots";
    }

    static std::shared_ptr<ItemSlotsArgumentType> itemSlots()
    {
        return std::make_shared<ItemSlotsArgumentType>();
    }
};

} // namespace command
} // namespace mc
