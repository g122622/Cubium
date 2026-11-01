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
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import net.minecraft.core.Holder;
import net.minecraft.core.registries.Registries;
import net.minecraft.world.item.enchantment.Enchantment;
import net.minecraft.world.item.enchantment.ItemEnchantments;

/**
 * {@code ItemEnchantments.Mutable} 增删改语义的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EnchantmentStorageOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ItemEnchantments.Mutable#set} / {@link ItemEnchantments.Mutable#upgrade}
 * / {@link ItemEnchantments.Mutable#getLevel}，不重写公式。
 *
 * <p>单元里的 `a`~`e` 映射到注册表中按标识符排序的前 5 个附魔。选择顺序在探针与 oracle
 * 两侧都不需要一致——结果是 `字母=等级` 的摘要，只要映射是固定的一一对应即可。
 *
 * <p>单元表必须与 probes/enchantment_storage.cpp 逐项一致。
 */
public final class EnchantmentStorageOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "set:a:3",
                    "set:a:3,set:a:5",
                    "set:a:3,set:a:0",
                    "set:a:3,set:a:-1",
                    "set:a:255",
                    "set:a:256",
                    "set:a:1000",
                    "upgrade:a:3",
                    "upgrade:a:3,upgrade:a:5",
                    "upgrade:a:5,upgrade:a:3",
                    "upgrade:a:3,upgrade:a:0",
                    "upgrade:a:3,upgrade:a:-1",
                    "upgrade:a:0",
                    "upgrade:a:300",
                    "upgrade:a:300,upgrade:a:400",
                    "set:a:3,set:b:1,set:c:2",
                    "set:a:3,upgrade:a:2",
                    "set:a:3,upgrade:a:5",
                    "upgrade:a:5,set:a:2",
                    "set:a:3,set:a:0,upgrade:a:1",
                    "set:a:1,set:b:2,set:c:3,set:b:0");

    /** 字母到附魔 Holder 的映射。 */
    private static Map<Character, Holder<Enchantment>> letterMap;

    private EnchantmentStorageOracle() {}

    public static void main(String[] args) throws IOException {
        var registries = DataPackRegistries.load();
        var lookup = registries.lookup.lookupOrThrow(Registries.ENCHANTMENT);

        // 按标识符排序后取前 5 个，构成 a~e 的固定映射。
        List<Holder.Reference<Enchantment>> pool = new ArrayList<>();
        lookup.listElements().forEach(pool::add);
        pool.sort((left, right) -> left.key().identifier().toString()
                .compareTo(right.key().identifier().toString()));

        letterMap = new TreeMap<>();
        for (int i = 0; i < 5 && i < pool.size(); i++) {
            letterMap.put((char) ('a' + i), pool.get(i));
        }

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: EnchantmentStorageOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject summarise(String unit) {
        ItemEnchantments.Mutable mutable = new ItemEnchantments.Mutable(ItemEnchantments.EMPTY);

        for (String opText : unit.split(",", -1)) {
            String[] parts = opText.split(":", -1);
            if (parts.length != 3 || parts[1].length() != 1) {
                throw new IllegalArgumentException("enchantment_storage: 非法操作 '" + opText + "'");
            }
            char letter = parts[1].charAt(0);
            Holder<Enchantment> ench = letterMap.get(letter);
            if (ench == null) {
                throw new IllegalArgumentException("enchantment_storage: 未映射的附魔 " + letter);
            }
            int level = Integer.parseInt(parts[2]);

            switch (parts[0]) {
                case "set" -> mutable.set(ench, level);
                case "upgrade" -> mutable.upgrade(ench, level);
                default -> throw new IllegalArgumentException(
                        "enchantment_storage: 未知操作 '" + parts[0] + "'");
            }
        }

        // 用 a~e 的顺序重建摘要。
        StringBuilder summary = new StringBuilder();
        int size = 0;
        for (Map.Entry<Character, Holder<Enchantment>> entry : letterMap.entrySet()) {
            int level = mutable.getLevel(entry.getValue());
            if (level > 0) {
                if (summary.length() > 0) {
                    summary.append(',');
                }
                summary.append(entry.getKey()).append('=').append(level);
                size++;
            }
        }
        if (summary.length() == 0) {
            summary.append('-');
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("result", summary.toString());
        result.addProperty("size", size);
        return result;
    }
}