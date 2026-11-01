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
import net.minecraft.world.level.block.state.properties.AttachFace;
import net.minecraft.world.level.block.state.properties.BedPart;
import net.minecraft.world.level.block.state.properties.ChestType;
import net.minecraft.world.level.block.state.properties.DoubleBlockHalf;
import net.minecraft.world.level.block.state.properties.Half;

/**
 * 方块状态枚举字符串映射的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockStateEnumStringsOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用各枚举的 {@code getSerializedName()}，反向用 {@code StringRepresentable.fromEnum}
 * 的单参数 {@code byName}（未知输入返回 {@code null}）。
 *
 * <p><b>本能力的探针对接 Cubium 的真实实现</b>
 * （{@code EnumProperty<T>::Traits::toString} / {@code fromName}）。
 *
 * <p>注意 {@code Half} 与 {@code DoubleBlockHalf} 是<b>两个不同的枚举</b>：
 * 前者 {@code TOP}/{@code BOTTOM}（{@code top}/{@code bottom}），
 * 后者 {@code UPPER}/{@code LOWER}（{@code upper}/{@code lower}）。
 * {@code DoubleBlockHalf} 没有自定义 id 字段，走 {@link StringRepresentable} 的默认实现
 * （{@code name().toLowerCase(Locale.ROOT)}），并且重写了 {@code toString()}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/block_state_enum_strings.cpp 逐项一致。
 */
public final class BlockStateEnumStringsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "count|AttachFace",
                    "count|BedPart",
                    "count|ChestType",
                    "count|DoubleBlockHalf",
                    "count|Half",
                    "AttachFace|toString|FLOOR",
                    "AttachFace|toString|WALL",
                    "AttachFace|toString|CEILING",
                    "AttachFace|fromName|floor",
                    "AttachFace|fromName|ceiling",
                    "AttachFace|fromName|dunno",
                    "AttachFace|fromName|FLOOR",
                    "BedPart|toString|HEAD",
                    "BedPart|toString|FOOT",
                    "BedPart|fromName|head",
                    "BedPart|fromName|foot",
                    "BedPart|fromName|dunno",
                    "ChestType|toString|SINGLE",
                    "ChestType|toString|LEFT",
                    "ChestType|toString|RIGHT",
                    "ChestType|fromName|single",
                    "ChestType|fromName|left",
                    "ChestType|fromName|right",
                    "ChestType|fromName|dunno",
                    "DoubleBlockHalf|toString|UPPER",
                    "DoubleBlockHalf|toString|LOWER",
                    "DoubleBlockHalf|fromName|upper",
                    "DoubleBlockHalf|fromName|lower",
                    "DoubleBlockHalf|fromName|top",
                    "Half|toString|TOP",
                    "Half|toString|BOTTOM",
                    "Half|fromName|top",
                    "Half|fromName|bottom",
                    "Half|fromName|upper");

    private BlockStateEnumStringsOracle() {}

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
        System.err.println("usage: BlockStateEnumStringsOracle list <units.txt> "
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
            throw new IllegalArgumentException("block_state_enum_strings: 单元段数不符: " + unit);
        }

        String enumName = parts[0];
        String op = parts[1];
        String arg = parts[2];

        if (op.equals("toString")) {
            out.addProperty("value", serializedNameOf(enumName, arg));
            return out;
        }
        if (!op.equals("fromName")) {
            throw new IllegalArgumentException("block_state_enum_strings: 未知操作: " + op);
        }

        String found = null;
        switch (enumName) {
            case "AttachFace" -> {
                AttachFace v = byName(AttachFace.values(), arg);
                found = v == null ? null : v.name();
            }
            case "BedPart" -> {
                BedPart v = byName(BedPart.values(), arg);
                found = v == null ? null : v.name();
            }
            case "ChestType" -> {
                ChestType v = byName(ChestType.values(), arg);
                found = v == null ? null : v.name();
            }
            case "DoubleBlockHalf" -> {
                DoubleBlockHalf v = byName(DoubleBlockHalf.values(), arg);
                found = v == null ? null : v.name();
            }
            case "Half" -> {
                Half v = byName(Half.values(), arg);
                found = v == null ? null : v.name();
            }
            default -> throw new IllegalArgumentException(
                    "block_state_enum_strings: 未知枚举: " + enumName);
        }
        out.addProperty("value", found == null ? "null" : found);
        return out;
    }

    private static int countOf(String enumName) {
        return switch (enumName) {
            case "AttachFace" -> AttachFace.values().length;
            case "BedPart" -> BedPart.values().length;
            case "ChestType" -> ChestType.values().length;
            case "DoubleBlockHalf" -> DoubleBlockHalf.values().length;
            case "Half" -> Half.values().length;
            default -> throw new IllegalArgumentException(
                    "block_state_enum_strings: 未知枚举: " + enumName);
        };
    }

    private static String serializedNameOf(String enumName, String valueName) {
        String upper = valueName.toUpperCase(Locale.ROOT);
        return switch (enumName) {
            case "AttachFace" -> AttachFace.valueOf(upper).getSerializedName();
            case "BedPart" -> BedPart.valueOf(upper).getSerializedName();
            case "ChestType" -> ChestType.valueOf(upper).getSerializedName();
            case "DoubleBlockHalf" -> DoubleBlockHalf.valueOf(upper).getSerializedName();
            case "Half" -> Half.valueOf(upper).getSerializedName();
            default -> throw new IllegalArgumentException(
                    "block_state_enum_strings: 未知枚举: " + enumName);
        };
    }
}