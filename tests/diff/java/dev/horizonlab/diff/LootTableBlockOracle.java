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

import com.google.gson.JsonObject;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.storage.loot.LootParams;
import net.minecraft.world.level.storage.loot.parameters.LootContextParams;

/**
 * 方块类战利品表在「挖掘」上下文下的抽取，原版参照实现。
 *
 * <pre>
 *   java dev.horizonlab.diff.LootTableBlockOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/loot_table_block.cpp 同构：`block` 给出方块名，`tool` 给出工具，
 * `fortune` / `silkTouch` 给出附魔等级。原版的「时运/精准采集」不通过独立上下文键
 * 传递，而是由 {@code match_tool} 条件直接读工具上的附魔；因此这里把对应附魔真实地
 * 写到工具物品上——这正是「工具带附魔」在真实游戏里的形态。
 */
public final class LootTableBlockOracle {
    private LootTableBlockOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var data = LootServerData.get();
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
        System.err.println("usage: LootTableBlockOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var level = LootLevel.get();
        LootRoll.run(input, output, "table", (table, request, result) -> {
            Block block = BuiltInRegistries.BLOCK
                .getOptional(Identifier.parse(request.get("block").getAsString()))
                .orElse(null);
            if (block == null) {
                result.addProperty("missing", true);
                return;
            }

            ItemStack tool = tool(request);
            result.add("samples", LootRoll.samples(table, random -> new LootParams.Builder(level.level)
                .withParameter(LootContextParams.BLOCK_STATE, block.defaultBlockState())
                .withParameter(LootContextParams.ORIGIN, LootTableOracle.origin(request))
                .withParameter(LootContextParams.TOOL, tool)
                .create(table.getParamSet())));
        });
    }

    /**
     * 按输入构造工具：附魔直接写到物品上，与原版「手持附魔镐」完全一致。
     *
     * <p>不传工具时给一个空栈——原版 `match_tool` 会因「没有工具」而失败，
     * 与 C++ 探针「不设置 tool 参数」的语义一致。
     */
    private static ItemStack tool(JsonObject request) {
        if (!request.has("tool")) {
            return ItemStack.EMPTY;
        }
        var toolId = Identifier.parse(request.get("tool").getAsString());
        var item = BuiltInRegistries.ITEM.getOptional(toolId).orElseThrow(
            () -> new IllegalStateException("unknown tool item: " + toolId));
        ItemStack stack = new ItemStack(item);
        int fortune = request.has("fortune") ? request.get("fortune").getAsInt() : 0;
        if (fortune > 0) {
            stack.enchant(levelOf("minecraft:fortune"), fortune);
        }
        if (request.has("silkTouch") && request.get("silkTouch").getAsBoolean()) {
            stack.enchant(levelOf("minecraft:silk_touch"), 1);
        }
        return stack;
    }

    /** 附魔引用：原版 1.21 起附魔在注册表里，工具附魔需要对应的 Holder。 */
    private static net.minecraft.core.Holder<net.minecraft.world.item.enchantment.Enchantment> levelOf(String id) {
        var lookup = LootServerData.get().registryAccess()
            .lookupOrThrow(net.minecraft.core.registries.Registries.ENCHANTMENT);
        return lookup.getOrThrow(net.minecraft.resources.ResourceKey.create(
            net.minecraft.core.registries.Registries.ENCHANTMENT, Identifier.parse(id)));
    }
}