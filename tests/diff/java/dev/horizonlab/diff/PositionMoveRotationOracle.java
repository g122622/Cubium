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
import java.math.BigDecimal;
import java.math.MathContext;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import net.minecraft.world.entity.PositionMoveRotation;
import net.minecraft.world.entity.Relative;
import net.minecraft.world.phys.Vec3;

/**
 * {@code PositionMoveRotation.calculateAbsolute} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.PositionMoveRotationOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版
 * {@link PositionMoveRotation#calculateAbsolute(PositionMoveRotation, PositionMoveRotation, Set)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元格式（17 段）：
 * `p1` 的 8 段 + `p2` 的 8 段 + `relatives`（逗号分隔，空集写 `-`）。
 *
 * <p>单元表必须与 probes/position_move_rotation.cpp 逐项一致。
 */
public final class PositionMoveRotationOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "0|0|0|0|0|0|0|0|1|2|3|0.5|0.25|0.125|10|20|-",
                    "1|2|3|0|0|0|0|0|10|20|30|0|0|0|0|0|x,y,z",
                    "0|0|0|0|0|0|0|0|10|20|30|0|0|0|0|0|x,y,z",
                    "5|0|0|0|0|0|0|0|10|20|30|0|0|0|0|0|x",
                    "0|0|0|0|0|0|45|0|0|0|0|0|0|0|10|0|y_rot",
                    "0|0|0|0|0|0|45|0|0|0|0|0|0|0|10|0|-",
                    "0|0|0|0|0|0|0|70|0|0|0|0|0|0|0|40|x_rot",
                    "0|0|0|0|0|0|0|-70|0|0|0|0|0|0|0|-40|x_rot",
                    "0|0|0|0|0|0|170|0|0|0|0|0|0|0|40|0|y_rot",
                    "0|0|0|1|2|3|0|0|0|0|0|10|20|30|0|0|delta_x,delta_y,delta_z",
                    "0|0|0|1|2|3|0|0|0|0|0|10|20|30|0|0|-",
                    "0|0|0|1|0|0|30|0|0|0|0|10|0|0|0|0|rotate_delta");

    private PositionMoveRotationOracle() {}

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
        System.err.println("usage: PositionMoveRotationOracle list <units.txt> "
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

    /** 格式化浮点：9 位有效数字并去掉尾随零。 */
    private static String formatNumber(double value) {
        if (Double.isNaN(value)) {
            return "nan";
        }
        if (Double.isInfinite(value)) {
            return value > 0 ? "inf" : "-inf";
        }
        BigDecimal rounded = new BigDecimal(value).round(new MathContext(9));
        String plain = rounded.stripTrailingZeros().toPlainString();
        return plain.equals("-0") ? "0" : plain;
    }

    private static Set<Relative> parseMask(String text) {
        Set<Relative> set = EnumSet.noneOf(Relative.class);
        if (text.isEmpty() || text.equals("-")) {
            return set;
        }
        for (String token : text.split(",", -1)) {
            if (!token.isEmpty()) {
                set.add(Relative.valueOf(token.toUpperCase(Locale.ROOT)));
            }
        }
        return set;
    }

    private static PositionMoveRotation parsePmr(String[] parts, int offset) {
        return new PositionMoveRotation(
                new Vec3(Double.parseDouble(parts[offset]),
                        Double.parseDouble(parts[offset + 1]),
                        Double.parseDouble(parts[offset + 2])),
                new Vec3(Double.parseDouble(parts[offset + 3]),
                        Double.parseDouble(parts[offset + 4]),
                        Double.parseDouble(parts[offset + 5])),
                Float.parseFloat(parts[offset + 6]),
                Float.parseFloat(parts[offset + 7]));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 17) {
            throw new IllegalArgumentException(
                    "position_move_rotation: 单元需要 17 段: " + unit);
        }

        PositionMoveRotation p1 = parsePmr(parts, 0);
        PositionMoveRotation p2 = parsePmr(parts, 8);
        Set<Relative> relatives = parseMask(parts[16]);

        PositionMoveRotation result =
                PositionMoveRotation.calculateAbsolute(p1, p2, relatives);

        String value = formatNumber(result.position().x) + ","
                + formatNumber(result.position().y) + ","
                + formatNumber(result.position().z) + "|"
                + formatNumber(result.deltaMovement().x) + ","
                + formatNumber(result.deltaMovement().y) + ","
                + formatNumber(result.deltaMovement().z) + "|"
                + formatNumber(result.yRot()) + "|"
                + formatNumber(result.xRot());

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        out.addProperty("value", value);
        return out;
    }
}