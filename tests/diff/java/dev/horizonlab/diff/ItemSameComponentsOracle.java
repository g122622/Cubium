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
 * 「同物品且同组件」比较的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemSameComponentsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@code ItemStack.isSameItemSameComponents} 与 {@code ItemStack.matches}。
 *
 * <p>单元表必须与 probes/item_same_components.cpp 逐项一致。
 */
public final class ItemSameComponentsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表：{@code 左物品|左损伤|左数量|右物品|右损伤|右数量}。 */
    private static final List<String> UNITS =
            List.of(
                    "minecraft:dirt|0|1|minecraft:dirt|0|1",
                    "minecraft:dirt|0|1|minecraft:dirt|0|5",
                    "minecraft:dirt|0|64|minecraft:dirt|0|1",
                    "minecraft:dirt|0|1|minecraft:stone|0|1",
                    "minecraft:diamond_sword|0|1|minecraft:diamond_sword|1|1",
                    "minecraft:diamond_sword|100|1|minecraft:diamond_sword|200|1",
                    "minecraft:diamond_sword|100|1|minecraft:diamond_sword|100|1",
                    "minecraft:air|0|0|minecraft:air|0|0",
                    "minecraft:air|0|0|minecraft:dirt|0|1",
                    "minecraft:diamond_sword|0|1|minecraft:dirt|0|1",
                    "minecraft:bow|50|1|minecraft:bow|50|2",
                    "minecraft:bow|0|1|minecraft:bow|1|1");

    private ItemSameComponentsOracle() {}

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
                "usage: ItemSameComponentsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        // 借 DataPackRegistries.load() 完成 Bootstrap 与游戏版本设置。
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
        if (parts.length != 6) {
            throw new IllegalArgumentException("item_same_components: 单元应为 6 段: " + unit);
        }

        ItemStack left = build(parts[0], Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
        ItemStack right = build(parts[3], Integer.parseInt(parts[4]), Integer.parseInt(parts[5]));

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("leftEmpty", left.isEmpty());
        result.addProperty("rightEmpty", right.isEmpty());
        result.addProperty("leftDamage", left.getDamageValue());
        result.addProperty("rightDamage", right.getDamageValue());
        result.addProperty("leftCount", left.getCount());
        result.addProperty("rightCount", right.getCount());
        result.addProperty("sameItemSameComponents", ItemStack.isSameItemSameComponents(left, right));
        result.addProperty("matches", ItemStack.matches(left, right));
        return result;
    }

    private static ItemStack build(String itemId, int damage, int count) {
        if (count == 0) {
            return ItemStack.EMPTY;
        }
        Item item = BuiltInRegistries.ITEM.getValue(Identifier.parse(itemId));
        ItemStack stack = new ItemStack(item, count);
        if (damage > 0) {
            stack.setDamageValue(damage);
        }
        return stack;
    }
}