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
import java.math.RoundingMode;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.util.Mth;

/**
 * {@code Mth} 钳制/映射/平方函数族的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthClampOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#clamp(int, int, int)} / {@link Mth#clamp(float, float, float)} /
 * {@link Mth#clamp(double, double, double)} / {@link Mth#clampedLerp(double, double, double)} /
 * {@link Mth#clampedLerp(float, float, float)} / {@link Mth#absMax(int, int)} /
 * {@link Mth#chessboardDistance(int, int, int, int)} / {@link Mth#square(int)} /
 * {@link Mth#square(long)} / {@link Mth#inverseLerp(double, double, double)} /
 * {@link Mth#clampedMap(double, double, double, double, double)}。
 *
 * <p>浮点输出按 `%.9g` 等价格式化（`BigDecimal` 有效数字 9 位），与探针侧一致。
 *
 * <p>单元表必须与 probes/mth_clamp.cpp 逐项一致。
 */
public final class MthClampOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "clampi|5|0|10",
                    "clampi|0|0|10",
                    "clampi|10|0|10",
                    "clampi|15|0|10",
                    "clampi|-5|0|10",
                    "clampi|5|10|0",
                    "clampi|5|5|5",
                    "clampf|5|0|10",
                    "clampf|-5|0|10",
                    "clampf|NaN|0|10",
                    "clampf|Inf|0|10",
                    "clampd|0.5|0|1",
                    "clampd|-0.1|0|1",
                    "clampd|NaN|0|1",
                    "clampedLerpD|-0.5|10|20",
                    "clampedLerpD|0|10|20",
                    "clampedLerpD|0.5|10|20",
                    "clampedLerpD|1|10|20",
                    "clampedLerpD|1.5|10|20",
                    "clampedLerpF|-0.5|10|20",
                    "clampedLerpF|0.25|10|20",
                    "absMaxI|-5|3",
                    "absMaxI|5|-3",
                    "absMaxI|0|0",
                    "absMaxD|-2.5|1.5",
                    "chessboard|0|3|0|4",
                    "chessboard|0|0|0|0",
                    "chessboard|10|3|10|4",
                    "squareI|5",
                    "squareI|-5",
                    "squareI|46340",
                    "squareI|46341",
                    "squareL|46341",
                    "squareD|2.5",
                    "inverseLerpD|15|10|20",
                    "inverseLerpD|10|10|20",
                    "inverseLerpD|20|10|20",
                    "inverseLerpD|5|10|10",
                    "clampedMapD|15|10|20|0|100",
                    "clampedMapD|5|10|20|0|100",
                    "clampedMapD|25|10|20|0|100",
                    "clampedMapD|10|10|10|0|100",
                    // clampedMap 的边界样本：上面的用例都是 10/20、0/100 这类整齐数值，
                    // 在小数端点上无法暴露 `outMin + t*(outMax-outMin)` 被 FMA 融合的问题。
            "clampedMapD|12.545929800000003|2.7720000000000002|17.118000000000002|4.8760000000000003|6.9310000000000009",
            "clampedMapD|11.2066044|8.1850000000000005|16.625|6.8540000000000001|23.667999999999999",
            "clampedMapD|15.084479330000002|1.7890000000000001|16.327999999999999|13.249000000000001|26.125",
            "clampedMapD|3.0866682900000004|1.1879999999999999|4.8849999999999998|6.7789999999999999|17.59",
            "clampedMapD|15.342021949999999|10.59|15.994999999999999|1.2130000000000001|4.4690000000000003",
            "clampedMapD|13.80464952|9.4369999999999994|15.183|12.515000000000001|19.741");

    private MthClampOracle() {}

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
                "usage: MthClampOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static int parseInt(String text) {
        return switch (text) {
            case "MIN" -> Integer.MIN_VALUE;
            case "MAX" -> Integer.MAX_VALUE;
            default -> Integer.parseInt(text);
        };
    }

    private static double parseDouble(String text) {
        return switch (text) {
            case "NaN" -> Double.NaN;
            case "Inf" -> Double.POSITIVE_INFINITY;
            case "-Inf" -> Double.NEGATIVE_INFINITY;
            default -> Double.parseDouble(text);
        };
    }

    /**
     * 与探针侧 `%.9g` 等价的格式化：9 位有效数字，去掉尾随零。
     *
     * <p>Java 没有直接等价于 `%g` 的格式化，这里用 `BigDecimal` 的 9 位有效数字近似，并手工
     * 去零。对能力里的输入（都是可精确表示的十进制小数或整数）足以对齐。
     */
    private static String formatNumber(double value) {
        if (Double.isNaN(value)) {
            return "NaN";
        }
        if (Double.isInfinite(value)) {
            return value > 0 ? "Infinity" : "-Infinity";
        }
        if (value == 0.0) {
            return "0";
        }
        BigDecimal rounded = new BigDecimal(value)
                .round(new MathContext(9, RoundingMode.HALF_EVEN))
                .stripTrailingZeros();
        // 用 16 位有效数字输出，再做与探针一致的去零处理。
        String text = rounded.toEngineeringString();
        // 去掉 `E+` 里的 `+`，与 C 的 `%g` 输出保持一致。
        text = text.replace("E+", "E");
        return text;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_clamp: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "clampi" -> out.addProperty("value",
                    Integer.toString(Mth.clamp(
                            parseInt(parts[1]), parseInt(parts[2]), parseInt(parts[3]))));
            case "clampf" -> out.addProperty("value",
                    formatNumber(Mth.clamp(
                            (float) parseDouble(parts[1]),
                            (float) parseDouble(parts[2]),
                            (float) parseDouble(parts[3]))));
            case "clampd" -> out.addProperty("value",
                    formatNumber(Mth.clamp(
                            parseDouble(parts[1]), parseDouble(parts[2]), parseDouble(parts[3]))));
            case "clampedLerpD" -> out.addProperty("value",
                    formatNumber(Mth.clampedLerp(
                            parseDouble(parts[1]), parseDouble(parts[2]), parseDouble(parts[3]))));
            case "clampedLerpF" -> out.addProperty("value",
                    formatNumber(Mth.clampedLerp(
                            (float) parseDouble(parts[1]),
                            (float) parseDouble(parts[2]),
                            (float) parseDouble(parts[3]))));
            case "absMaxI" -> out.addProperty("value",
                    Integer.toString(Mth.absMax(parseInt(parts[1]), parseInt(parts[2]))));
            case "absMaxD" -> out.addProperty("value",
                    formatNumber(Math.max(
                            Math.abs(parseDouble(parts[1])), Math.abs(parseDouble(parts[2])))));
            case "chessboard" -> out.addProperty("value",
                    Integer.toString(Mth.chessboardDistance(
                            parseInt(parts[1]), parseInt(parts[2]),
                            parseInt(parts[3]), parseInt(parts[4]))));
            case "squareI" -> out.addProperty("value",
                    Integer.toString(Mth.square(parseInt(parts[1]))));
            case "squareL" -> out.addProperty("value",
                    Long.toString(Mth.square(Long.parseLong(parts[1]))));
            case "squareD" -> {
                double v = parseDouble(parts[1]);
                out.addProperty("value", formatNumber(v * v));
            }
            case "inverseLerpD" -> out.addProperty("value",
                    formatNumber(Mth.inverseLerp(
                            parseDouble(parts[1]), parseDouble(parts[2]), parseDouble(parts[3]))));
            case "clampedMapD" -> out.addProperty("value",
                    formatNumber(Mth.clampedMap(
                            parseDouble(parts[1]),
                            parseDouble(parts[2]),
                            parseDouble(parts[3]),
                            parseDouble(parts[4]),
                            parseDouble(parts[5]))));
            default -> throw new IllegalArgumentException("mth_clamp: 无法识别的单元: " + unit);
        }

        return out;
    }
}