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
import java.util.Locale;
import net.minecraft.util.StringRepresentable;
import net.minecraft.world.level.block.state.properties.BambooLeaves;
import net.minecraft.world.level.block.state.properties.BellAttachType;
import net.minecraft.world.level.block.state.properties.DoorHingeSide;
import net.minecraft.world.level.block.state.properties.DripstoneThickness;
import net.minecraft.world.level.block.state.properties.PistonType;
import net.minecraft.world.level.block.state.properties.RailShape;
import net.minecraft.world.level.block.state.properties.RedstoneSide;
import net.minecraft.world.level.block.state.properties.SculkSensorPhase;
import net.minecraft.world.level.block.state.properties.SlabType;
import net.minecraft.world.level.block.state.properties.StairsShape;
import net.minecraft.world.level.block.state.properties.Tilt;

/**
 * 第二组方块状态枚举字符串映射的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockStateEnumStrings2Oracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用各枚举的 {@code getSerializedName()}，反向用 {@code StringRepresentable.fromEnum}
 * 的单参数 {@code byName}（未知输入返回 {@code null}）。
 *
 * <p><b>本能力的探针对接 Cubium 的真实实现</b>
 * （{@code EnumProperty<T>::Traits::toString} / {@code fromName}）。
 *
 * <p>注意两个命名陷阱：
 * <ul>
 *   <li>{@code PistonType.DEFAULT} 的序列化名是 <b>{@code normal}</b>，不是 {@code default}；</li>
 *   <li>{@code SlabType} 的顺序是 {@code BOTTOM}/{@code TOP}/{@code DOUBLE}，
 *       与 {@code Half} 的 {@code TOP}/{@code BOTTOM} <b>相反</b>。</li>
 * </ul>
 *
 * <p>注意原版类名与 Cubium 枚举名不完全一致：
 * {@code BellAttachType}（原版）↔ {@code BellAttachment}（Cubium），
 * {@code DoorHingeSide}（原版）↔ {@code DoorHinge}（Cubium）。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/block_state_enum_strings_2.cpp 逐项一致。
 */
