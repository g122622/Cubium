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
import net.minecraft.SharedConstants;
import net.minecraft.core.Direction;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.level.block.state.properties.BlockStateProperties;
import net.minecraft.world.level.block.state.properties.EnumProperty;

/**
 * {@code EnumProperty} 值顺序的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EnumPropertyOrderOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>本能力验证 {@code getPossibleValues()} 的顺序，以及由它决定的 {@code cycle()} 语义：
 *
 * <pre>{@code
 * public <T extends Comparable<T>> S cycle(Property<T> prop) {
 *     return this.setValue(prop, findNextInCollection(prop.getPossibleValues(), this.getValue(prop)));
 * }
 * protected static <T> T findNextInCollection(List<T> list, T value) {
 *     int i = list.indexOf(value) + 1;
 *     return i == list.size() ? list.getFirst() : list.get(i);
 * }
 * }</pre>
 *
 * <p>{@code cycle()} 的消费者是<b>调试棒（Debug Stick）</b>。
 *
 * <p>核心对照是 {@code BlockStateProperties.FACING}，它用了<b>显式列表</b>
 * {@code NORTH, EAST, SOUTH, WEST, UP, DOWN}，而 Cubium 的 {@code FACING()} 走
 * {@code Directions::all()}（{@code DOWN, UP, NORTH, SOUTH, WEST, EAST}）。
 *
 * <p>注意底层 {@code Direction} 枚举顺序两边<b>完全一致</b>
 * （{@code DOWN=0, UP=1, NORTH=2, SOUTH=3, WEST=4, EAST=5}），差异只在属性层。
 *
 * <p>本能力<b>需要 bootstrap</b>：{@code BlockStateProperties} 的静态初始化构造全部属性。
 *
 * <p>{@code cycle} 单元通过一个有单属性状态的方块复现：直接对
 * {@code Property} 的 {@code getPossibleValues()} 应用 {@code findNextInCollection} 语义，
 * 与 {@code StateHolder.cycle} 在单属性状态下的行为等价。
 *
 * <p>单元表必须与 probes/enum_property_order.cpp 逐项一致。
 */
public final class EnumPropertyOrderOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 参与比较的属性名（原版字段名小写）。 */
    private static final List<String> PROP_NAMES = List.of("facing", "horizontal_facing");

    /** 全部方向常量名。 */
    private static final List<String> DIR_NAMES =
            List.of("DOWN", "UP", "NORTH", "SOUTH", "WEST", "EAST");

    private EnumPropertyOrderOracle() {}

    @SuppressWarnings("unchecked")
    private static EnumProperty<Direction> lookup(String name) {
        String fieldName = name.equals("horizontal_facing")
                ? "HORIZONTAL_FACING"
                : name.toUpperCase(java.util.Locale.ROOT);
        try {
            java.lang.reflect.Field field = BlockStateProperties.class.getField(fieldName);
            return (EnumProperty<Direction>) field.get(null);
        } catch (ReflectiveOperationException ex) {
            throw new IllegalArgumentException("enum_property_order: 找不到原版属性: " + name, ex);
        }
    }

    public static void main(String[] args) throws IOException {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String name : PROP_NAMES) {
                text.append(name).append("|values\n");
                text.append(name).append("|count\n");
                text.append(name).append("|name\n");
                boolean sixWay = name.equals("facing");
                for (String dir : DIR_NAMES) {
                    // 水平属性不包含 UP/DOWN
                    if (!sixWay && (dir.equals("UP") || dir.equals("DOWN"))) {
                        continue;
                    }
                    text.append(name).append("|index|").append(dir).append('\n');
                    text.append(name).append("|cycle|").append(dir).append('\n');
                }
                int atCount = sixWay ? 6 : 4;
                for (int i = 0; i < atCount; ++i) {
                    text.append(name).append("|at|").append(i).append('\n');
                }
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EnumPropertyOrderOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
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

    /** 对齐 {@code StateHolder.findNextInCollection}。 */
    private static <T> T findNextInCollection(List<T> list, T value) {
        int i = list.indexOf(value) + 1;
        return i == list.size() ? list.getFirst() : list.get(i);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length < 2) {
            throw new IllegalArgumentException("enum_property_order: 单元段数不足: " + unit);
        }

        String mode = parts[1];
        EnumProperty<Direction> prop = lookup(parts[0]);
        String value;

        switch (mode) {
            case "values" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException(
                            "enum_property_order: values 段数不符: " + unit);
                }
                List<String> names = new ArrayList<>();
                for (Direction d : prop.getPossibleValues()) {
                    names.add(d.name());
                }
                value = String.join(",", names);
            }
            case "count" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException(
                            "enum_property_order: count 段数不符: " + unit);
                }
                value = String.valueOf(prop.getPossibleValues().size());
            }
            case "name" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException(
                            "enum_property_order: name 段数不符: " + unit);
                }
                value = prop.getName();
            }
            case "index" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException(
                            "enum_property_order: index 段数不符: " + unit);
                }
                Direction d = Direction.valueOf(parts[2]);
                value = String.valueOf(prop.getInternalIndex(d));
            }
            case "at" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException(
                            "enum_property_order: at 段数不符: " + unit);
                }
                int index = Integer.parseInt(parts[2]);
                List<Direction> possible = prop.getPossibleValues();
                value = (index < 0 || index >= possible.size())
                        ? "null"
                        : possible.get(index).name();
            }
            case "cycle" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException(
                            "enum_property_order: cycle 段数不符: " + unit);
                }
                Direction d = Direction.valueOf(parts[2]);
                if (!prop.getPossibleValues().contains(d)) {
                    throw new IllegalArgumentException(
                            "enum_property_order: 值不在属性中: " + parts[2]);
                }
                value = findNextInCollection(prop.getPossibleValues(), d).name();
            }
            default -> throw new IllegalArgumentException(
                    "enum_property_order: 无法识别的单元: " + unit);
        }

        out.addProperty("value", value);
        return out;
    }
}