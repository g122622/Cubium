#include "world/blockentity/interactive/SignEntity.hpp"
#include "world/IWorld.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"

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

} // namespace blockentity
} // namespace mc
