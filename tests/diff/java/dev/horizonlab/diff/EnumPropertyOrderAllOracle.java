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
import net.minecraft.SharedConstants;
import net.minecraft.server.Bootstrap;
import net.minecraft.util.StringRepresentable;
import net.minecraft.world.level.block.state.properties.BlockStateProperties;
import net.minecraft.world.level.block.state.properties.Property;

/**
 * 全部 40 个 {@code EnumProperty} 值顺序的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EnumPropertyOrderAllOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>每个属性产出 {@code values}（按索引顺序的序列化名，逗号分隔）、{@code count}、
 * {@code name}（属性字符串名）。
 *
 * <p>只比 {@code values} 就够：{@code enum_property_order} 已验证 {@code cycle()} 与
 * {@code getInternalIndex} 的差异<b>完全来自</b> {@code getPossibleValues()} 的顺序。
 *
 * <p>字段名映射（左 = Cubium 前缀，右 = 原版字段名）：
 *
 * <pre>
 *   FACING_HOPPER        -> <b>FACING_EXCEPT_UP</b>
 *   DOOR_HINGE           -> <b>HINGE</b>
 *   NOTEBLOCK_INSTRUMENT -> <b>NOTE_BLOCK_INSTRUMENT</b>
 *   STRUCTUREBLOCK_MODE  -> <b>STRUCTURE_MODE</b>
 *   MODE_COMPARATOR      -> <b>COMPARATOR_MODE</b>
 *   EAST_WALL            -> <b>WALL_HEIGHT_EAST</b>（同理 N/S/W）
 *   EAST_REDSTONE        -> <b>REDSTONE_EAST</b>（同理 N/S/W）
 *   BAMBOO_LEAVES        -> <b>BAMBOO_LEAVES_PROP</b>
 * </pre>
 *
 * <p>已知陷阱：原版 {@code VERTICAL_DIRECTION} 是显式列表 {@code UP, DOWN}；
 * 若 Cubium 走谓词遍历 {@code Directions::all()}，会得到 {@code DOWN, UP}——顺序相反。
 *
 * <p>本能力<b>需要 bootstrap</b>：{@code BlockStateProperties} 的静态初始化构造全部属性。
 */
public final class EnumPropertyOrderAllOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** Cubium 访问器名 → 原版字段名。顺序即单元表顺序。 */
    private static final Map<String, String> ACCESSOR_TO_FIELD = new LinkedHashMap<>();

    static {
        ACCESSOR_TO_FIELD.put("FACING", "FACING");
        ACCESSOR_TO_FIELD.put("HORIZONTAL_FACING", "HORIZONTAL_FACING");
        ACCESSOR_TO_FIELD.put("VERTICAL_DIRECTION", "VERTICAL_DIRECTION");
        ACCESSOR_TO_FIELD.put("FACING_EXCEPT_UP", "FACING_HOPPER");
        ACCESSOR_TO_FIELD.put("AXIS", "AXIS");
        ACCESSOR_TO_FIELD.put("HORIZONTAL_AXIS", "HORIZONTAL_AXIS");
        ACCESSOR_TO_FIELD.put("CHEST_TYPE", "CHEST_TYPE");
        ACCESSOR_TO_FIELD.put("DOUBLE_BLOCK_HALF", "DOUBLE_BLOCK_HALF");
        ACCESSOR_TO_FIELD.put("HINGE", "DOOR_HINGE");
        ACCESSOR_TO_FIELD.put("BED_PART", "BED_PART");
        ACCESSOR_TO_FIELD.put("BELL_ATTACHMENT", "BELL_ATTACHMENT");
        ACCESSOR_TO_FIELD.put("STAIRS_SHAPE", "STAIRS_SHAPE");
        ACCESSOR_TO_FIELD.put("SLAB_TYPE", "SLAB_TYPE");
        ACCESSOR_TO_FIELD.put("WALL_HEIGHT_NORTH", "NORTH_WALL");
        ACCESSOR_TO_FIELD.put("WALL_HEIGHT_EAST", "EAST_WALL");
        ACCESSOR_TO_FIELD.put("WALL_HEIGHT_SOUTH", "SOUTH_WALL");
        ACCESSOR_TO_FIELD.put("WALL_HEIGHT_WEST", "WEST_WALL");
        ACCESSOR_TO_FIELD.put("ATTACH_FACE", "ATTACH_FACE");
        ACCESSOR_TO_FIELD.put("BAMBOO_LEAVES_PROP", "BAMBOO_LEAVES");
        ACCESSOR_TO_FIELD.put("HALF", "HALF");
        ACCESSOR_TO_FIELD.put("RAIL_SHAPE", "RAIL_SHAPE");
        ACCESSOR_TO_FIELD.put("RAIL_SHAPE_STRAIGHT", "RAIL_SHAPE_STRAIGHT");
        ACCESSOR_TO_FIELD.put("REDSTONE_NORTH", "NORTH_REDSTONE");
        ACCESSOR_TO_FIELD.put("REDSTONE_EAST", "EAST_REDSTONE");
        ACCESSOR_TO_FIELD.put("REDSTONE_SOUTH", "SOUTH_REDSTONE");
        ACCESSOR_TO_FIELD.put("REDSTONE_WEST", "WEST_REDSTONE");
        ACCESSOR_TO_FIELD.put("PISTON_TYPE", "PISTON_TYPE");
        ACCESSOR_TO_FIELD.put("COMPARATOR_MODE", "MODE_COMPARATOR");
        ACCESSOR_TO_FIELD.put("NOTE_BLOCK_INSTRUMENT", "NOTEBLOCK_INSTRUMENT");
        ACCESSOR_TO_FIELD.put("STRUCTURE_MODE", "STRUCTUREBLOCK_MODE");
        ACCESSOR_TO_FIELD.put("TEST_BLOCK_MODE", "TEST_BLOCK_MODE");
        ACCESSOR_TO_FIELD.put("DRIPSTONE_THICKNESS", "DRIPSTONE_THICKNESS");
        ACCESSOR_TO_FIELD.put("TILT", "TILT");
        ACCESSOR_TO_FIELD.put("SCULK_SENSOR_PHASE", "SCULK_SENSOR_PHASE");
        ACCESSOR_TO_FIELD.put("TRIAL_SPAWNER_STATE", "TRIAL_SPAWNER_STATE");
        ACCESSOR_TO_FIELD.put("VAULT_STATE", "VAULT_STATE");
        ACCESSOR_TO_FIELD.put("CREAKING_HEART_STATE", "CREAKING_HEART_STATE");
        ACCESSOR_TO_FIELD.put("SIDE_CHAIN_PART", "SIDE_CHAIN_PART");
        ACCESSOR_TO_FIELD.put("COPPER_GOLEM_POSE", "COPPER_GOLEM_POSE");
        ACCESSOR_TO_FIELD.put("ORIENTATION", "ORIENTATION");
    }

    private EnumPropertyOrderAllOracle() {}

    /** 反射取原版 {@code BlockStateProperties} 的字段。 */
    private static Property<?> lookup(String fieldName) {
        try {
            java.lang.reflect.Field field = BlockStateProperties.class.getField(fieldName);
            return (Property<?>) field.get(null);
        } catch (ReflectiveOperationException ex) {
            throw new IllegalArgumentException(
                    "enum_property_order_all: 找不到原版字段: " + fieldName, ex);
        }
    }

    /** 序列化名：优先 {@code getSerializedName()}，退化用 {@code name()}。 */
    @SuppressWarnings({"rawtypes", "unchecked"})
    private static String serializedName(Property<?> prop, int index) {
        Object value = prop.getPossibleValues().get(index);
        if (value instanceof StringRepresentable sr) {
            return sr.getSerializedName();
        }
        return String.valueOf(value).toLowerCase(java.util.Locale.ROOT);
    }

    public static void main(String[] args) throws IOException {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String accessor : ACCESSOR_TO_FIELD.keySet()) {
                text.append(accessor).append("|values\n");
                text.append(accessor).append("|count\n");
                text.append(accessor).append("|name\n");
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EnumPropertyOrderAllOracle list <units.txt> "
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

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 2) {
            throw new IllegalArgumentException("enum_property_order_all: 单元段数不符: " + unit);
        }

        String fieldName = ACCESSOR_TO_FIELD.get(parts[0]);
        if (fieldName == null) {
            throw new IllegalArgumentException("enum_property_order_all: 未知属性: " + parts[0]);
        }
        Property<?> prop = lookup(fieldName);

        String value;
        switch (parts[1]) {
            case "values" -> {
                List<String> names = new ArrayList<>();
                for (int i = 0; i < prop.getPossibleValues().size(); ++i) {
                    names.add(serializedName(prop, i));
                }
                value = String.join(",", names);
            }
            case "count" -> value = String.valueOf(prop.getPossibleValues().size());
            case "name" -> value = prop.getName();
            default -> throw new IllegalArgumentException(
                    "enum_property_order_all: 未知单元类型: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        out.addProperty("value", value);
        return out;
    }
}