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

#include "common/core/Result.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/TextStyle.hpp"

namespace mc::text {

/**
 * @brief Style ↔ NBT 复合标签编解码
 *
 * 对应原版 `Style.Serializer.MAP_CODEC`：样式字段**平铺**在承载它的复合标签里（没有
 * 嵌套的 "style" 子标签），键名与类型如下：
 *
 * | 键 | 类型 | 说明 |
 * |---|---|---|
 * | `color` | StringTag | 具名色写名字（`"red"`），自定义色写 `"#RRGGBB"` |
 * | `bold` / `italic` / `underlined` / `strikethrough` / `obfuscated` | ByteTag | 三态：只有显式设置过才写出 |
 * | `insertion` | StringTag | 点击后填入聊天框的文本 |
 * | `shadow_color` | IntTag | ARGB **打包整数**（`ExtraCodecs.ARGB_COLOR_CODEC` 的主形态） |
 * | `font` | StringTag | 字体资源位置（`FontDescription.CODEC` 只支持 Resource 变体） |
 *
 * 这是**唯一**的 Style NBT 实现：Component 的 NBT 序列化（`ComponentNbtSerialization`）与
 * 数字格式（`text/numbers/NumberFormat`）都走这里，避免出现两份会各自漂移的字段清单。
 *
 * TODO: `click_event` / `hover_event` 两个字段尚未编码。项目的 `ClickEvent` / `HoverEvent`
 * 仍是「action + value 字符串」模型，缺少 1.21.11 各动作的具名分量（`url` / `command` /
 * `path` / `id` / `count` / `components`）与 Component 化的 hover 载荷，无法产出合规 NBT；
 * 强行写旧形状会让真 Java 客户端在解码 Style 时失败。补齐需先重做事件模型，属独立任务。
 */
/// 把 Style 的字段平铺写入复合标签（不清理 out 中已有键）
void styleToNbt(nbt::tags::compound_tag& out, const Style& style);

/**
 * @brief 从复合标签的平铺字段还原 Style，对应原版 `Style.Serializer.MAP_CODEC` 的解码方向
 *
 * 数值型的样式标志（`{bold:1}` 这类 SNBT 写法）与原版 `Codec.BOOL` 经 `NbtOps` 的取值规则
 * 一致——任何数值标签都按其是否为 0 解释。`color` 无法解析为颜色时报错（原版同样是解码失败，
 * 而不是静默丢色）。未知键一律忽略。
 *
 * @param tag 承载样式字段的复合标签
 * @return 还原出的样式，或错误
 */
[[nodiscard]] Result<Style> styleFromNbt(const nbt::tags::compound_tag& tag);

} // namespace mc::text
