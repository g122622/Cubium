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
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.core.registries.Registries;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.enchantment.EnchantmentHelper;

/**
 * 附魔台可选附魔列表的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EnchantmentSelectionOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@code EnchantmentHelper.getAvailableEnchantmentResults}，不重写筛选逻辑。
 *
 * <p>物品表与等级表必须与 probes/enchantment_selection.cpp 逐项一致。
 */
public final class EnchantmentSelectionOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 覆盖用的物品表，与 C++ 侧 ITEMS 逐项一致。 */
    private static final List<String> ITEMS =
            List.of(
                    "minecraft:book",
                    "minecraft:diamond_sword",
                    "minecraft:diamond_pickaxe",
                    "minecraft:diamond_axe",
                    "minecraft:diamond_helmet",
                    "minecraft:diamond_boots",
                    "minecraft:bow",
                    "minecraft:fishing_rod",
                    "minecraft:trident",
                    "minecraft:elytra",
                    "minecraft:shield",
                    "minecraft:air",
                    "minecraft:dirt",
                    "minecraft:stick");

    /** 覆盖附魔等级代价曲线的关键取值，与 C++ 侧 LEVELS 逐项一致。 */
    private static final int[] LEVELS = {0, 1, 5, 10, 15, 20, 25, 30, 35, 40, 50, 100};

    private EnchantmentSelectionOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String item : ITEMS) {
                for (int level : LEVELS) {
                    text.append(item).append('|').append(level).append('\n');
                }
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: EnchantmentSelectionOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var enchantments = registries.lookup.lookupOrThrow(Registries.ENCHANTMENT);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            var obj = JsonParser.parseString(line).getAsJsonObject();
            String itemId = obj.get("item").getAsString();
            int level = obj.get("level").getAsInt();
            text.append(GSON.toJson(summarise(enchantments, itemId, level))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(
            net.minecraft.core.HolderLookup.RegistryLookup<net.minecraft.world.item.enchantment.Enchantment>
                    enchantments,
            String itemId,
            int level) {
        Item item = BuiltInRegistries.ITEM.getValue(Identifier.parse(itemId));
        ItemStack stack = new ItemStack(item);

        // 排序后再输出：原版返回顺序由附魔注册表决定，Cubium 的遍历顺序不同。
        // 顺序本身不是本能力要验证的口径（它属于注册表加载顺序，另有能力覆盖），
        // 这里统一按附魔 id 排序，让差异集中在「哪些附魔、各几级」上。
        var results = EnchantmentHelper.getAvailableEnchantmentResults(
                        level, stack, enchantments.listElements().map(
                                h -> (net.minecraft.core.Holder<net.minecraft.world.item.enchantment.Enchantment>) h))
                .stream()
                .sorted(java.util.Comparator.comparing(i -> enchantmentIdOf(i.enchantment())))
                .toList();

        JsonArray array = new JsonArray();
        for (var instance : results) {
            JsonObject entry = new JsonObject();
            entry.addProperty("enchantment", enchantmentIdOf(instance.enchantment()));
            entry.addProperty("level", instance.level());
            array.add(entry);
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", itemId + "|" + level);
        result.addProperty("item", itemId);
        result.addProperty("level", level);
        result.addProperty("count", array.size());
        result.add("results", array);
        return result;
    }

    /**
     * 取出附魔的注册表 id。
     *
     * {@code Holder} 在不同映射下暴露的访问器形态不完全一致，这里统一走
     * {@code unwrapKey()} 再取 {@code ResourceKey}，避免依赖具体 API 形态。
     */
    private static String enchantmentIdOf(
            net.minecraft.core.Holder<net.minecraft.world.item.enchantment.Enchantment> holder) {
        return holder.unwrapKey().map(k -> k.identifier().toString()).orElse("");
    }
}
