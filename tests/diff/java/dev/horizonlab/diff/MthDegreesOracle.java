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
import net.minecraft.util.Mth;

/**
 * {@code Mth} 角度差与朝向平滑的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthDegreesOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#degreesDifference(float, float)} /
 * {@link Mth#degreesDifferenceAbs(float, float)} /
 * {@link Mth#approachDegrees(float, float, float)} /
 * {@link Mth#rotateIfNecessary(float, float, float)} /
 * {@link Mth#absMax(int, int)} / {@link Mth#absMax(float, float)} /
 * {@link Mth#absMax(double, double)} / {@link Mth#sign(double)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>角度类结果是 `float`，按 `%.9g` 格式化后去尾零；`absMaxI` 与 `sign` 输出整数。
 *
 * <p>单元表必须与 probes/mth_degrees.cpp 逐项一致。
 */
public final class MthDegreesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "diff|0|90",
                    "diff|90|0",
                    "diff|170|-170",
                    "diff|-170|170",
                    "diff|0|180",
                    "diff|180|0",
                    "diffAbs|170|-170",
                    "diffAbs|-170|170",
                    "approachDeg|0|90|10",
                    "approachDeg|0|90|100",
                    "approachDeg|90|0|10",
                    "approachDeg|170|-170|10",
                    "approachDeg|-170|170|10",
                    "rotate|0|90|10",
                    "rotate|0|90|100",
                    "rotate|90|0|10",
                    "rotate|170|-170|5",
                    "rotate|-170|170|5",
                    "rotate|170|-170|100",
                    "absMaxI|5|-10",
                    "absMaxI|-2147483648|0",
                    "absMaxI|-2147483648|5",
                    "absMaxF|-3.5|2",
                    "absMaxD|-3.5|2",
                    "sign|0",
                    "sign|5",
                    "sign|-5",
                    "sign|0.0001",
                    "sign|-0.0001");

    private MthDegreesOracle() {}

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
                "usage: MthDegreesOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 按 `%.9g` 格式化，去尾零，特殊值单独处理。 */
    private static String formatNumber(double value) {
        if (Double.isNaN(value)) {
            return "NaN";
        }
        if (Double.isInfinite(value)) {
            return value > 0 ? "Infinity" : "-Infinity";
        }
        String text = String.format(Locale.ROOT, "%.9g", value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            int end = text.length();
            while (end > 0 && text.charAt(end - 1) == '0') {
                end--;
            }
            if (end > 0 && text.charAt(end - 1) == '.') {
                end--;
            }
            text = text.substring(0, end);
        }
        return text;
    }

    private static float parseF(String text) {
        return (float) Double.parseDouble(text);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_degrees: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "diff" -> out.addProperty("value", formatNumber(
                    Mth.degreesDifference(parseF(parts[1]), parseF(parts[2]))));
            case "diffAbs" -> out.addProperty("value", formatNumber(
                    Mth.degreesDifferenceAbs(parseF(parts[1]), parseF(parts[2]))));
            case "approachDeg" -> out.addProperty("value", formatNumber(Mth.approachDegrees(
                    parseF(parts[1]), parseF(parts[2]), parseF(parts[3]))));
            case "rotate" -> out.addProperty("value", formatNumber(Mth.rotateIfNecessary(
                    parseF(parts[1]), parseF(parts[2]), parseF(parts[3]))));
            case "absMaxI" -> out.addProperty("value", Mth.absMax(
                    Integer.parseInt(parts[1]), Integer.parseInt(parts[2])));
            case "absMaxF" -> out.addProperty("value",
                    formatNumber(Mth.absMax(parseF(parts[1]), parseF(parts[2]))));
            case "absMaxD" -> out.addProperty("value", formatNumber(Mth.absMax(
                    Double.parseDouble(parts[1]), Double.parseDouble(parts[2]))));
            case "sign" -> out.addProperty("value", Mth.sign(Double.parseDouble(parts[1])));
            default -> throw new IllegalArgumentException("mth_degrees: 无法识别的单元: " + unit);
        }

        return out;
    }
}