public final class BlockStateEnumStrings2Oracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "count|PistonType",
                    "count|RedstoneSide",
                    "count|DoorHinge",
                    "count|SlabType",
                    "count|StairsShape",
                    "count|RailShape",
                    "count|BellAttachment",
                    "count|BambooLeaves",
                    "count|Tilt",
                    "count|DripstoneThickness",
                    "count|SculkSensorPhase",
                    "PistonType|toString|DEFAULT",
                    "PistonType|toString|STICKY",
                    "PistonType|fromName|normal",
                    "PistonType|fromName|sticky",
                    "PistonType|fromName|default",
                    "RedstoneSide|toString|UP",
                    "RedstoneSide|toString|SIDE",
                    "RedstoneSide|toString|NONE",
                    "RedstoneSide|fromName|up",
                    "RedstoneSide|fromName|none",
                    "DoorHinge|toString|LEFT",
                    "DoorHinge|toString|RIGHT",
                    "DoorHinge|fromName|left",
                    "DoorHinge|fromName|right",
                    "SlabType|toString|BOTTOM",
                    "SlabType|toString|TOP",
                    "SlabType|toString|DOUBLE",
                    "SlabType|fromName|bottom",
                    "SlabType|fromName|double",
                    "StairsShape|toString|STRAIGHT",
                    "StairsShape|toString|INNER_LEFT",
                    "StairsShape|toString|INNER_RIGHT",
                    "StairsShape|toString|OUTER_LEFT",
                    "StairsShape|toString|OUTER_RIGHT",
                    "StairsShape|fromName|inner_left",
                    "StairsShape|fromName|outer_right",
                    "RailShape|toString|NORTH_SOUTH",
                    "RailShape|toString|EAST_WEST",
                    "RailShape|toString|ASCENDING_EAST",
                    "RailShape|toString|ASCENDING_WEST",
                    "RailShape|toString|ASCENDING_NORTH",
                    "RailShape|toString|ASCENDING_SOUTH",
                    "RailShape|toString|SOUTH_EAST",
                    "RailShape|toString|SOUTH_WEST",
                    "RailShape|toString|NORTH_WEST",
                    "RailShape|toString|NORTH_EAST",
                    "RailShape|fromName|north_south",
                    "RailShape|fromName|north_east",
                    "BellAttachment|toString|FLOOR",
                    "BellAttachment|toString|CEILING",
                    "BellAttachment|toString|SINGLE_WALL",
                    "BellAttachment|toString|DOUBLE_WALL",
                    "BellAttachment|fromName|single_wall",
                    "BellAttachment|fromName|double_wall",
                    "BambooLeaves|toString|NONE",
                    "BambooLeaves|toString|SMALL",
                    "BambooLeaves|toString|LARGE",
                    "BambooLeaves|fromName|large",
                    "Tilt|toString|NONE",
                    "Tilt|toString|UNSTABLE",
                    "Tilt|toString|PARTIAL",
                    "Tilt|toString|FULL",
                    "Tilt|fromName|unstable",
                    "DripstoneThickness|toString|TIP_MERGE",
                    "DripstoneThickness|toString|TIP",
                    "DripstoneThickness|toString|FRUSTUM",
                    "DripstoneThickness|toString|MIDDLE",
                    "DripstoneThickness|toString|BASE",
                    "DripstoneThickness|fromName|tip_merge",
                    "DripstoneThickness|fromName|frustum",
                    "SculkSensorPhase|toString|INACTIVE",
                    "SculkSensorPhase|toString|ACTIVE",
                    "SculkSensorPhase|toString|COOLDOWN",
                    "SculkSensorPhase|fromName|cooldown",
                    "PistonType|fromName|dunno",
                    "RailShape|fromName|dunno",
                    "StairsShape|fromName|dunno");

    private BlockStateEnumStrings2Oracle() {}

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
        System.err.println("usage: BlockStateEnumStrings2Oracle list <units.txt> "
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

    /** 通用反向查找：按序列化名精确匹配，未命中返回 null。 */
    private static <E extends Enum<E> & StringRepresentable> E byName(E[] values, String name) {
        for (E value : values) {
            if (value.getSerializedName().equals(name)) {
                return value;
            }
        }
        return null;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 2 && parts[0].equals("count")) {
            out.addProperty("value", String.valueOf(countOf(parts[1])));
            return out;
        }
        if (parts.length != 3) {
            throw new IllegalArgumentException(
                    "block_state_enum_strings_2: 单元段数不符: " + unit);
        }

        String enumName = parts[0];
        String op = parts[1];
        String arg = parts[2];

        if (op.equals("toString")) {
            out.addProperty("value", serializedNameOf(enumName, arg));
            return out;
        }
        if (!op.equals("fromName")) {
            throw new IllegalArgumentException(
                    "block_state_enum_strings_2: 未知操作: " + op);
        }

        String found;
        switch (enumName) {
            case "PistonType" -> {
                PistonType v = byName(PistonType.values(), arg);
                found = v == null ? null : v.name();
            }
            case "RedstoneSide" -> {
                RedstoneSide v = byName(RedstoneSide.values(), arg);
                found = v == null ? null : v.name();
            }
            case "DoorHinge" -> {
                DoorHingeSide v = byName(DoorHingeSide.values(), arg);
                found = v == null ? null : v.name();
            }
            case "SlabType" -> {
                SlabType v = byName(SlabType.values(), arg);
                found = v == null ? null : v.name();
            }
            case "StairsShape" -> {
                StairsShape v = byName(StairsShape.values(), arg);
                found = v == null ? null : v.name();
            }
            case "RailShape" -> {
                RailShape v = byName(RailShape.values(), arg);
                found = v == null ? null : v.name();
            }
            case "BellAttachment" -> {
                BellAttachType v = byName(BellAttachType.values(), arg);
                found = v == null ? null : v.name();
            }
            case "BambooLeaves" -> {
                BambooLeaves v = byName(BambooLeaves.values(), arg);
                found = v == null ? null : v.name();
            }
            case "Tilt" -> {
                Tilt v = byName(Tilt.values(), arg);
                found = v == null ? null : v.name();
            }
            case "DripstoneThickness" -> {
                DripstoneThickness v = byName(DripstoneThickness.values(), arg);
                found = v == null ? null : v.name();
            }
            case "SculkSensorPhase" -> {
                SculkSensorPhase v = byName(SculkSensorPhase.values(), arg);
                found = v == null ? null : v.name();
            }
            default -> throw new IllegalArgumentException(
                    "block_state_enum_strings_2: 未知枚举: " + enumName);
        }
        out.addProperty("value", found == null ? "null" : found);
        return out;
    }

    private static int countOf(String enumName) {
        return switch (enumName) {
            case "PistonType" -> PistonType.values().length;
            case "RedstoneSide" -> RedstoneSide.values().length;
            case "DoorHinge" -> DoorHingeSide.values().length;
            case "SlabType" -> SlabType.values().length;
            case "StairsShape" -> StairsShape.values().length;
            case "RailShape" -> RailShape.values().length;
            case "BellAttachment" -> BellAttachType.values().length;
            case "BambooLeaves" -> BambooLeaves.values().length;
            case "Tilt" -> Tilt.values().length;
            case "DripstoneThickness" -> DripstoneThickness.values().length;
            case "SculkSensorPhase" -> SculkSensorPhase.values().length;
            default -> throw new IllegalArgumentException(
                    "block_state_enum_strings_2: 未知枚举: " + enumName);
        };
    }

    private static String serializedNameOf(String enumName, String valueName) {
        String upper = valueName.toUpperCase(Locale.ROOT);
        return switch (enumName) {
            case "PistonType" -> PistonType.valueOf(upper).getSerializedName();
            case "RedstoneSide" -> RedstoneSide.valueOf(upper).getSerializedName();
            case "DoorHinge" -> DoorHingeSide.valueOf(upper).getSerializedName();
            case "SlabType" -> SlabType.valueOf(upper).getSerializedName();
            case "StairsShape" -> StairsShape.valueOf(upper).getSerializedName();
            case "RailShape" -> RailShape.valueOf(upper).getSerializedName();
            case "BellAttachment" -> BellAttachType.valueOf(upper).getSerializedName();
            case "BambooLeaves" -> BambooLeaves.valueOf(upper).getSerializedName();
            case "Tilt" -> Tilt.valueOf(upper).getSerializedName();
            case "DripstoneThickness" -> DripstoneThickness.valueOf(upper).getSerializedName();
            case "SculkSensorPhase" -> SculkSensorPhase.valueOf(upper).getSerializedName();
            default -> throw new IllegalArgumentException(
                    "block_state_enum_strings_2: 未知枚举: " + enumName);
        };
    }
}