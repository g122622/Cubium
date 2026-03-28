#include "world/blockentity/interactive/SignEntity.hpp"
#include "world/IWorld.hpp"
#include "entity/Player.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

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
    return m_lines[line];
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

    m_lines[line] = validated;
    setChanged();
    return true;
}

void SignEntity::setLines(const std::array<String, LINE_COUNT>& lines) {
    for (i32 i = 0; i < LINE_COUNT; ++i) {
        m_lines[i] = truncateText(lines[i]);
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
    // 告示牌不需要tick更新
}

bool SignEntity::validateText(const String& text) {
    // 检查是否包含非法字符
    // TODO: 实现完整的文本验证
    MC_UNUSED(text);
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
                m_lines[i] = linesJson[i].get<String>();
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
