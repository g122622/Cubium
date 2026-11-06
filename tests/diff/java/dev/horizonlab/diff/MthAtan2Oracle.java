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
 * {@code Mth.atan2} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthAtan2Oracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#atan2(double, double)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>输出的是结果的 {@code double} 原始位模式（{@code 0x%016x}）。不能按十进制比较：
 * {@code Mth.atan2} 与 {@code Math.atan2} 相差 1e-6~1e-8，用 {@code %.9g} 会把部分结果
 * 打印成同一个字符串，从而漏掉差异。
 *
 * <p>单元表必须与 probes/mth_atan2.cpp 逐项一致。
 */
@SuppressWarnings("deprecation")
public final class MthAtan2Oracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "1|0",
                    "-1|0",
                    "0|1",
                    "0|-1",
                    "1|1",
                    "-1|-1",
                    "2|1",
                    "1|2",
                    "-2|1",
                    "1|-2",
                    "1|-1",
                    "-1|1",
                    "2|-1",
                    "-2|-1",
                    "-1|-2",
                    "-1|2",
                    "0.1|1",
                    "1|0.1",
                    "0.001|1",
                    "1|0.001",
                    "1|4",
                    "4|1",
                    "3|4",
                    "0.75|1",
                    "1e-8|1",
                    "1|1e-8",
                    "1e8|1",
                    "1|1e8",
                    "1e-150|1",
                    "1e150|1",
                    "0|0",
                    "nan|1",
                    "1|nan");

    private MthAtan2Oracle() {}

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
                "usage: MthAtan2Oracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 解析分量：接受十进制字面量与 {@code nan} / {@code inf} / {@code -inf}。 */
    private static double parseComponent(String text) {
        return switch (text) {
            case "nan", "NaN" -> Double.NaN;
            case "inf", "Infinity" -> Double.POSITIVE_INFINITY;
            case "-inf", "-Infinity" -> Double.NEGATIVE_INFINITY;
            default -> Double.parseDouble(text);
        };
    }

    /** 把 {@code double} 的原始位模式格式化成 {@code 0x%016x}。 */
    private static String bitsOf(double value) {
        return String.format(java.util.Locale.ROOT, "0x%016x", Double.doubleToRawLongBits(value));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 2) {
            throw new IllegalArgumentException("mth_atan2: unit needs two fields 'y|x': " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        out.addProperty("bits", bitsOf(Mth.atan2(parseComponent(parts[0]), parseComponent(parts[1]))));
        return out;
    }
}
