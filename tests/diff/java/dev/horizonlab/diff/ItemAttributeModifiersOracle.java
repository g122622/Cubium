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
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.Holder;
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.entity.ai.attributes.Attribute;
import net.minecraft.world.entity.ai.attributes.AttributeModifier;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.component.ItemAttributeModifiers;

/**
 * 物品属性修饰符的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemAttributeModifiersOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.ItemAttributeModifiersOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/item_attribute_modifiers.cpp 同构：对每个物品、每个装备槽位取
 * {@code ATTRIBUTE_MODIFIERS} 组件里作用于该槽位的条目，列出属性 ID、修饰符 ID、
 * 运算与数值。
 *
 * <p>只输出非空槽位——原版这是「槽位 → 条目列表」的映射，空槽位在场与否没有语义。
 */
public final class ItemAttributeModifiersOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与原版 {@code EquipmentSlot} 的值与序列化键一致。 */
    private static final EquipmentSlot[] SLOTS = {
        EquipmentSlot.MAINHAND,
        EquipmentSlot.OFFHAND,
        EquipmentSlot.FEET,
        EquipmentSlot.LEGS,
        EquipmentSlot.CHEST,
        EquipmentSlot.HEAD,
        EquipmentSlot.BODY,
    };

    private ItemAttributeModifiersOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.ITEM.keySet()) {
                names.add(id.toString());
            }
            names.sort(Comparator.naturalOrder());
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
                "usage: ItemAttributeModifiersOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("item").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String itemId) {
        JsonObject result = new JsonObject();
        result.addProperty("item", itemId);

        Item item = BuiltInRegistries.ITEM.getOptional(Identifier.parse(itemId)).orElse(null);
        if (item == null) {
            result.addProperty("missing", true);
            return result;
        }

        JsonObject slots = new JsonObject();
        for (EquipmentSlot slot : SLOTS) {
            ItemAttributeModifiers modifiers = item.components().get(DataComponents.ATTRIBUTE_MODIFIERS);
            JsonArray entries = new JsonArray();
            if (modifiers == null) {
                continue;
            }
            for (ItemAttributeModifiers.Entry entry : modifiers.modifiers()) {
                // 1.21.11 的条目按 EquipmentSlotGroup 生效（"mainhand"、"armor"、"body"……），
                // 不是单个槽位；这里按槽位展开成 item × slot 的二维表，与 Cubium 的
                // Item::getAttributeModifiers(slot) 同构。
                if (!entry.slot().test(slot)) {
                    continue;
                }
                Holder<Attribute> attribute = entry.attribute();
                AttributeModifier modifier = entry.modifier();
                JsonObject row = new JsonObject();
                row.addProperty("attribute", attribute.unwrapKey().orElseThrow().identifier().toString());
                row.addProperty("id", modifier.id().toString());
                row.addProperty("operation", modifier.operation().getSerializedName());
                row.addProperty("amount", modifier.amount());
                entries.add(row);
            }
            if (!entries.isEmpty()) {
                slots.add(slot.getSerializedName(), entries);
            }
        }
        result.add("slots", slots);
        return result;
    }
}