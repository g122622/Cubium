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
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;

/**
 * 物品能否堆叠的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemStackableOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@code ItemStack#getMaxStackSize} / {@code isDamageableItem} /
 * {@code isDamaged} / {@code isStackable}，不重写判定。
 *
 * <p>单元表必须与 probes/item_stackable.cpp 逐项一致。
 */
public final class ItemStackableOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表：{@code 物品 id|损伤值}，与 C++ 侧 UNITS 逐项一致。 */
    private static final List<String> UNITS =
            List.of(
                    "minecraft:dirt|0",
                    "minecraft:stick|0",
                    "minecraft:apple|0",
                    "minecraft:dirt|5",
                    "minecraft:snowball|0",
                    "minecraft:ender_pearl|0",
                    "minecraft:nether_star|0",
                    "minecraft:shield|0",
                    "minecraft:saddle|0",
                    "minecraft:potion|0",
                    "minecraft:bucket|0",
                    "minecraft:diamond_sword|0",
                    "minecraft:diamond_pickaxe|0",
                    "minecraft:diamond_helmet|0",
                    "minecraft:elytra|0",
                    "minecraft:bow|0",
                    "minecraft:diamond_sword|1",
                    "minecraft:diamond_sword|500",
                    "minecraft:diamond_pickaxe|1",
                    "minecraft:diamond_helmet|1",
                    "minecraft:elytra|1",
                    "minecraft:diamond_sword|1560",
                    "minecraft:air|0");

    private ItemStackableOracle() {}

    public static void main(String[] args) throws IOException {
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
                "usage: ItemStackableOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        // 借 DataPackRegistries.load() 完成 Bootstrap 与游戏版本设置——
        // 单独调 Bootstrap.bootStrap() 会因「Game version not set」失败。
        DataPackRegistries.load();

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
        String[] parts = unit.split("\\|");
        if (parts.length != 2) {
            throw new IllegalArgumentException("item_stackable: 单元应为 2 段: " + unit);
        }

        String itemId = parts[0];
        int damage = Integer.parseInt(parts[1]);

        Item item = BuiltInRegistries.ITEM.getValue(Identifier.parse(itemId));
        ItemStack stack = new ItemStack(item);
        if (damage > 0) {
            stack.setDamageValue(damage);
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("item", itemId);
        result.addProperty("damage", damage);
        result.addProperty("empty", stack.isEmpty());
        result.addProperty("maxStackSize", stack.getMaxStackSize());
        result.addProperty("damageable", stack.isDamageableItem());
        result.addProperty("damaged", stack.isDamaged());
        result.addProperty("stackable", stack.isStackable());
        return result;
    }
}
