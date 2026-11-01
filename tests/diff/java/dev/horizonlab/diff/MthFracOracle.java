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
 * {@code Mth.frac} 两个重载的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthFracOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#frac(float)} / {@link Mth#frac(double)} /
 * {@link Mth#floor(float)} / {@link Mth#lfloor(double)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>`floorF` 输出 `int`，`floorD` 输出 `long`（按字符串）；`fracF` / `fracD` 按 `%.9g` 格式化。
 *
 * <p>单元表必须与 probes/mth_frac.cpp 逐项一致。
 */
public final class MthFracOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "fracF|0",
                    "fracF|1.5",
                    "fracF|1.25",
                    "fracF|0.75",
                    "fracF|2",
                    "fracF|-1.5",
                    "fracF|-1.25",
                    "fracF|-2",
                    "fracF|-0.25",
                    "fracF|1e10",
                    "fracF|3e9",
                    "fracF|-3e9",
                    "fracF|16777216",
                    "fracF|16777217",
                    "fracF|8388608.5",
                    "fracD|1.5",
                    "fracD|-1.5",
                    "fracD|1e10",
                    "fracD|1e300",
                    "floorF|1.5",
                    "floorF|-1.5",
                    "floorF|1e10",
                    "floorF|-1e10",
                    "floorD|1.5",
                    "floorD|-1.5",
                    "floorD|1e10",
                    "floorD|-1e300");

    private MthFracOracle() {}

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
                "usage: MthFracOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        // 去掉小数部分的尾零，同时处理指数形式（如 1.00000000e+300 -> 1e+300）。
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
            throw new IllegalArgumentException("mth_frac: 单元需要 2 段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "fracF" -> out.addProperty("value",
                    formatNumber(Mth.frac((float) Double.parseDouble(parts[1]))));
            case "fracD" -> out.addProperty("value",
                    formatNumber(Mth.frac(Double.parseDouble(parts[1]))));
            case "floorF" -> out.addProperty("value",
                    Mth.floor((float) Double.parseDouble(parts[1])));
            case "floorD" -> out.addProperty("value",
                    Long.toString(Mth.lfloor(Double.parseDouble(parts[1]))));
            default -> throw new IllegalArgumentException("mth_frac: 无法识别的单元: " + unit);
        }

        return out;
    }
}