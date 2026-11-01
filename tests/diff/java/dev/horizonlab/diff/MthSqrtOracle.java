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
 * {@code Mth.fastInvSqrt} 与 {@code Mth.sqrt} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthSqrtOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#fastInvSqrt(double)} 与 {@link Mth#sqrt(float)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>全部按 `%.9g` 格式化后去尾零（尾数与指数分开处理，否则 `1e+100` 这类值会报假差异）。
 *
 * <p>单元表必须与 probes/mth_sqrt.cpp 逐项一致。
 */
@SuppressWarnings("deprecation")
public final class MthSqrtOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "fast|1",
                    "fast|4",
                    "fast|9",
                    "fast|16",
                    "fast|100",
                    "fast|2",
                    "fast|3",
                    "fast|10",
                    "fast|0.5",
                    "fast|0.25",
                    "fast|0",
                    "fast|-1",
                    "fast|-4",
                    "fast|1e100",
                    "fast|1e-100",
                    "sqrtF|0",
                    "sqrtF|1",
                    "sqrtF|2",
                    "sqrtF|4",
                    "sqrtF|9",
                    "sqrtF|0.25",
                    "sqrtF|-1",
                    "sqrtF|1e-40",
                    "sqrtF|3.4028235e38");

    private MthSqrtOracle() {}

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
                "usage: MthSqrtOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 按 `%.9g` 格式化，去尾零（尾数与指数分开处理），特殊值单独处理。 */
    private static String formatNumber(double value) {
        if (Double.isNaN(value)) {
            return "NaN";
        }
        if (Double.isInfinite(value)) {
            return value > 0 ? "Infinity" : "-Infinity";
        }
        String text = String.format(Locale.ROOT, "%.9g", value);

        int exponentAt = text.indexOf('e');
        if (exponentAt < 0) {
            exponentAt = text.indexOf('E');
        }
        String mantissa = exponentAt < 0 ? text : text.substring(0, exponentAt);
        String exponent = exponentAt < 0 ? "" : text.substring(exponentAt);

        if (mantissa.indexOf('.') >= 0) {
            int end = mantissa.length();
            while (end > 0 && mantissa.charAt(end - 1) == '0') {
                end--;
            }
            if (end > 0 && mantissa.charAt(end - 1) == '.') {
                end--;
            }
            mantissa = mantissa.substring(0, end);
        }
        return mantissa + exponent;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 2) {
            throw new IllegalArgumentException("mth_sqrt: 单元需要 2 段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "fast" -> out.addProperty("value",
                    formatNumber(Mth.fastInvSqrt(Double.parseDouble(parts[1]))));
            case "sqrtF" -> out.addProperty("value",
                    formatNumber(Mth.sqrt((float) Double.parseDouble(parts[1]))));
            default -> throw new IllegalArgumentException("mth_sqrt: 无法识别的单元: " + unit);
        }

        return out;
    }
}