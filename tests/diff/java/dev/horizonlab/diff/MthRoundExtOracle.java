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
import net.minecraft.util.Mth;

/**
 * Mth.ceilLong 与 Mth.map 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthRoundExtOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 本能力不需要注册表。
 *
 * 单元表必须与 probes/mth_round_ext.cpp 逐项一致。
 */
public final class MthRoundExtOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final List<String> UNITS =
            List.of(
                    "ceillong|2.1",
                    "ceillong|2.0",
                    "ceillong|2.9",
                    "ceillong|0.1",
                    "ceillong|0.0",
                    "ceillong|0.9",
                    "ceillong|-2.1",
                    "ceillong|-2.0",
                    "ceillong|-2.9",
                    "ceillong|-0.1",
                    "ceillong|-0.0",
                    "ceillong|-0.9",
                    "ceillong|1000000.5",
                    "ceillong|-1000000.5",
                    "ceillong|1e15",
                    "map|0|0|10|0|100",
                    "map|5|0|10|0|100",
                    "map|10|0|10|0|100",
                    "map|-5|0|10|0|100",
                    "map|15|0|10|0|100",
                    "map|5|0|10|100|0",
                    "map|0|10|0|100|0",
                    "map|-5|-10|10|0|100",
                    "map|0|-10|10|-50|50",
                    "map|5|5|5|0|100");

    private MthRoundExtOracle() {}

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
                "usage: MthRoundExtOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static String formatDouble(double value) {
        if (Double.isNaN(value)) {
            return "NaN";
        }
        if (Double.isInfinite(value)) {
            return value > 0 ? "Infinity" : "-Infinity";
        }
        // Java 的 %.9g 会保留尾随零（25 输出成 25.0000000），而 C++ 的 %g 会去掉。
        // 两边必须统一：这里按 C++ 的语义去掉尾随零，指数部分不动。
        String text = String.format(java.util.Locale.ROOT, "%.9g", value);
        int exponentIndex = text.indexOf('e');
        String mantissa = exponentIndex >= 0 ? text.substring(0, exponentIndex) : text;
        String exponent = exponentIndex >= 0 ? text.substring(exponentIndex) : "";
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
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_round_ext: unit needs at least two fields: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String mode = parts[0];

        if (mode.equals("ceillong")) {
            out.addProperty("value", Long.toString(Mth.ceilLong(Double.parseDouble(parts[1]))));
            return out;
        }

        if (mode.equals("map")) {
            if (parts.length != 6) {
                throw new IllegalArgumentException("mth_round_ext: map needs five numbers: " + unit);
            }
            double result = Mth.map(
                    Double.parseDouble(parts[1]),
                    Double.parseDouble(parts[2]),
                    Double.parseDouble(parts[3]),
                    Double.parseDouble(parts[4]),
                    Double.parseDouble(parts[5]));
            out.addProperty("value", formatDouble(result));
            return out;
        }

        throw new IllegalArgumentException("mth_round_ext: unrecognized unit: " + unit);
    }
}