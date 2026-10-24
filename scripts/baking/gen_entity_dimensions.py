#!/usr/bin/env python3
"""从原版实体尺寸导出结果生成 C++ 数据表。

输入是 ``tests/diff/java`` 下 ``EntityDimensionsExport`` 的输出：

    java dev.horizonlab.diff.EntityDimensionsExport entity_dimensions.json

本脚本把它转成 ``src/common/entity/registry/VanillaEntityDimensions.cpp``，
供 ``EntityRegistry::registerType`` 按实体名统一校正碰撞箱（宽、高、眼高、是否固定）。

为什么整组覆盖而不只校正眼高：原版把尺寸定义在 *实体类* 里（``EntityType.Builder.sized``），
本项目的注册表另写一份；两边一旦不同步就会出现"僵尸撞墙判定偏宽""海螺眼睛比身体还高"
这类问题。让原版数据成为唯一来源，注册点的 ``.size(w, h)`` 只是兜底。

用法：

    python scripts/baking/gen_entity_dimensions.py <entity_dimensions.json>
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
OUTPUT = REPO_ROOT / "src/common/entity/registry/VanillaEntityDimensions.cpp"

HEADER = """/*
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

// 本文件由 scripts/baking/gen_entity_dimensions.py 生成，请勿手改。
// 数据来源：原版 1.21.11 服务端 jar 的 EntityType#getDimensions()（见
// tests/diff/java/dev/horizonlab/diff/EntityDimensionsExport.java）。
"""

PROLOGUE = """#include "common/entity/core/EntitySize.hpp"

#include <charconv>
#include <string_view>
#include <unordered_map>

namespace mc::entity {

namespace {

/// 一个实体类型的碰撞箱尺寸。
struct VanillaSize {
    f32 width;
    f32 height;
    f32 eyeHeight;
    bool fixed;
};

/// 实体名 → 原版碰撞箱尺寸。
const std::unordered_map<std::string_view, VanillaSize>& sizeTable()
{
    static const std::unordered_map<std::string_view, VanillaSize> table = {
"""

EPILOGUE = """    };
    return table;
}

} // namespace

std::optional<EntitySize> vanillaEntitySize(std::string_view entityName)
{
    const auto& table = sizeTable();
    const auto found = table.find(entityName);
    if (found == table.end()) {
        return std::nullopt;
    }
    const VanillaSize& size = found->second;
    return EntitySize(size.width, size.height, size.eyeHeight, size.fixed);
}

std::optional<f32> vanillaEyeHeight(std::string_view entityName)
{
    const auto& table = sizeTable();
    const auto found = table.find(entityName);
    if (found == table.end()) {
        return std::nullopt;
    }
    return found->second.eyeHeight;
}

} // namespace mc::entity
"""


def literal(value: str) -> str:
    """把导出的浮点文本写成 C++ f32 字面量。

    直接复用 Java 的 Float.toString 结果：它是最短往返表示，同一个 f32 读进来再
    写回去位模式不变。这里只补 `f` 后缀并处理 `NaN`/`Infinity` 这类 Java 写法。
    """
    text = value.strip()
    if text in {"NaN", "Infinity", "-Infinity"}:
        raise ValueError(f"实体尺寸里出现非有限值 {text}，无法写进静态表")
    if "." not in text and "e" not in text.lower():
        text += ".0"
    return f"{text}f"


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2

    entries = json.loads(Path(argv[1]).read_text(encoding="utf-8"))
    lines = [HEADER, PROLOGUE]
    for entry in entries:
        lines.append(
            f'        {{"{entry["entity"]}", '
            f'{{{literal(entry["width"])}, {literal(entry["height"])}, '
            f'{literal(entry["eyeHeight"])}, {"true" if entry["fixed"] else "false"}}}}},',
        )
    lines.append(EPILOGUE)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {OUTPUT.relative_to(REPO_ROOT)} ({len(entries)} entities)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))