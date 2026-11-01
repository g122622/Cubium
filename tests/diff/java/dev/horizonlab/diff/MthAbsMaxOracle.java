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
import java.util.List;
import net.minecraft.util.Mth;

/**
 * {@code Mth.absMax} / {@code chessboardDistance} / {@code smallestSquareSide} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthAbsMaxOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#absMax(int, int)} / {@link Mth#absMax(float, float)} /
 * {@link Mth#absMax(double, double)} / {@link Mth#chessboardDistance(int, int, int, int)} /
 * {@link Mth#smallestSquareSide(int)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/mth_absmax.cpp 逐项一致。
 */
public final class MthAbsMaxOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "absMaxInt|3|-5",
                    "absMaxInt|-3|-5",
                    "absMaxInt|0|0",
                    "absMaxInt|7|0",
                    "absMaxInt|-2147483648|5",
                    "absMaxInt|-2147483648|-2147483648",
                    "absMaxInt|-2147483648|0",
                    "absMaxFloat|3.5|-5.25",
                    "absMaxFloat|-1.5|-2",
                    "absMaxFloat|0|-0",
                    "absMaxDouble|3.5|-5.25",
                    "absMaxDouble|-1e10|1e9",
                    "chess|0|0|3|4",
                    "chess|0|0|4|3",
                    "chess|5|5|5|5",
                    "chess|-2|-3|1|1",
                    "chess|0|0|2147483647|-1",
                    "square|0",
                    "square|1",
                    "square|2",
                    "square|3",
                    "square|4",
                    "square|5",
                    "square|9",
                    "square|10",
                    "square|16",
                    "square|17");

    private MthAbsMaxOracle() {}

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
                "usage: MthAbsMaxOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 与探针一致的浮点格式化：9 位有效数字、去尾随零。 */
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

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_absmax: 单元段数不足: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String value;
        switch (parts[0]) {
            case "absMaxInt" -> value = String.valueOf(Mth.absMax(
                    Integer.parseInt(parts[1]), Integer.parseInt(parts[2])));
            case "absMaxFloat" -> value = formatNumber(Mth.absMax(
                    Float.parseFloat(parts[1]), Float.parseFloat(parts[2])));
            case "absMaxDouble" -> value = formatNumber(Mth.absMax(
                    Double.parseDouble(parts[1]), Double.parseDouble(parts[2])));
            case "chess" -> value = String.valueOf(Mth.chessboardDistance(
                    Integer.parseInt(parts[1]), Integer.parseInt(parts[2]),
                    Integer.parseInt(parts[3]), Integer.parseInt(parts[4])));
            case "square" -> {
                try {
                    value = String.valueOf(Mth.smallestSquareSide(Integer.parseInt(parts[1])));
                } catch (IllegalArgumentException exception) {
                    value = "!IllegalArgumentException";
                }
            }
            default -> throw new IllegalArgumentException("mth_absmax: 无法识别的单元: " + unit);
        }

        out.addProperty("value", value);
        return out;
    }
}