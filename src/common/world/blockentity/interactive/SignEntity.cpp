#include "world/blockentity/interactive/SignEntity.hpp"
#include "world/IWorld.hpp"
#include "entity/entities/player/Player.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/CommandRegistry.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/text/StringTextComponent.hpp"
#include "util/text/TextParser.hpp"
#include "util/math/Vector2.hpp"
#include <regex>

namespace mc {
namespace blockentity {

// ========== SignEntity 实现 ==========

SignEntity::SignEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Sign, pos) {
    // 初始化空文本行
    for (auto& line : m_lines) {
        line = std::make_unique<text::StringTextComponent>("");
    }
}

SignEntity::~SignEntity() = default;

const text::ITextComponent* SignEntity::getLine(i32 line) const {
    if (line < 0 || line >= LINE_COUNT) {
        return nullptr;
    }
    return m_lines[static_cast<std::size_t>(line)].get();
}

bool SignEntity::setLine(i32 line, std::unique_ptr<text::ITextComponent> text) {
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

bool SignEntity::setLineFromLegacy(i32 line, const std::string& text) {
    // 解析 § 代码格式的文本
    auto component = text::TextParser::parse(text);
    return setLine(line, std::move(component));
}

std::string SignEntity::getLineText(i32 line) const {
    const auto* component = getLine(line);
    return component ? component->getUnformattedText() : "";
}

std::string SignEntity::getLineFormatted(i32 line) const {
    const auto* component = getLine(line);
    return component ? component->getFormattedText() : "";
}

void SignEntity::setLines(std::array<std::unique_ptr<text::ITextComponent>, LINE_COUNT> lines) {
    for (std::size_t i = 0; i < LINE_COUNT; ++i) {
        if (lines[i]) {
            m_lines[i] = truncateText(std::move(lines[i]));
            MC_ASSERT_RELEASE(validateText(*m_lines[i]));
        }
    }
    setChanged();
}

void SignEntity::clearLines() {
    for (auto& line : m_lines) {
        line = std::make_unique<text::StringTextComponent>("");
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

bool SignEntity::validateText(const text::ITextComponent& text) {
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

std::unique_ptr<text::ITextComponent> SignEntity::truncateText(
    std::unique_ptr<text::ITextComponent> text) {
    std::string plainText = text->getUnformattedText();
    if (plainText.length() <= static_cast<size_t>(MAX_LINE_LENGTH)) {
        return text;
    }

    // 截断纯文本并创建新组件
    std::string truncated = plainText.substr(0, MAX_LINE_LENGTH);
    return std::make_unique<text::StringTextComponent>(std::move(truncated));
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

void SignEntity::save(nlohmann::json& data) const {
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

std::unique_ptr<BlockEntity> SignEntity::clone() const {
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

bool SignEntity::executeCommand(IWorld& world, Player& player) {
    MC_UNUSED(world);

    // MC 1.16.5: 参考 SignTileEntity.executeCommand()
    // 遍历所有行文本，检查并执行点击事件
    bool executedAny = false;

    // 尝试转换为 ServerPlayer 以执行命令
    ServerPlayer* serverPlayer = player.asServerPlayer();

    for (const auto& line : m_lines) {
        if (!line) {
            continue;
        }

        // 获取点击事件
        const text::Style& style = line->getStyle();
        const text::ClickEvent* clickEvent = style.getClickEvent();

        if (clickEvent && clickEvent->isValid()) {
            // 执行点击事件
            switch (clickEvent->getAction()) {
                case text::ClickAction::RunCommand: {
                    // MC 1.16.5: 服务端执行命令
                    // 参考 SignTileEntity.executeCommand():
                    // playerIn.getServer().getCommandManager().handleCommand(
                    //     this.getCommandSource((ServerPlayerEntity)playerIn), clickevent.getValue());

                    if (serverPlayer != nullptr && serverPlayer->getServer() != nullptr) {
                        // 获取命令字符串
                        std::string command = clickEvent->getValue();

                        // 如果命令不以 '/' 开头，自动添加
                        if (!command.empty() && command[0] != '/') {
                            command = "/" + command;
                        }

                        // 创建命令源
                        // MC 1.16.5: 告示牌命令源的权限级别为 2，位置为告示牌位置
                        mc::command::ServerCommandSource source(
                            serverPlayer->getServer(),
                            serverPlayer,
                            serverPlayer->getWorld(),
                            Vector3d(
                                static_cast<f64>(m_pos.x) + 0.5,
                                static_cast<f64>(m_pos.y) + 0.5,
                                static_cast<f64>(m_pos.z) + 0.5
                            ),
                            Vector2f(0.0f, 0.0f),
                            2,  // 权限级别 2（相当于 OP 级别）
                            serverPlayer->playerId(),
                            serverPlayer->username()
                        );

                        // 执行命令
                        auto result = serverPlayer->getServer()->commandRegistry().execute(command, source);

                        if (result.success()) {
                            executedAny = true;
                        } else {
                            // 命令执行失败，发送错误消息给玩家
                            serverPlayer->sendSystemMessage("§c" + result.error().message());
                        }
                    }
                    break;
                }
                case text::ClickAction::SuggestCommand: {
                    // MC 1.16.5: 客户端功能 - 将命令填入聊天输入框
                    // 参考 Screen.handleComponentClicked() 中的 SUGGEST_COMMAND 处理
                    // this.insertText(clickevent.getValue(), true);
                    // 当前为服务端实现，仅记录日志，实际功能需要在客户端实现
                    MC_UNUSED(player);
                    executedAny = true;
                    break;
                }
                case text::ClickAction::OpenUrl: {
                    // MC 1.16.5: 客户端功能 - 打开 URL
                    // 参考 Screen.handleComponentClicked() 中的 OPEN_URL 处理
                    // 当前为服务端实现，仅记录日志，实际功能需要在客户端实现
                    MC_UNUSED(player);
                    executedAny = true;
                    break;
                }
                case text::ClickAction::CopyToClipboard: {
                    // MC 1.16.5: 客户端功能 - 复制到剪贴板
                    // 参考 Screen.handleComponentClicked() 中的 COPY_TO_CLIPBOARD 处理
                    // this.minecraft.keyboardListener.setClipboardString(clickevent.getValue());
                    // 当前为服务端实现，仅记录日志，实际功能需要在客户端实现
                    MC_UNUSED(player);
                    executedAny = true;
                    break;
                }
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
                    // 执行子组件中的命令
                    if (serverPlayer != nullptr && serverPlayer->getServer() != nullptr) {
                        std::string command = siblingClick->getValue();
                        if (!command.empty() && command[0] != '/') {
                            command = "/" + command;
                        }

                        mc::command::ServerCommandSource source(
                            serverPlayer->getServer(),
                            serverPlayer,
                            serverPlayer->getWorld(),
                            Vector3d(
                                static_cast<f64>(m_pos.x) + 0.5,
                                static_cast<f64>(m_pos.y) + 0.5,
                                static_cast<f64>(m_pos.z) + 0.5
                            ),
                            Vector2f(0.0f, 0.0f),
                            2,
                            serverPlayer->playerId(),
                            serverPlayer->username()
                        );

                        serverPlayer->getServer()->commandRegistry().execute(command, source);
                        executedAny = true;
                    }
                }
            }
        }
    }

    return executedAny;
}

} // namespace blockentity
} // namespace mc
