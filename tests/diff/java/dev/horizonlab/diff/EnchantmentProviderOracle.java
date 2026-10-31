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
import net.minecraft.world.item.enchantment.providers.EnchantmentsByCost;
import net.minecraft.world.item.enchantment.providers.EnchantmentsByCostWithDifficulty;
import net.minecraft.world.item.enchantment.providers.SingleEnchantment;

/**
 * 附魔提供器的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EnchantmentProviderOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/enchantment_provider.cpp 同构：比较提供器的 {@code type}、附魔集合引用，
 * 以及 {@code by_cost_with_difficulty} 的代价区间。
 *
 * <p>代价上界 {@code minCost + (int)(specialMultiplier * maxCostSpan)} 里的 {@code (int)}
 * 是截断而非四舍五入，这里用 Java 的强制转换原样复刻，确保与 C++ 侧的
 * {@code static_cast<i32>} 语义一致。
 */
public final class EnchantmentProviderOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EnchantmentProviderOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            List<String> names = new ArrayList<>();
            registries
                    .lookup
                    .lookupOrThrow(Registries.ENCHANTMENT_PROVIDER)
                    .listElements()
                    .forEach(holder -> names.add(holder.key().identifier().toString()));
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
        System.err.println(
                "usage: EnchantmentProviderOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var providers = registries.lookup.lookupOrThrow(Registries.ENCHANTMENT_PROVIDER);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            var obj = JsonParser.parseString(line).getAsJsonObject();
            String name = obj.get("provider").getAsString();
            // 输入按字符串传递，再解析成 f32，保证两侧得到同一个位模式。
            float specialMultiplier = Float.parseFloat(obj.get("specialMultiplier").getAsString());
            text.append(GSON.toJson(summarise(providers, name, specialMultiplier))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(
            net.minecraft.core.HolderLookup.RegistryLookup<net.minecraft.world.item.enchantment.providers.EnchantmentProvider>
                    registry,
            String name,
            float specialMultiplier) {
        JsonObject result = new JsonObject();
        result.addProperty("provider", name);

        var holder =
                registry.get(
                                net.minecraft.resources.ResourceKey.create(
                                        Registries.ENCHANTMENT_PROVIDER, Identifier.parse(name)))
                        .orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }

        var provider = holder.value();
        if (provider instanceof SingleEnchantment single) {
            result.addProperty("type", "minecraft:single");
            result.addProperty("enchantment", single.enchantment().unwrapKey().orElseThrow().identifier().toString());
            result.addProperty("level", intProviderToString(single.level()));
        } else if (provider instanceof EnchantmentsByCost byCost) {
            result.addProperty("type", "minecraft:by_cost");
            result.addProperty("enchantments", holderSetToString(byCost.enchantments()));
            result.addProperty("cost", intProviderToString(byCost.cost()));
        } else if (provider instanceof EnchantmentsByCostWithDifficulty byDifficulty) {
            result.addProperty("type", "minecraft:by_cost_with_difficulty");
            result.addProperty("enchantments", holderSetToString(byDifficulty.enchantments()));
            result.addProperty("minCost", byDifficulty.minCost());
            result.addProperty("maxCostSpan", byDifficulty.maxCostSpan());
            result.addProperty("costMin", byDifficulty.minCost());
            // 原版：Mth.randomBetweenInclusive(rng, minCost, minCost + (int)(f * maxCostSpan))
            result.addProperty(
                    "costMax",
                    byDifficulty.minCost() + (int) (specialMultiplier * (float) byDifficulty.maxCostSpan()));
        } else {
            result.addProperty("unknownType", true);
        }

        result.addProperty("specialMultiplier", formatFloat(specialMultiplier));
        return result;
    }

    /**
     * 把 HolderSet 转成与 C++ 侧一致的字符串：标签原样、单项原样、数组排序后逗号连接。
     */
    private static String holderSetToString(net.minecraft.core.HolderSet<?> holderSet) {
        var tagKey = holderSet.unwrap().left();
        if (tagKey.isPresent()) {
            return "#" + tagKey.get().location();
        }
        List<String> parts = new ArrayList<>();
        for (var holder : holderSet) {
            holder.unwrapKey()
                    .ifPresent(key -> parts.add(key.identifier().toString()));
        }
        Collections.sort(parts);
        return String.join(",", parts);
    }

    /**
     * 把 IntProvider 转成与 C++ 侧一致的字符串：常量直接输出整数，其他形态输出类名。
     *
     * 数据包 7 项用的都是整数字面量（ConstantInt）；非字面量时 C++ 侧原样 dump JSON，
     * 这里退化为类名——两侧口径一致地「不做解释」，不会假装比对成功。
     */
    private static String intProviderToString(net.minecraft.util.valueproviders.IntProvider provider) {
        if (provider instanceof net.minecraft.util.valueproviders.ConstantInt constant) {
            return Integer.toString(constant.getValue());
        }
        return provider.getClass().getSimpleName();
    }

    private static String formatFloat(float value) {
        String text = String.format("%.6g", (double) value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }
}
