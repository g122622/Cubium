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
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.core.Holder;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.item.enchantment.Enchantment;

/**
 * 附魔互斥性的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EnchantmentCompatOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>被验证的原版逻辑</h2>
 *
 * <pre>{@code
 * public static boolean areCompatible(Holder<Enchantment> a, Holder<Enchantment> b) {
 *     return !a.equals(b)
 *         && !a.value().exclusiveSet().contains(b)
 *         && !b.value().exclusiveSet().contains(a);
 * }
 * }</pre>
 *
 * <h2>为什么用注册表里的真实附魔</h2>
 *
 * <p>{@code HolderSet.contains} 按 **Holder 的身份**比较。想用合成实例验证，就得保证同一附魔名
 * 在所有 `exclusiveSet` 里是同一个 `Holder` 对象，而 `Enchantment` 构造需要 `exclusiveSet` 又
 * 形成循环依赖，`Holder.Reference.bindValue` 还是 `protected`。
 *
 * <p>注册表里的附魔天然满足「一个 id 一个唯一 Holder」，互斥关系也直接来自数据包，因此这里
 * 只用真实附魔求值——既避开构造难题，验证的又是真实数据。
 *
 * <p>单元里的 `aExcl` / `bExcl` 仅用于**与探针侧对齐输入**；oracle 自身不读它们，而是让原版
 * 从注册表里取。两侧若对数据包的理解不一致，结果就会不同。
 *
 * <p>单元表由 harness 侧的 `listEnchantmentCompatUnits()` 计算得出，`list` 子命令不适用。
 */
public final class EnchantmentCompatOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 注册表里附魔 id -> Holder。 */
    private static Map<String, Holder<Enchantment>> registryHolders;

    private EnchantmentCompatOracle() {}

    public static void main(String[] args) throws IOException {
        DataPackRegistries registries = DataPackRegistries.load();
        registryHolders = new LinkedHashMap<>();
        var lookup = registries.lookup.lookupOrThrow(Registries.ENCHANTMENT);
        lookup.listElements().forEach(holder -> {
            registryHolders.put(holder.key().identifier().toString(), holder);
        });

        if (args.length == 2 && args[0].equals("list")) {
            // 单元由 harness 计算，这里输出空列表即可（框架会改用 computed units）。
            Files.writeString(Path.of(args[1]), "", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: EnchantmentCompatOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /**
     * 求 `areCompatible(a, b)`。
     *
     * <p>`a` / `b` 必须是注册表里存在的附魔 id。第二个参数为 `null` 时表示「不存在」，
     * 此时结果记为 `absent`，由框架判定两侧是否一致地认为该附魔不存在。
     */
    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 4) {
            throw new IllegalArgumentException(
                    "enchantment_compat: 单元应为 4 段: <a>|<b>|<aExcl>|<bExcl>: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        Holder<Enchantment> a = registryHolders.get(parts[0]);
        Holder<Enchantment> b = registryHolders.get(parts[1]);
        if (a == null || b == null) {
            // 附魔不在注册表里：两侧应当一致地报「缺失」。
            out.addProperty("value", "absent");
            return out;
        }

        boolean result = Enchantment.areCompatible(a, b);
        out.addProperty("value", result ? "true" : "false");
        return out;
    }
}