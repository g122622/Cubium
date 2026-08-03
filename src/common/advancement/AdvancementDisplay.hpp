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

#include "AdvancementFrame.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc::text {
class ITextComponent;
}

namespace mc::advancement {

/**
 * @brief 成就显示信息
 *
 * 定义成就在UI中的显示方式，包括图标、标题、描述、框架类型等。
 * 参考 MC 1.16.5: net.minecraft.advancements.DisplayInfo
 *
 * 只有具有 DisplayInfo 的成就才会在成就界面中显示。
 * 没有 DisplayInfo 的成就作为隐藏的进度追踪节点。
 */
class AdvancementDisplay {
public:
    AdvancementDisplay() = default;

    /**
     * @brief 构造显示信息
     * @param icon 图标物品
     * @param title 标题文本组件
     * @param description 描述文本组件
     * @param frame 框架类型
     * @param showToast 是否显示Toast通知
     * @param announceToChat 是否在聊天中公告
     * @param hidden 是否隐藏（完成后才显示）
     * @param background 背景纹理（仅根成就）
     */
    AdvancementDisplay(ItemStack icon,
        std::unique_ptr<text::ITextComponent> title,
        std::unique_ptr<text::ITextComponent> description,
        AdvancementFrame frame,
        bool showToast,
        bool announceToChat,
        bool hidden,
        std::optional<ResourceLocation> background = std::nullopt);

    // 移动构造和赋值
    AdvancementDisplay(AdvancementDisplay&&) noexcept = default;
    AdvancementDisplay& operator=(AdvancementDisplay&&) noexcept = default;

    // 禁止拷贝（因为包含 unique_ptr）
    AdvancementDisplay(const AdvancementDisplay&) = delete;
    AdvancementDisplay& operator=(const AdvancementDisplay&) = delete;

    // ========== 访问器 ==========

    /**
     * @brief 获取图标物品
     */
    [[nodiscard]] const ItemStack& getIcon() const noexcept { return m_icon; }

    /**
     * @brief 获取标题
     */
    [[nodiscard]] const text::ITextComponent& getTitle() const noexcept { return *m_title; }

    /**
     * @brief 获取描述
     */
    [[nodiscard]] const text::ITextComponent& getDescription() const noexcept { return *m_description; }

    /**
     * @brief 获取标题的可修改引用（用于深拷贝）
     */
    [[nodiscard]] std::unique_ptr<text::ITextComponent> getTitleCopy() const;

    /**
     * @brief 获取框架类型
     */
    [[nodiscard]] AdvancementFrame getFrame() const noexcept { return m_frame; }

    /**
     * @brief 是否显示Toast通知
     */
    [[nodiscard]] bool shouldShowToast() const noexcept { return m_showToast; }

    /**
     * @brief 是否在聊天中公告
     */
    [[nodiscard]] bool shouldAnnounceToChat() const noexcept { return m_announceToChat; }

    /**
     * @brief 是否隐藏
     */
    [[nodiscard]] bool isHidden() const noexcept { return m_hidden; }

    /**
     * @brief 获取背景纹理
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getBackground() const noexcept { return m_background; }

    // ========== UI布局坐标 ==========

    /**
     * @brief 获取X坐标（由AdvancementTreeNode计算）
     */
    [[nodiscard]] f32 getX() const noexcept { return m_x; }

    /**
     * @brief 获取Y坐标（由AdvancementTreeNode计算）
     */
    [[nodiscard]] f32 getY() const noexcept { return m_y; }

    /**
     * @brief 设置X坐标
     */
    void setX(f32 x) noexcept { m_x = x; }

    /**
     * @brief 设置Y坐标
     */
    void setY(f32 y) noexcept { m_y = y; }

    // ========== 序列化 ==========

    /**
     * @brief 从JSON解析
     * @param json JSON对象
     * @return 显示信息或错误
     */
    static Result<AdvancementDisplay> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    ItemStack m_icon;
    std::unique_ptr<text::ITextComponent> m_title;
    std::unique_ptr<text::ITextComponent> m_description;
    AdvancementFrame m_frame = AdvancementFrame::Task;
    bool m_showToast = true;
    bool m_announceToChat = true;
    bool m_hidden = false;
    std::optional<ResourceLocation> m_background;

    // UI布局坐标
    f32 m_x = 0.0f;
    f32 m_y = 0.0f;
};

} // namespace mc::advancement
