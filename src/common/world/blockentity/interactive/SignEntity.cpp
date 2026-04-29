#include "world/blockentity/interactive/SignEntity.hpp"
#include "world/IWorld.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"
#include <regex>

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 判断字符是否为告示牌允许的控制字符。
 *
 * 当前仅允许换行和制表等常见空白控制符，拒绝其他不可见控制字符。
 */
[[nodiscard]] bool isAllowedControlCharacter(char c) {
    return c == '\n' || c == '\r' || c == '\t';
}

/**
 * @brief 从文本中提取命令。
 *
 * MC 1.16.5 使用 ITextComponent 存储富文本和点击事件。
 * 在 ITextComponent 系统实现前，暂时使用简单的命令检测：
 * 如果文本以 "/" 开头，则视为命令。
 *
 * @param text 文本内容
 * @return 如果是命令则返回命令内容（不含 "/"），否则返回空
 */
[[nodiscard]] String extractCommand(const String& text) {
    if (text.empty()) {
        return "";
    }

    // 检查是否以 "/" 开头的命令
    if (text[0] == '/') {
        return text.substr(1);
    }

    return "";
}

} // namespace

// ========== SignEntity 实现 ==========

SignEntity::SignEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Sign, pos)
    , m_lines{{"", "", "", ""}} {
}

SignEntity::~SignEntity() = default;

const String& SignEntity::getLine(i32 line) const {
    static const String empty = "";
    if (line < 0 || line >= LINE_COUNT) {
        return empty;
    }
    return m_lines[static_cast<std::size_t>(line)];
}

bool SignEntity::setLine(i32 line, const String& text) {
    if (line < 0 || line >= LINE_COUNT) {
        return false;
    }

    // 验证并截断文本
    String validated = truncateText(text);
    if (!validateText(validated)) {
        return false;
    }

    m_lines[static_cast<std::size_t>(line)] = validated;
    setChanged();
    return true;
}

void SignEntity::setLines(const std::array<String, LINE_COUNT>& lines) {
    for (std::size_t i = 0; i < m_lines.size(); ++i) {
        m_lines[i] = truncateText(lines[i]);
        MC_ASSERT_RELEASE(validateText(m_lines[i]));
    }
    setChanged();
}

void SignEntity::clearLines() {
    for (auto& line : m_lines) {
        line.clear();
    }
    setChanged();
}

void SignEntity::setEditable(bool editable) {
    if (m_editable != editable) {
        m_editable = editable;
        setChanged();
    }
}

void SignEntity::setEditor(Player* player) {
    m_editor = player;
}

void SignEntity::setTextColor(i32 color) {
    if (m_textColor != color) {
        m_textColor = color;
        setChanged();
    }
}

void SignEntity::setGlowing(bool glowing) {
    if (m_glowing != glowing) {
        m_glowing = glowing;
        setChanged();
    }
}

void SignEntity::tick(IWorld& world) {
    MC_UNUSED(world);
    // 告示牌不需要 tick 更新
}

bool SignEntity::validateText(const String& text) {
    for (char c : text) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 0x20 && !isAllowedControlCharacter(c)) {
            return false;
        }
    }

    return true;
}

String SignEntity::truncateText(const String& text) {
    if (text.length() <= static_cast<size_t>(MAX_LINE_LENGTH)) {
        return text;
    }
    return text.substr(0, MAX_LINE_LENGTH);
}

bool SignEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载文本
    if (data.contains("lines")) {
        const auto& linesJson = data["lines"];
        if (linesJson.is_array()) {
            for (size_t i = 0; i < LINE_COUNT && i < linesJson.size(); ++i) {
                const String line = truncateText(linesJson[i].get<String>());
                if (!validateText(line)) {
                    return false;
                }
                m_lines[i] = line;
            }
        }
    }

    // 加载可编辑状态
    if (data.contains("editable")) {
        m_editable = data["editable"].get<bool>();
    }

    // 加载文本颜色
    if (data.contains("color")) {
        m_textColor = data["color"].get<i32>();
    }

    // 加载发光状态
    if (data.contains("glowing")) {
        m_glowing = data["glowing"].get<bool>();
    }

    return true;
}

void SignEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    // 保存文本
    nlohmann::json linesJson = nlohmann::json::array();
    for (const auto& line : m_lines) {
        linesJson.push_back(line);
    }
    data["lines"] = linesJson;

    // 保存可编辑状态
    data["editable"] = m_editable;

    // 保存文本颜色
    data["color"] = m_textColor;

    // 保存发光状态
    data["glowing"] = m_glowing;
}

std::unique_ptr<BlockEntity> SignEntity::clone() const {
    auto clone = std::make_unique<SignEntity>(m_pos);
    clone->m_lines = m_lines;
    clone->m_editable = m_editable;
    clone->m_textColor = m_textColor;
    clone->m_glowing = m_glowing;
    return clone;
}

bool SignEntity::executeCommand(IWorld& world, Player& player) {
    MC_UNUSED(world);

    // MC 1.16.5: 参考 SignTileEntity.executeCommand()
    // 遍历所有行文本，提取并执行命令
    // 在完整的 ITextComponent 实现后，应该检查 Style.getClickEvent()

    bool executedAny = false;

    for (const String& line : m_lines) {
        String command = extractCommand(line);
        if (!command.empty()) {
            // 执行命令
            // TODO: 当命令系统完善后，应该使用 player.getServer().getCommandManager().execute()
            // 当前仅标记已找到命令，实际执行需要命令系统支持
            executedAny = true;

            // 临时实现：直接通过玩家执行命令
            // 当完整的命令系统集成后，应该使用：
            // player.getServer().getCommandManager().handleCommand(commandSource, command);
            MC_UNUSED(command);
        }
    }

    return executedAny;
}

} // namespace blockentity
} // namespace mc
