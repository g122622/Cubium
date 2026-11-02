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
import java.util.Optional;
import net.minecraft.SharedConstants;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.level.block.state.properties.BlockStateProperties;
import net.minecraft.world.level.block.state.properties.BooleanProperty;

/**
 * {@code BooleanProperty} 内部值顺序的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BooleanPropertyIndexOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>本能力验证的是 <b>{@code getInternalIndex}</b> 的语义：
 *
 * <pre>{@code
 * public final class BooleanProperty extends Property<Boolean> {
 *     private static final List<Boolean> VALUES = List.of(true, false);   // true 在前
 *     private static final int TRUE_INDEX = 0;
 *     private static final int FALSE_INDEX = 1;
 *
 *     public int getInternalIndex(Boolean b) { return b ? TRUE_INDEX : FALSE_INDEX; }
 *     public List<Boolean> getPossibleValues() { return VALUES; }
 * }
 * }</pre>
 *
 * <p>注意 Cubium 的 {@code BooleanProperty} 构造函数用的是 {@code {false, true}}，
 * 顺序相反。本能力的 {@code index|true} / {@code at|0} 单元会暴露这一点。
 *
 * <p>原版有 {@code BlockStateProperties.MAP}（{@code map}），但 Cubium 没有；Cubium 另有
 * {@code WAXED}（{@code waxed}），原版没有。两者都不列入本能力的单元表——只覆盖双方共有的属性。
 *
 * <p>本能力<b>需要 bootstrap</b>：{@code BlockStateProperties} 的静态初始化会构造
 * {@code EnumProperty}，进而访问枚举的 {@code values()}。
 *
 * <p>单元表必须与 probes/boolean_property_index.cpp 逐项一致。
 */
public final class BooleanPropertyIndexOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版与 Cubium 共有的布尔属性名（按字母序）。 */
    private static final List<String> PROPERTY_NAMES =
            List.of(
                    "attached",
                    "berries",
                    "bloom",
                    "bottom",
                    "can_summon",
                    "conditional",
                    "cracked",
                    "crafting",
                    "disarmed",
                    "down",
                    "drag",
                    "east",
                    "enabled",
                    "extended",
                    "eye",
                    "falling",
                    "hanging",
                    "has_book",
                    "has_bottle_0",
                    "has_bottle_1",
                    "has_bottle_2",
                    "has_record",
                    "in_wall",
                    "inverted",
                    "lit",
                    "locked",
                    "natural",
                    "north",
                    "occupied",
                    "ominous",
                    "open",
                    "persistent",
                    "powered",
                    "short",
                    "shrieking",
                    "signal_fire",
                    "slot_0_occupied",
                    "slot_1_occupied",
                    "slot_2_occupied",
                    "slot_3_occupied",
                    "slot_4_occupied",
                    "slot_5_occupied",
                    "snowy",
                    "south",
                    "tip",
                    "triggered",
                    "unstable",
                    "up",
                    "waterlogged",
                    "west");

    private BooleanPropertyIndexOracle() {}

    /**
     * 按名字取原版的 {@code BooleanProperty}。
     *
     * <p>原版所有布尔属性都是 {@code BlockStateProperties} 的 {@code public static final} 字段，
     * 字段名是属性名的大写。用反射按名字查找，避免维护 51 个重复的 switch 分支。
     */
    private static BooleanProperty lookup(String name) {
        String fieldName = name.toUpperCase(java.util.Locale.ROOT);
        try {
            java.lang.reflect.Field field = BlockStateProperties.class.getField(fieldName);
            return (BooleanProperty) field.get(null);
        } catch (ReflectiveOperationException ex) {
            throw new IllegalArgumentException(
                    "boolean_property_index: 找不到原版布尔属性: " + name, ex);
        }
    }

    public static void main(String[] args) throws IOException {
        // 需要 bootstrap：BlockStateProperties 的静态初始化构造所有属性。
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String name : PROPERTY_NAMES) {
                appendUnits(text, name);
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BooleanPropertyIndexOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void appendUnits(StringBuilder text, String name) {
        text.append("values|").append(name).append('\n');
        text.append("count|").append(name).append('\n');
        text.append("name|").append(name).append('\n');
        text.append("index|").append(name).append("|true\n");
        text.append("index|").append(name).append("|false\n");
        text.append("at|").append(name).append("|0\n");
        text.append("at|").append(name).append("|1\n");
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
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length < 2) {
            throw new IllegalArgumentException("boolean_property_index: 单元段数不足: " + unit);
        }

        String mode = parts[0];
        BooleanProperty prop = lookup(parts[1]);
        String value;

        switch (mode) {
            case "values" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException(
                            "boolean_property_index: values 段数不符: " + unit);
                }
                List<String> flags = new ArrayList<>();
                for (Boolean b : prop.getPossibleValues()) {
                    flags.add(b ? "true" : "false");
                }
                value = String.join(",", flags);
            }
            case "count" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException(
                            "boolean_property_index: count 段数不符: " + unit);
                }
                value = String.valueOf(prop.getPossibleValues().size());
            }
            case "name" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException(
                            "boolean_property_index: name 段数不符: " + unit);
                }
                value = prop.getName();
            }
            case "index" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException(
                            "boolean_property_index: index 段数不符: " + unit);
                }
                if (!parts[2].equals("true") && !parts[2].equals("false")) {
                    throw new IllegalArgumentException(
                            "boolean_property_index: 非布尔值: " + parts[2]);
                }
                // 原版的公开入口是 getInternalIndex；getPossibleValues().indexOf 与之等价。
                value = String.valueOf(prop.getInternalIndex(Boolean.parseBoolean(parts[2])));
            }
            case "at" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException(
                            "boolean_property_index: at 段数不符: " + unit);
                }
                int index = Integer.parseInt(parts[2]);
                List<Boolean> possible = prop.getPossibleValues();
                if (index < 0 || index >= possible.size()) {
                    value = "null";
                } else {
                    value = possible.get(index) ? "true" : "false";
                }
            }
            default -> throw new IllegalArgumentException(
                    "boolean_property_index: 无法识别的单元: " + unit);
        }

        out.addProperty("value", value);
        return out;
    }
}