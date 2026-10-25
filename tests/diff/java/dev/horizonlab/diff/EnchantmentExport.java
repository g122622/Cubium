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

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.core.registries.Registries;
import net.minecraft.world.item.enchantment.Enchantment;

/**
 * 导出附魔表（等级上限、每级最小附魔法力）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EnchantmentExport list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>附魔在 1.21 起是数据包注册表，所以这里走 {@link DataPackRegistries}，而不是
 * {@link net.minecraft.server.Bootstrap} 之后的静态注册表。输出形状与
 * probes/enchantment_table.cpp 同构：
 *
 * <pre>
 *   { "enchantment": "minecraft:sharpness", "minLevel": 1, "maxLevel": 5,
 *     "costs": [ { "level": 1, "minCost": 1, "maxCost": 21 },
 *                { "level": 5, "minCost": 17, "maxCost": 37 } ] }
 * </pre>
 *
 * <p>costs 只给 minLevel 与 maxLevel 两端：vanilla 的代价公式按等级线性增长，端点一致
 * 即全等级一致；附魔台每格能给的附魔等级上限由它决定，这里逐条锁定，避免手抄。
 */
public final class EnchantmentExport {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EnchantmentExport() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            var enchantments = registries.lookup.lookupOrThrow(Registries.ENCHANTMENT);
            // 写文件而不是 stdout/stderr：原版 log4j 的配置会把 stdout 拿去记日志，
            // System.err 又被它套成 "[time] [main/INFO]: [STDERR]:" 行，两者都不能当协议通道。
            StringBuilder names = new StringBuilder();
            enchantments.listElements()
                    .map(holder -> holder.key().identifier().toString())
                    .sorted()
                    .forEach(name -> names.append(name).append('\n'));
            Files.writeString(Path.of(args[1]), names, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EnchantmentExport list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var enchantments = registries.lookup.lookupOrThrow(Registries.ENCHANTMENT);

        JsonArray results = new JsonArray();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = com.google.gson.JsonParser.parseString(line).getAsJsonObject().get("enchantment").getAsString();
            results.add(summarise(enchantments, name));
        }

        JsonArray sorted = new JsonArray();
        results.asList().stream()
                .map(com.google.gson.JsonElement::getAsJsonObject)
                .sorted((left, right) ->
                        left.get("enchantment").getAsString().compareTo(right.get("enchantment").getAsString()))
                .forEach(sorted::add);
        StringBuilder text = new StringBuilder();
        for (var element : sorted) {
            text.append(GSON.toJson(element)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(net.minecraft.core.HolderLookup.RegistryLookup<Enchantment> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("enchantment", name);
        var key = net.minecraft.resources.ResourceKey.create(Registries.ENCHANTMENT, net.minecraft.resources.Identifier.parse(name));
        var holder = registry.get(key).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }
        Enchantment enchantment = holder.value();
        int minLevel = enchantment.getMinLevel();
        int maxLevel = enchantment.getMaxLevel();
        result.addProperty("minLevel", minLevel);
        result.addProperty("maxLevel", maxLevel);
        // 只给两端：附魔代价公式（base + perLevel × (level − 1)）是线性的，端点一致
        // 即全等级一致；逐级导出只会让报告变大，不会多证明什么。
        JsonArray costs = new JsonArray();
        for (int level : new int[] {minLevel, maxLevel}) {
            JsonObject cost = new JsonObject();
            cost.addProperty("level", level);
            cost.addProperty("minCost", enchantment.getMinCost(level));
            cost.addProperty("maxCost", enchantment.getMaxCost(level));
            costs.add(cost);
        }
        result.add("costs", costs);
        // 权重 / 铁砧代价：附魔台抽取概率与铁砧合并代价直接依赖这两个字段。
        // 槽位组（EquipmentSlotGroup）在 Cubium 尚未建模，先不导出，避免产生无实现方的假差异。
        result.addProperty("weight", enchantment.getWeight());
        result.addProperty("anvilCost", enchantment.getAnvilCost());
        return result;
    }
}