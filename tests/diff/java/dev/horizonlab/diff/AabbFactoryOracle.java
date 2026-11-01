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
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;

/**
 * {@code AABB} 工厂方法与基础实例方法的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AabbFactoryOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link AABB#encapsulatingFullBlocks(BlockPos, BlockPos)} /
 * {@link AABB#ofSize(Vec3, double, double, double)} /
 * {@link AABB#getCenter()} / {@link AABB#getSize()} /
 * {@link AABB#inflate(double, double, double)} / {@link AABB#move(double, double, double)} /
 * {@link AABB#intersect(AABB)} / {@link AABB#hasNaN()}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>输出浮点用 `%.9g` 格式化，`nan` / `inf` 统一小写——与探针一致。
 *
 * <p>单元表必须与 probes/aabb_factory.cpp 逐项一致。
 */
public final class AabbFactoryOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "encap|0|0|0|0|0|0",
                    "encap|0|0|0|2|3|1",
                    "encap|2|3|1|0|0|0",
                    "encap|-1|-2|-3|1|2|3",
                    "ofSize|0|0|0|1|1|1",
                    "ofSize|10|64|-20|2|4|6",
                    "ofSize|0|0|0|0|0|0",
                    "center|0|0|0|1|1|1",
                    "center|10|64|-20|12|68|-16",
                    "center|-1|-1|-1|1|1|1",
                    "size|0|0|0|1|1|1",
                    "size|-1|-2|-3|1|2|3",
                    "size|0|0|0|3|1|1",
                    "axisSizes|0|0|0|1|1|1",
                    "axisSizes|-1|-2|-3|1|2|3",
                    "inflate|0|0|0|1|1|1|0.5|0.5|0.5",
                    "inflate|0|0|0|1|1|1|-0.1|-0.1|-0.1",
                    "move|0|0|0|1|1|1|10|64|-20",
                    "move|0|0|0|1|1|1|-0.5|-0.5|-0.5",
                    "intersect|0|0|0|2|2|2|1|1|1|3|3|3",
                    "intersect|0|0|0|1|1|1|5|5|5|6|6|6",
                    "intersect|0|0|0|10|10|10|2|2|2|3|3|3",
                    "hasNaN|0|0|0|1|1|1",
                    "hasNaN|nan|0|0|1|1|1",
                    "hasNaN|0|0|0|inf|1|1");

    private AabbFactoryOracle() {}

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
                "usage: AabbFactoryOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 格式化浮点：`%.9g`，`nan` / `inf` 统一小写。 */
    private static String formatNumber(double value) {
        if (Double.isNaN(value)) {
            return "nan";
        }
        if (Double.isInfinite(value)) {
            return value > 0 ? "inf" : "-inf";
        }
        // C 的 %g 会去掉尾随零，Java 的 %.9g 不会——这里手工去零，与探针的 printf("%.9g") 对齐。
        String formatted = String.format(Locale.ROOT, "%.9g", value);
        int exponentAt = formatted.indexOf('e');
        String mantissa = exponentAt < 0 ? formatted : formatted.substring(0, exponentAt);
        String exponent = exponentAt < 0 ? "" : formatted.substring(exponentAt);
        if (mantissa.indexOf('.') >= 0) {
            while (mantissa.endsWith("0")) {
                mantissa = mantissa.substring(0, mantissa.length() - 1);
            }
            if (mantissa.endsWith(".")) {
                mantissa = mantissa.substring(0, mantissa.length() - 1);
            }
        }
        return mantissa + exponent;
    }

    /** 渲染盒子为 `minX,minY,minZ,maxX,maxY,maxZ`。 */
    private static String render(AABB box) {
        return formatNumber(box.minX) + "," + formatNumber(box.minY) + ","
                + formatNumber(box.minZ) + "," + formatNumber(box.maxX) + ","
                + formatNumber(box.maxY) + "," + formatNumber(box.maxZ);
    }

    /** 渲染三维点为 `x,y,z`。 */
    private static String renderPoint(Vec3 vec) {
        return formatNumber(vec.x) + "," + formatNumber(vec.y) + "," + formatNumber(vec.z);
    }

    /** 解析浮点参数，支持 `nan` / `inf` 字面量。 */
    private static double parseNumber(String text) {
        return switch (text) {
            case "nan" -> Double.NaN;
            case "inf" -> Double.POSITIVE_INFINITY;
            case "-inf" -> Double.NEGATIVE_INFINITY;
            default -> Double.parseDouble(text);
        };
    }

    /** 从分段中解析一个盒子（偏移 6 个字段）。 */
    private static AABB parseBox(String[] parts, int offset) {
        return new AABB(
                parseNumber(parts[offset]), parseNumber(parts[offset + 1]),
                parseNumber(parts[offset + 2]), parseNumber(parts[offset + 3]),
                parseNumber(parts[offset + 4]), parseNumber(parts[offset + 5]));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("aabb_factory: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "encap" -> out.addProperty("value", render(AABB.encapsulatingFullBlocks(
                    new BlockPos(Integer.parseInt(parts[1]), Integer.parseInt(parts[2]),
                            Integer.parseInt(parts[3])),
                    new BlockPos(Integer.parseInt(parts[4]), Integer.parseInt(parts[5]),
                            Integer.parseInt(parts[6])))));
            case "ofSize" -> out.addProperty("value", render(AABB.ofSize(
                    new Vec3(parseNumber(parts[1]), parseNumber(parts[2]),
                            parseNumber(parts[3])),
                    parseNumber(parts[4]), parseNumber(parts[5]), parseNumber(parts[6]))));
            case "center" -> out.addProperty("value",
                    renderPoint(parseBox(parts, 1).getCenter()));
            case "size" -> out.addProperty("value", formatNumber(parseBox(parts, 1).getSize()));
            case "axisSizes" -> out.addProperty("value", renderPoint(new Vec3(
                    parseBox(parts, 1).getXsize(), parseBox(parts, 1).getYsize(),
                    parseBox(parts, 1).getZsize())));
            case "inflate" -> out.addProperty("value", render(parseBox(parts, 1).inflate(
                    parseNumber(parts[7]), parseNumber(parts[8]), parseNumber(parts[9]))));
            case "move" -> out.addProperty("value", render(parseBox(parts, 1).move(
                    parseNumber(parts[7]), parseNumber(parts[8]), parseNumber(parts[9]))));
            case "intersect" -> out.addProperty("value", render(
                    parseBox(parts, 1).intersect(parseBox(parts, 7))));
            case "hasNaN" -> out.addProperty("value", parseBox(parts, 1).hasNaN());
            default -> throw new IllegalArgumentException(
                    "aabb_factory: 无法识别的单元: " + unit);
        }

        return out;
    }
}