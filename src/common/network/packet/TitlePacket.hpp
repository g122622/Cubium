#pragma once

#include "Packet.hpp"
#include "PacketSerializer.hpp"
#include "../../core/Types.hpp"
#include "../../util/text/ITextComponentFwd.hpp"
#include <memory>
#include <string>
#include <optional>

namespace mc::network {

/**
 * @brief 标题动作类型
 *
 * 定义标题包的动作类型，参考 MC 1.16.5 STitlePacket.Type
 */
enum class TitleAction : u8 {
    Title = 0,      // 设置主标题
    Subtitle = 1,   // 设置副标题
    Actionbar = 2,  // 设置动作栏
    Times = 3,      // 设置时间参数
    Clear = 4,      // 清除标题
    Reset = 5       // 重置标题（清除并重置时间参数）
};

/**
 * @brief 标题显示数据包 (S->C)
 *
 * 服务端向客户端发送标题显示指令。
 * 参考 MC 1.16.5 STitlePacket
 *
 * 协议格式:
 * | 字段        | 类型            | 说明                        |
 * |-------------|-----------------|----------------------------|
 * | action      | VarInt          | 动作类型 (TitleAction)      |
 * | text        | std::string (opt)    | 文本组件JSON（仅TITLE/SUBTITLE/ACTIONBAR）|
 * | fadeIn      | i32 (opt)       | 淡入时间（仅TIMES，tick）    |
 * | stay        | i32 (opt)       | 停留时间（仅TIMES，tick）    |
 * | fadeOut     | i32 (opt)       | 淡出时间（仅TIMES，tick）    |
 *
 * ## 使用场景
 *
 * 1. /title <player> title <json> - 设置主标题
 * 2. /title <player> subtitle <json> - 设置副标题
 * 3. /title <player> actionbar <json> - 设置动作栏
 * 4. /title <player> times <fadeIn> <stay> <fadeOut> - 设置时间
 * 5. /title <player> clear - 清除标题
 * 6. /title <player> reset - 重置标题
 *
 * ## 标题显示逻辑
 *
 * - 主标题：大字体，居中显示，支持淡入淡出
 * - 副标题：小字体，居中显示在主标题下方
 * - 动作栏：小字体，居中显示在快捷栏上方
 * - 时间参数：fadeIn/stay/fadeOut，单位为 tick (1/20 秒)
 */
class TitlePacket : public Packet {
public:
    TitlePacket();

    /**
     * @brief 创建主标题包
     * @param text 标题文本（JSON格式）
     */
    static TitlePacket createTitle(const std::string& text);

    /**
     * @brief 创建主标题包（从ITextComponent）
     * @param text 文本组件
     */
    static TitlePacket createTitle(const text::ITextComponent& text);

    /**
     * @brief 创建副标题包
     * @param text 副标题文本（JSON格式）
     */
    static TitlePacket createSubtitle(const std::string& text);

    /**
     * @brief 创建副标题包（从ITextComponent）
     * @param text 文本组件
     */
    static TitlePacket createSubtitle(const text::ITextComponent& text);

    /**
     * @brief 创建动作栏包
     * @param text 动作栏文本（JSON格式）
     */
    static TitlePacket createActionbar(const std::string& text);

    /**
     * @brief 创建动作栏包（从ITextComponent）
     * @param text 文本组件
     */
    static TitlePacket createActionbar(const text::ITextComponent& text);

    /**
     * @brief 创建时间设置包
     * @param fadeIn 淡入时间（tick）
     * @param stay 停留时间（tick）
     * @param fadeOut 淡出时间（tick）
     */
    static TitlePacket createTimes(i32 fadeIn, i32 stay, i32 fadeOut);

    /**
     * @brief 创建清除包
     */
    static TitlePacket createClear();

    /**
     * @brief 创建重置包
     */
    static TitlePacket createReset();

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getters ==========

    [[nodiscard]] TitleAction action() const { return m_action; }
    [[nodiscard]] const std::optional<std::string>& text() const { return m_text; }
    [[nodiscard]] i32 fadeIn() const { return m_fadeIn; }
    [[nodiscard]] i32 stay() const { return m_stay; }
    [[nodiscard]] i32 fadeOut() const { return m_fadeOut; }

    // ========== Setters ==========

    void setAction(TitleAction action) { m_action = action; }
    void setText(const std::string& text) { m_text = text; }
    void setText(std::optional<std::string>&& text) { m_text = std::move(text); }
    void setTimes(i32 fadeIn, i32 stay, i32 fadeOut);

private:
    /**
     * @brief 私有构造函数
     * @param action 动作类型
     */
    explicit TitlePacket(TitleAction action);

    /**
     * @brief 将ITextComponent序列化为JSON字符串
     * @param text 文本组件
     * @return JSON字符串
     */
    static std::string serializeText(const text::ITextComponent& text);

    TitleAction m_action = TitleAction::Clear;
    std::optional<std::string> m_text;  // 文本组件JSON（仅TITLE/SUBTITLE/ACTIONBAR）
    i32 m_fadeIn = -1;   // 淡入时间（tick），-1表示未设置
    i32 m_stay = -1;     // 停留时间（tick），-1表示未设置
    i32 m_fadeOut = -1;  // 淡出时间（tick），-1表示未设置
};

} // namespace mc::network
