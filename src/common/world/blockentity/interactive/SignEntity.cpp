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

#include "world/blockentity/interactive/SignEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/text/StringTextComponent.hpp"
#include "util/text/TextParser.hpp"
#include "world/IWorld.hpp"
#include <regex>

namespace mc {
namespace blockentity {

// ========== SignEntity 实现 ==========

SignEntity::SignEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Sign, pos)
{
    // 初始化空文本行
    for (auto& line : m_lines) {
        line = std::make_unique<text::StringTextComponent>("");
    }
}

SignEntity::~SignEntity() = default;

const text::ITextComponent* SignEntity::getLine(i32 line) const
{
    if (line < 0 || line >= LINE_COUNT) {
        return nullptr;
    }
    return m_lines[static_cast<std::size_t>(line)].get();
}

bool SignEntity::setLine(i32 line, std::unique_ptr<text::ITextComponent> text)
{
    if (line < 0 || line >= LINE_COUNT || !text) {
        return false;
    }

    // 验证并截断文本
    auto truncated = truncateText(std::move(text));
    if (!validateText(*truncated)) {
        return false;
    }

    m_lines[static_cast<std::size_t>(line)] = std::move(truncated);
    setChanged();
    return true;
}

bool SignEntity::setLineFromLegacy(i32 line, const std::string& text)
{
    // 解析 § 代码格式的文本
    auto component = text::TextParser::parse(text);
    return setLine(line, std::move(component));
}

std::string SignEntity::getLineText(i32 line) const
{
    const auto* component = getLine(line);
    return component ? component->getUnformattedText() : "";
}

std::string SignEntity::getLineFormatted(i32 line) const
{
    const auto* component = getLine(line);
    return component ? component->getFormattedText() : "";
}

void SignEntity::setLines(std::array<std::unique_ptr<text::ITextComponent>, LINE_COUNT> lines)
{
    for (std::size_t i = 0; i < LINE_COUNT; ++i) {
        if (lines[i]) {
            m_lines[i] = truncateText(std::move(lines[i]));
            MC_ASSERT_RELEASE(validateText(*m_lines[i]));
        }
    }
    setChanged();
}

void SignEntity::clearLines()
{
    for (auto& line : m_lines) {
        line = std::make_unique<text::StringTextComponent>("");
    }
    setChanged();
}

void SignEntity::setEditable(bool editable)
{
    if (m_editable != editable) {
        m_editable = editable;
        setChanged();
    }
}

void SignEntity::setEditor(Player* player)
{
    m_editor = player;
}

void SignEntity::setTextColor(i32 color)
{
    if (m_textColor != color) {
        m_textColor = color;
        setChanged();
    }
}

void SignEntity::setGlowing(bool glowing)
{
    if (m_glowing != glowing) {
        m_glowing = glowing;
        setChanged();
    }
}

void SignEntity::tick(IWorld& world)
{
    MC_UNUSED(world);
    // 告示牌不需要 tick 更新
}

bool SignEntity::validateText(const text::ITextComponent& text)
{
    // 验证纯文本内容中的控制字符
    std::string plainText = text.getUnformattedText();
    for (char c : plainText) {
        const unsigned char uc = static_cast<unsigned char>(c);
        // 只允许可打印字符和常见空白符
        if (uc < 0x20 && c != '\n' && c != '\r' && c != '\t') {
            return false;
        }
    }
    return true;
}

std::unique_ptr<text::ITextComponent> SignEntity::truncateText(std::unique_ptr<text::ITextComponent> text)
{
    std::string plainText = text->getUnformattedText();
    if (plainText.length() <= static_cast<size_t>(MAX_LINE_LENGTH)) {
        return text;
    }

    // 截断纯文本并创建新组件
    std::string truncated = plainText.substr(0, MAX_LINE_LENGTH);
    return std::make_unique<text::StringTextComponent>(std::move(truncated));
}

bool SignEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载文本
    if (data.contains("lines")) {
        const auto& linesJson = data["lines"];
        if (linesJson.is_array()) {
            for (size_t i = 0; i < LINE_COUNT && i < linesJson.size(); ++i) {
                const auto& lineJson = linesJson[i];
                if (lineJson.is_string()) {
                    // 旧格式：纯字符串
                    m_lines[i] = text::TextParser::parse(lineJson.get<std::string>());
                } else if (lineJson.is_object()) {
                    // 新格式：JSON 文本组件
                    m_lines[i] = text::ITextComponent::fromJson(lineJson);
                }
                if (m_lines[i]) {
                    m_lines[i] = truncateText(std::move(m_lines[i]));
                    if (!validateText(*m_lines[i])) {
                        m_lines[i] = std::make_unique<text::StringTextComponent>("");
                    }
                } else {
                    m_lines[i] = std::make_unique<text::StringTextComponent>("");
                }
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

void SignEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    // 保存文本（新格式）
    nlohmann::json linesJson = nlohmann::json::array();
    for (const auto& line : m_lines) {
        if (line) {
            linesJson.push_back(line->toJson());
        } else {
            linesJson.push_back(nlohmann::json::object());
        }
    }
    data["lines"] = linesJson;

    // 保存可编辑状态
    data["editable"] = m_editable;

    // 保存文本颜色
    data["color"] = m_textColor;

    // 保存发光状态
    data["glowing"] = m_glowing;
}

std::unique_ptr<BlockEntity> SignEntity::clone() const
{
    auto cloned = std::make_unique<SignEntity>(m_pos);
    for (std::size_t i = 0; i < m_lines.size(); ++i) {
        if (m_lines[i]) {
            cloned->m_lines[i] = m_lines[i]->deepCopy();
        }
    }
    cloned->m_editable = m_editable;
    cloned->m_textColor = m_textColor;
    cloned->m_glowing = m_glowing;
    return cloned;
}

bool SignEntity::executeCommand(IWorld& world, Player& player)
{
    MC_UNUSED(world);

    // MC 1.16.5: 参考 SignTileEntity.executeCommand()
    // 遍历所有行文本，检查并执行点击事件
    //
    // 注意：此方法在 mc_common 库中，无法直接访问服务端的命令系统。
    // 命令执行逻辑由服务端的 SignBlock 交互处理。
    //
    // 此方法仅检查是否存在有效的点击事件，实际命令执行在服务端完成。
    bool hasCommand = false;

    for (const auto& line : m_lines) {
        if (!line) {
            continue;
        }

        // 获取点击事件
        const text::Style& style = line->getStyle();
        const text::ClickEvent* clickEvent = style.getClickEvent();

        if (clickEvent && clickEvent->isValid()) {
            switch (clickEvent->getAction()) {
                case text::ClickAction::RunCommand:
                case text::ClickAction::SuggestCommand:
                case text::ClickAction::OpenUrl:
                case text::ClickAction::CopyToClipboard:
                    hasCommand = true;
                    break;
                case text::ClickAction::OpenFile:
                    // MC 1.16.5: 出于安全原因，不自动执行 OpenFile
                    break;
            }
        }

        // 递归检查子组件
        for (const auto& sibling : line->getSiblings()) {
            if (sibling) {
                const text::Style& siblingStyle = sibling->getStyle();
                const text::ClickEvent* siblingClick = siblingStyle.getClickEvent();
                if (siblingClick && siblingClick->isValid() &&
                    siblingClick->getAction() == text::ClickAction::RunCommand) {
                    hasCommand = true;
                }
            }
        }
    }

    return hasCommand;
}

} // namespace blockentity
} // namespace mc
