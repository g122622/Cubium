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
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/numbers/NumberFormat.hpp"

#include <optional>
#include <vector>

namespace mc::text::numbers {

/**
 * @brief NumberFormat 的 NBT 编解码（存档用）
 *
 * 对应原版 `NumberFormatTypes.CODEC`（= `byNameCodec().dispatchMap(...)`）：一个复合标签，含
 * `type`（类型注册名）以及该变体自己的字段——变体字段与 `type` **平铺**在同一层，不嵌套。
 *
 * | 变体 | NBT |
 * |---|---|
 * | blank | `{type:"minecraft:blank"}` |
 * | styled | `{type:"minecraft:styled", color:..., bold:...}`（样式字段见 `StyleNbt.hpp`） |
 * | fixed | `{type:"minecraft:fixed", value:<Component 标签>}` |
 *
 * 记分板存档里这两个字段的名字是 `format`（原版 `Objective.Packed` / `Score.Packed` 的
 * `optionalFieldOf("format")`），缺省表示「无自定义格式」。
 */

/**
 * @brief 把数字格式编码为 NBT 复合标签
 */
[[nodiscard]] nbt::tags::compound_tag numberFormatToNbt(const NumberFormat& format);

/**
 * @brief 从 NBT 复合标签还原数字格式
 *
 * @param tag 复合标签（含 `type`）
 * @return 还原出的格式，或错误（缺 `type` / 类型名未知 / 变体载荷非法）
 */
[[nodiscard]] Result<NumberFormat> numberFormatFromNbt(const nbt::tags::compound_tag& tag);

/**
 * @brief NumberFormat 的 Java 线格式编码（填 `ir::play::SetObjective/SetScore::numberFormat`）
 *
 * 对应原版 `NumberFormatTypes.STREAM_CODEC`：
 * `ByteBufCodecs.registry(Registries.NUMBER_FORMAT_TYPE).dispatch(...)`，
 * 即 `VarInt(注册表 id)` + 该变体的 `streamCodec` 载荷：
 * - blank：无载荷
 * - styled：`Style.Serializer.TRUSTED_STREAM_CODEC` = Style 的 NBT 根标签（`0x0A` + 字段 + `0x00`）
 * - fixed：`ComponentSerialization.TRUSTED_STREAM_CODEC` = Component 的 NBT 根标签
 *   （可折叠纯文本是 `0x08` + U16 长度 + UTF8）
 *
 * IR 里的 `numberFormat` 字段是 `std::vector<u8>`：**空 vector 表示 absent**（原版
 * `Optional<NumberFormat>` 的 false），非空即本函数产出的完整字节（含 typeId）。因此空格式
 * 也要占 1 个字节（`VarInt(0)`），不能返回空 vector。
 *
 * @param format 数字格式
 * @return 线格式字节（至少 1 字节）
 */
[[nodiscard]] std::vector<u8> numberFormatToWireBytes(const NumberFormat& format);

/**
 * @brief 把 `std::optional<NumberFormat>` 编码为 IR 字段值（nullopt → 空 vector）
 */
[[nodiscard]] std::vector<u8> numberFormatToWireBytes(const std::optional<NumberFormat>& format);

} // namespace mc::text::numbers