#!/usr/bin/env python3
"""从原版附魔能力导出结果生成 C++ 数据表。

输入是 ``tests/diff/java`` 下 ``ItemEnchantableExport`` 的输出（只含非 0 项）：

    java dev.horizonlab.diff.ItemEnchantableExport item_enchantable.json

本脚本转成 ``src/common/item/VanillaItemEnchantable.cpp``，由 ``Items::initialize()``
在注册完成后统一套用。

为什么和稀有度一样走生成表：附魔能力是纯数据，75 个物品分属 11 种取值，散落在
上百处注册调用里逐个手写既难核对也易漏；让原版数据做唯一来源，差分测试即可全量比对。
缺省值 0（不可附魔）不导出，表里没有的物品即为 0。
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
OUTPUT = REPO_ROOT / "src/common/item/VanillaItemEnchantable.cpp"

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

// 本文件由 scripts/baking/gen_item_enchantable.py 生成，请勿手改。
// 数据来源：原版 1.21.11 服务端 jar 的 DataComponents.ENCHANTABLE（见
// tests/diff/java/dev/horizonlab/diff/ItemEnchantableExport.java）。只含非 0 项。
"""


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2

    entries = json.loads(Path(argv[1]).read_text(encoding="utf-8"))
    lines = [
        HEADER,
        '#include "VanillaItemEnchantable.hpp"',
        "",
        "#include <string_view>",
        "#include <unordered_map>",
        "",
        "namespace mc {",
        "",
        "namespace {",
        "",
        "/// 物品名 → 原版附魔能力（只含非 0 项）。",
        "const std::unordered_map<std::string_view, i32>& enchantableTable()",
        "{",
        "    static const std::unordered_map<std::string_view, i32> table = {",
    ]
    for entry in entries:
        value = entry["enchantable"]
        if not isinstance(value, int) or value <= 0:
            raise ValueError(f"附魔能力必须是正整数，得到 {value!r}（物品 {entry['item']}）")
        lines.append(f'        {{"{entry["item"]}", {value}}},')
    lines += [
        "    };",
        "    return table;",
        "}",
        "",
        "} // namespace",
        "",
        "i32 vanillaItemEnchantable(std::string_view itemName)",
        "{",
        "    const auto& table = enchantableTable();",
        "    const auto found = table.find(itemName);",
        "    return found == table.end() ? 0 : found->second;",
        "}",
        "",
        "} // namespace mc",
        "",
    ]
    OUTPUT.write_text(chr(10).join(lines), encoding="utf-8")
    print(f"wrote {OUTPUT.relative_to(REPO_ROOT)} ({len(entries)} enchantable items)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
