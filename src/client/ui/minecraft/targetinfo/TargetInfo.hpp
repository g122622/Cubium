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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mc::client::ui::minecraft::targetinfo {

/**
 * @brief 目标信息类型
 */
enum class TargetInfoKind : u8 {
    None,   ///< 无目标
    Block,  ///< 方块目标
    Entity, ///< 实体目标
};

/**
 * @brief 目标信息快照，用于向 UI 层传递当前注视目标的数据
 */
class TargetInfoSnapshot {
public:
    TargetInfoSnapshot(TargetInfoKind kind, std::string title, std::vector<std::string> details, u32 accentColor);

    /** @brief 创建一个表示"无目标"的空快照 */
    [[nodiscard]] static TargetInfoSnapshot none();

    [[nodiscard]] bool hasTarget() const noexcept { return m_kind != TargetInfoKind::None; }
    [[nodiscard]] TargetInfoKind kind() const noexcept { return m_kind; }
    [[nodiscard]] const std::string& title() const noexcept { return m_title; }
    [[nodiscard]] const std::vector<std::string>& details() const noexcept { return m_details; }
    [[nodiscard]] u32 accentColor() const noexcept { return m_accentColor; }

private:
    TargetInfoKind m_kind;
    std::string m_title;
    std::vector<std::string> m_details;
    u32 m_accentColor;
};

/**
 * @brief 将标识符（如方块/物品ID）转换为人类可读的名称
 *
 * 处理规则：分隔符替换为空格，每个单词首字母大写，驼峰处插入空格。
 * 例如 "oak_log" → "Oak Log"，"ironIngot" → "Iron Ingot"
 */
[[nodiscard]] std::string humanizeIdentifier(std::string_view identifier);

/** @brief 将资源定位符转换为人类可读名称，优先使用路径部分 */
[[nodiscard]] std::string humanizeResourceLocation(const ResourceLocation& location);

/** @brief 将距离格式化为字符串，例如 "3.14 m" */
[[nodiscard]] std::string formatDistance(f32 distance);

/** @brief 将方块坐标格式化为字符串，例如 "12, 64, -5" */
[[nodiscard]] std::string formatBlockPos(const BlockPos& pos);

/** @brief 将方向枚举转换为可读字符串 */
[[nodiscard]] std::string formatDirection(Direction direction);

} // namespace mc::client::ui::minecraft::targetinfo