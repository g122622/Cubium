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
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.util.BlockUtil;
import net.minecraft.world.entity.EntityDimensions;
import net.minecraft.world.level.portal.PortalShape;
import net.minecraft.world.phys.Vec3;

/**
 * {@code PortalShape.getRelativePosition} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.PortalRelativeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版
 * {@link PortalShape#getRelativePosition(BlockUtil.FoundRectangle, Direction.Axis, Vec3, EntityDimensions)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元格式：`cornerX|cornerY|cornerZ|axis1Size|axis2Size|axis|width|height|posX|posY|posZ`。
 *
 * <p>单元表必须与 probes/portal_relative.cpp 逐项一致。
 */
public final class PortalRelativeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "0|0|0|2|3|x|0.6|1.8|1|0|0",
                    "0|0|0|2|3|x|0.6|1.8|-1|0|0",
                    "0|0|0|2|3|x|0.6|1.8|3|0|0",
                    "0|0|0|2|3|z|0.6|1.8|0|0|1",
                    "0|0|0|2|3|z|0.6|1.8|1|0|0",
                    "0|0|0|1|3|x|2.0|1.8|0|0|0",
                    "0|0|0|0|3|x|1.0|1.8|0|0|0",
                    "0|0|0|2|1|x|0.6|2.0|0|0|0",
                    "0|0|0|2|0|x|0.6|1.8|0|0|0",
                    "0|0|0|1|1|x|2.0|2.0|0|0|0",
                    "0|0|0|2|3|y|0.6|1.8|0|1|2",
                    "10|64|-20|3|4|x|0.6|1.8|12|64|-20");

    private PortalRelativeOracle() {}

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
                "usage: PortalRelativeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 格式化浮点：`%.9g` 并去掉尾随零，与探针的 C `%g` 对齐。 */
    private static String formatNumber(double value) {
        if (Double.isNaN(value)) {
            return "nan";
        }
        if (Double.isInfinite(value)) {
            return value > 0 ? "inf" : "-inf";
        }
        // Java 的 %.9g 中 9 是「小数位数」，与 C 的「有效数字位数」语义不同。
        // 改用 BigDecimal 按 9 位有效数字舍入后再去掉尾随零。
        java.math.BigDecimal rounded =
                new java.math.BigDecimal(value).round(new java.math.MathContext(9));
        String plain = rounded.stripTrailingZeros().toPlainString();
        if (plain.equals("-0")) {
            return "0";
        }
        return plain;
    }

    private static Direction.Axis parseAxis(String text) {
        return switch (text) {
            case "x" -> Direction.Axis.X;
            case "y" -> Direction.Axis.Y;
            case "z" -> Direction.Axis.Z;
            default -> throw new IllegalArgumentException("portal_relative: 未知轴: " + text);
        };
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 11) {
            throw new IllegalArgumentException("portal_relative: 单元需要 11 段: " + unit);
        }

        BlockPos corner = new BlockPos(
                (int) Math.floor(Double.parseDouble(parts[0])),
                (int) Math.floor(Double.parseDouble(parts[1])),
                (int) Math.floor(Double.parseDouble(parts[2])));
        // FoundRectangle 的两个尺寸字段是 int，不是 double。
        BlockUtil.FoundRectangle rectangle = new BlockUtil.FoundRectangle(
                corner, Integer.parseInt(parts[3]), Integer.parseInt(parts[4]));
        Direction.Axis axis = parseAxis(parts[5]);
        EntityDimensions dimensions = EntityDimensions.scalable(
                (float) Double.parseDouble(parts[6]), (float) Double.parseDouble(parts[7]));
        Vec3 pos = new Vec3(
                Double.parseDouble(parts[8]), Double.parseDouble(parts[9]),
                Double.parseDouble(parts[10]));

        Vec3 result = PortalShape.getRelativePosition(rectangle, axis, pos, dimensions);
        String value = formatNumber(result.x) + "," + formatNumber(result.y) + ","
                + formatNumber(result.z);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        out.addProperty("value", value);
        return out;
    }
}