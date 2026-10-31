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
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.item.enchantment.Enchantment;

/**
 * 附魔等级与代价曲线的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EnchantmentCostOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/enchantment_cost.cpp 同构：比较最大等级、铁砧代价，以及每一级的
 * 附魔台等级代价区间。附魔是数据包注册表，必须走 {@link DataPackRegistries}。
 */
public final class EnchantmentCostOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EnchantmentCostOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            var enchantments = registries.lookup.lookupOrThrow(Registries.ENCHANTMENT);
            List<String> names = new ArrayList<>();
            enchantments.listElements().forEach(holder -> {
                names.add(holder.key().identifier().toString());
            });
            Collections.sort(names);
            StringBuilder text = new StringBuilder();
            for (String name : names) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EnchantmentCostOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            String name = JsonParser.parseString(line).getAsJsonObject().get("enchantment").getAsString();
            text.append(GSON.toJson(summarise(enchantments, name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(
            net.minecraft.core.HolderLookup.RegistryLookup<Enchantment> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("enchantment", name);

        ResourceKey<Enchantment> key = ResourceKey.create(Registries.ENCHANTMENT, Identifier.parse(name));
        var holder = registry.get(key).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }

        Enchantment enchantment = holder.value();
        int maxLevel = enchantment.getMaxLevel();
        result.addProperty("maxLevel", maxLevel);
        result.addProperty("anvilCost", enchantment.getAnvilCost());

        JsonArray minCost = new JsonArray();
        JsonArray maxCost = new JsonArray();
        // 原版等级从 1 开始，按 1..maxLevel 逐级求值。
        for (int level = 1; level <= maxLevel; level++) {
            minCost.add(enchantment.getMinCost(level));
            maxCost.add(enchantment.getMaxCost(level));
        }
        result.add("minCost", minCost);
        result.add("maxCost", maxCost);
        return result;
    }
}