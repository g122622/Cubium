#!/usr/bin/env python3
"""从原版稀有度导出结果生成 C++ 数据表。

输入是 ``tests/diff/java`` 下 ``ItemRarityExport`` 的输出（只含非 common 项）：

    java dev.horizonlab.diff.ItemRarityExport item_rarity.json

本脚本转成 ``src/common/item/VanillaItemRarity.cpp``，由 ``Items::initialize()``
在注册完成后统一套用。

为什么不用注册点逐个写：稀有度是纯数据，散落在上百处注册调用里既难核对也易漏写；
差分测试一次就能全量比出来，因此让原版数据做唯一来源。common 是默认值、不导出，
所以表里没有的物品保持注册时写的值（应为 common）。
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
OUTPUT = REPO_ROOT / "src/common/item/VanillaItemRarity.cpp"

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

// 本文件由 scripts/baking/gen_item_rarity.py 生成，请勿手改。
// 数据来源：原版 1.21.11 服务端 jar 的 DataComponents.RARITY（见
// tests/diff/java/dev/horizonlab/diff/ItemRarityExport.java）。只含非 common 项。
"""

RARITY_ENUM = {
    "uncommon": "ItemRarity::Uncommon",
    "rare": "ItemRarity::Rare",
    "epic": "ItemRarity::Epic",
}


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2

    entries = json.loads(Path(argv[1]).read_text(encoding="utf-8"))
    lines = [
        HEADER,
        '#include "VanillaItemRarity.hpp"',
        "",
        "#include <string_view>",
        "#include <unordered_map>",
        "",
        "namespace mc {",
        "",
        "namespace {",
        "",
        "/// 物品名 → 原版稀有度（只含非 common 项）。",
        "const std::unordered_map<std::string_view, ItemRarity>& rarityTable()",
        "{",
        "    static const std::unordered_map<std::string_view, ItemRarity> table = {",
    ]
    for entry in entries:
        rarity = RARITY_ENUM.get(entry["rarity"])
        if rarity is None:
            raise ValueError(f"未知稀有度 {entry['rarity']!r}（物品 {entry['item']}）")
        lines.append(f'        {{"{entry["item"]}", {rarity}}},')
    lines += [
        "    };",
        "    return table;",
        "}",
        "",
        "} // namespace",
        "",
        "std::optional<ItemRarity> vanillaItemRarity(std::string_view itemName)",
        "{",
        "    const auto& table = rarityTable();",
        "    const auto found = table.find(itemName);",
        "    if (found == table.end()) {",
        "        return std::nullopt;",
        "    }",
        "    return found->second;",
        "}",
        "",
        "} // namespace mc",
        "",
    ]
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {OUTPUT.relative_to(REPO_ROOT)} ({len(entries)} non-common items)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))