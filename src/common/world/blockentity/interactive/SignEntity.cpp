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
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/TextEvents.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/serialization/NbtHelper.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/text/StringTextComponent.hpp"
#include "util/text/TextParser.hpp"
#include "world/IWorld.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

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

    // 涂蜡后的告示牌不允许修改文字
    if (m_waxed) {
        return false;
    }

    // 验证并截断文本
    auto truncated = _truncateText(std::move(text));
    if (!_validateText(*truncated)) {
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
    // 涂蜡后的告示牌不允许修改文字
    if (m_waxed) {
        return;
    }

    for (std::size_t i = 0; i < LINE_COUNT; ++i) {
        if (lines[i]) {
            m_lines[i] = _truncateText(std::move(lines[i]));
            MC_ASSERT_RELEASE(_validateText(*m_lines[i]));
        }
    }
    setChanged();
}

void SignEntity::clearLines()
{
    // 涂蜡后的告示牌不允许修改文字
    if (m_waxed) {
        return;
    }

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

void SignEntity::setAllowedPlayerEditor(const std::string& uuid)
{
    m_playerWhoMayEdit = uuid;
    // 编辑者状态是运行时瞬态数据，不需要调用 setChanged()
    // （不需要保存到磁盘，因为编辑锁在服务器重启后会自动清除）
}

bool SignEntity::otherPlayerIsEditing(const Player& player) const
{
    // 对应 MC Java 的 SignBlock.otherPlayerIsEditingSign()
    // 当 playerWhoMayEdit 已设置且与当前交互玩家不同时返回 true
    return !m_playerWhoMayEdit.empty() && m_playerWhoMayEdit != player.uuid();
}

bool SignEntity::playerIsTooFarAwayToEdit(IWorld& world, const std::string& uuid) const
{
    // 对应 MC Java 的 SignBlockEntity.playerIsTooFarAwayToEdit()
    // 1. 如果找不到该 UUID 对应的玩家（已离线），返回 true
    // 2. 如果玩家距离告示牌超过 MAX_EDIT_DISTANCE，返回 true
    Entity* entity = world.getEntityByUuid(uuid);
    if (entity == nullptr) {
        return true;
    }
    Player* player = dynamic_cast<Player*>(entity);
    if (player == nullptr) {
        return true;
    }
    // 计算玩家到告示牌的距离
    Vector3 signCenter = m_pos.center();
    f32 distSq = player->position().distanceSquared(signCenter);
    return distSq > MAX_EDIT_DISTANCE * MAX_EDIT_DISTANCE;
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

bool SignEntity::setWaxed(bool waxed)
{
    if (m_waxed != waxed) {
        m_waxed = waxed;
        setChanged();
        return true;
    }
    return false;
}

void SignEntity::tick(IWorld& world)
{
    // 对应 MC Java 的 SignBlockEntity.tick()
    // 定期检查编辑者是否距离过远或已离线，如果是则清除编辑锁
    if (!m_playerWhoMayEdit.empty()) {
        if (playerIsTooFarAwayToEdit(world, m_playerWhoMayEdit)) {
            m_playerWhoMayEdit.clear();
        }
    }
}

bool SignEntity::_validateText(const text::ITextComponent& text)
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

std::unique_ptr<text::ITextComponent> SignEntity::_truncateText(std::unique_ptr<text::ITextComponent> text)
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
                    m_lines[i] = _truncateText(std::move(m_lines[i]));
                    if (!_validateText(*m_lines[i])) {
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

    // 加载涂蜡状态
    if (data.contains("is_waxed")) {
        m_waxed = data["is_waxed"].get<bool>();
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

    // 保存涂蜡状态
    data["is_waxed"] = m_waxed;
}

bool SignEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    // 加载文本行
    auto* linesList = mc::entity::serialization::nbt_helper::tryGetList(tag, "lines");
    if (linesList != nullptr && linesList->element_id() == nbt::TagId::String) {
        auto& stringList = dynamic_cast<const nbt::tags::string_list_tag&>(*linesList);
        for (std::size_t i = 0; i < LINE_COUNT && i < stringList.value.size(); ++i) {
            const auto& lineText = stringList.value[i];
            m_lines[i] = text::TextParser::parse(lineText);
            if (m_lines[i]) {
                m_lines[i] = _truncateText(std::move(m_lines[i]));
                if (!_validateText(*m_lines[i])) {
                    m_lines[i] = std::make_unique<text::StringTextComponent>("");
                }
            } else {
                m_lines[i] = std::make_unique<text::StringTextComponent>("");
            }
        }
    }

    // 加载可编辑状态
    if (auto val = mc::entity::serialization::nbt_helper::tryGetBool(tag, "editable")) {
        m_editable = *val;
    }

    // 加载文本颜色
    if (auto val = mc::entity::serialization::nbt_helper::tryGetInt(tag, "color")) {
        m_textColor = *val;
    }

    // 加载发光状态
    if (auto val = mc::entity::serialization::nbt_helper::tryGetBool(tag, "glowing")) {
        m_glowing = *val;
    }

    // 加载涂蜡状态
    if (auto val = mc::entity::serialization::nbt_helper::tryGetBool(tag, "is_waxed")) {
        m_waxed = *val;
    }

    return true;
}

void SignEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    // 保存文本行（NBT 格式使用字符串列表）
    auto linesList = std::make_unique<nbt::tags::string_list_tag>();
    for (const auto& line : m_lines) {
        if (line) {
            linesList->value.push_back(line->getUnformattedText());
        } else {
            linesList->value.push_back("");
        }
    }
    tag.value.emplace("lines", std::move(linesList));

    // 保存可编辑状态
    tag.put("editable", static_cast<i8>(m_editable ? 1 : 0));

    // 保存文本颜色
    tag.put("color", static_cast<i32>(m_textColor));

    // 保存发光状态
    tag.put("glowing", static_cast<i8>(m_glowing ? 1 : 0));

    // 保存涂蜡状态
    tag.put("is_waxed", static_cast<i8>(m_waxed ? 1 : 0));
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
    cloned->m_waxed = m_waxed;
    return cloned;
}

bool SignEntity::executeCommand(IWorld& world, Player& player)
{
    MC_UNUSED(world);

    // 遍历所有行文本，检查并执行点击事件
    // 注意：此方法在 mc_common 库中，无法直接访问服务端的命令系统。
    // 命令执行逻辑由服务端的 SignBlock 交互处理。
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
                    // 出于安全原因，不自动执行 OpenFile
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
