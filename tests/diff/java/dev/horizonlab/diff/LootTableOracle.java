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

package dev.horizonlab.diff;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.storage.loot.LootParams;
import net.minecraft.world.level.storage.loot.parameters.LootContextParams;
import net.minecraft.world.phys.Vec3;

/**
 * 战利品表「无参数上下文」抽取的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.LootTableOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/loot_table.cpp 同构：只提供 {@code origin}，不给实体、方块或工具。
 * 输出 {@code { "table": ..., "samples": [[{id,count}...] × 4] }}。
 */
public final class LootTableOracle {
    private LootTableOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            LootServerData data = LootServerData.get();
            StringBuilder text = new StringBuilder();
            for (String id : data.tableIds()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: LootTableOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var level = LootLevel.get();
        LootRoll.run(input, output, "table", (table, request, result) -> {
            Vec3 origin = origin(request);
            // 与 C++ 探针同样的「最小通用上下文」：中性方块状态 + 空工具。
            // 原版 1.21.11 的 block 参数集把这三项都列为必需键，不给就掷不了任何方块表。
            result.add("samples", LootRoll.samples(table, random -> new LootParams.Builder(level.level)
                .withParameter(LootContextParams.ORIGIN, origin)
                .withParameter(LootContextParams.BLOCK_STATE, Blocks.AIR.defaultBlockState())
                .withParameter(LootContextParams.TOOL, ItemStack.EMPTY)
                .create(table.getParamSet())));
        });
    }

    /** 解析 `origin`：[x, y, z]；缺省 (0.5, 64, 0.5)，与 C++ 探针一致。 */
    static Vec3 origin(com.google.gson.JsonObject request) {
        if (!request.has("origin")) {
            return new Vec3(0.5, 64.0, 0.5);
        }
        var array = request.getAsJsonArray("origin");
        return new Vec3(array.get(0).getAsDouble(), array.get(1).getAsDouble(), array.get(2).getAsDouble());
    }
}