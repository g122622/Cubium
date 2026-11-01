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
 * {@code Mth.inverseLerp} 与 {@code Mth.lerp3} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthInverseLerpOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#inverseLerp(double, double, double)} /
 * {@link Mth#inverseLerp(float, float, float)} / {@link Mth#lerp3}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>浮点按 `%.9g` 格式化后去掉尾随零；`NaN` 与 `Infinity` 分别输出为字符串。
 *
 * <p>单元表必须与 probes/mth_inverse_lerp.cpp 逐项一致。
 */
public final class MthInverseLerpOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "invD|5|0|10",
                    "invD|0|0|10",
                    "invD|10|0|10",
                    "invD|2.5|0|10",
                    "invD|-5|0|10",
                    "invD|15|0|10",
                    "invD|5|10|0",
                    "invD|2.5|10|0",
                    "invD|0|0|0",
                    "invD|5|0|0",
                    "invD|-5|0|0",
                    "invF|5|0|10",
                    "invF|1|0|3",
                    "invF|2|0|3",
                    "invF|0|0|0",
                    "invF|5|0|0",
                    "lerp3|0|0|0|0|0|0|10|10|10|10|0",
                    "lerp3|0|1|0|0|0|0|10|10|10|10|0",
                    "lerp3|1|0|0|0|0|0|10|10|10|10|0",
                    "lerp3|1|1|0|0|0|0|10|10|10|10|0",
                    "lerp3|0.5|0.5|0|0|0|0|10|10|10|10|0",
                    "lerp3|0.5|0.5|0|0|0|0|10|10|10|10|999");

    private MthInverseLerpOracle() {}

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
                "usage: MthInverseLerpOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_inverse_lerp: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "invD" -> out.addProperty("value", formatNumber(Mth.inverseLerp(
                    Double.parseDouble(parts[1]),
                    Double.parseDouble(parts[2]),
                    Double.parseDouble(parts[3]))));
            case "invF" -> out.addProperty("value", formatNumber(Mth.inverseLerp(
                    (float) Double.parseDouble(parts[1]),
                    (float) Double.parseDouble(parts[2]),
                    (float) Double.parseDouble(parts[3]))));
            case "lerp3" -> {
                if (parts.length != 12) {
                    throw new IllegalArgumentException(
                            "mth_inverse_lerp: lerp3 需要 11 个参数（加上模式共 12 段）: " + unit);
                }
                double[] d = new double[11];
                for (int i = 0; i < 11; i++) {
                    d[i] = Double.parseDouble(parts[i + 1]);
                }
                out.addProperty("value", formatNumber(Mth.lerp3(
                        d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10])));
            }
            default -> throw new IllegalArgumentException(
                    "mth_inverse_lerp: 无法识别的单元: " + unit);
        }

        return out;
    }
}
