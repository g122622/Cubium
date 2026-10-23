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
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.Rarity;

/**
 * 物品默认组件（耐久、稀有度、附魔能力）的原版 oracle。
 *
 * <p>与 probes/item_components.cpp 同构：
 *
 * <pre>
 *   { "item": "minecraft:diamond_sword", "maxDamage": 1561, "rarity": "common", "enchantable": 10 }
 * </pre>
 *
 * 这三项是服务端很多判定的输入：耐久决定损坏，稀有度决定物品名颜色与战利品品质，
 * 附魔能力决定附魔台给的等级。它们都是纯数据，最容易被手抄错。
 */
public final class ItemComponentsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ItemComponentsOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            writeList(Path.of(args[1]));
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: ItemComponentsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    /** 单元写文件而不是 stdout：原版 log4j 的配置会把 stdout 拿去记日志。 */
    private static void writeList(Path output) throws IOException {
        List<String> names = new ArrayList<>();
        for (Identifier id : BuiltInRegistries.ITEM.keySet()) {
            names.add(id.toString());
        }
        names.sort(Comparator.naturalOrder());
        StringBuilder text = new StringBuilder();
        for (String name : names) {
            text.append(name).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static void run(Path input, Path output) throws IOException {
        try (BufferedReader reader = Files.newBufferedReader(input, StandardCharsets.UTF_8);
                BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.isBlank()) {
                    continue;
                }
                JsonObject request = JsonParser.parseString(line).getAsJsonObject();
                writer.write(GSON.toJson(summarise(request.get("item").getAsString())));
                writer.write('\n');
            }
        }
    }

    private static JsonObject summarise(String itemId) {
        JsonObject result = new JsonObject();
        result.addProperty("item", itemId);

        Item item = BuiltInRegistries.ITEM.getOptional(Identifier.parse(itemId)).orElse(null);
        if (item == null) {
            result.addProperty("missing", true);
            return result;
        }

        // 没有 MAX_DAMAGE 组件的物品不可损坏，记 0。
        Integer maxDamage = item.components().get(DataComponents.MAX_DAMAGE);
        result.addProperty("maxDamage", maxDamage == null ? 0 : maxDamage);

        Rarity rarity = item.components().getOrDefault(DataComponents.RARITY, Rarity.COMMON);
        result.addProperty("rarity", rarity.name().toLowerCase());

        // ENCHANTABLE 是 Enchantable 记录，其中的 value() 是附魔能力；
        // 没有该组件的物品不可附魔，记 0。
        net.minecraft.world.item.enchantment.Enchantable enchantable =
                item.components().get(DataComponents.ENCHANTABLE);
        result.addProperty("enchantable", enchantable == null ? 0 : enchantable.value());

        return result;
    }
}