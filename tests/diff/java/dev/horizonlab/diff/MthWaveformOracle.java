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
 * {@code Mth} 波形与缓动函数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthWaveformOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#triangleWave(float, float)} / {@link Mth#smoothstep(double)} /
 * {@link Mth#smoothstepDerivative(double)} / {@link Mth#square(float)} / {@link Mth#cube(float)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>浮点按 `%.9g` 格式化后去掉尾随零；`NaN` / `Infinity` 以字符串形式给出。
 *
 * <p>单元表必须与 probes/mth_waveform.cpp 逐项一致。
 */
public final class MthWaveformOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "tri|0|10",
                    "tri|2.5|10",
                    "tri|5|10",
                    "tri|7.5|10",
                    "tri|10|10",
                    "tri|-3|10",
                    "tri|-5|10",
                    "tri|-10|10",
                    "tri|1|4",
                    "tri|3|4",
                    "smooth|0",
                    "smooth|1",
                    "smooth|0.5",
                    "smooth|0.25",
                    "smooth|-0.5",
                    "smooth|2",
                    "smoothD|0",
                    "smoothD|1",
                    "smoothD|0.5",
                    "smoothD|0.25",
                    "square|3",
                    "square|-3",
                    "square|0.5",
                    "cube|3",
                    "cube|-3",
                    "cube|0.5");

    private MthWaveformOracle() {}

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
                "usage: MthWaveformOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            throw new IllegalArgumentException("mth_waveform: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "tri" -> out.addProperty("value", formatNumber(
                    Mth.triangleWave(parseF(parts[1]), parseF(parts[2]))));
            case "smooth" -> out.addProperty("value",
                    formatNumber(Mth.smoothstep(Double.parseDouble(parts[1]))));
            case "smoothD" -> out.addProperty("value",
                    formatNumber(Mth.smoothstepDerivative(Double.parseDouble(parts[1]))));
            case "square" -> out.addProperty("value", formatNumber(Mth.square(parseF(parts[1]))));
            case "cube" -> out.addProperty("value", formatNumber(Mth.cube(parseF(parts[1]))));
            default -> throw new IllegalArgumentException("mth_waveform: 无法识别的单元: " + unit);
        }

        return out;
    }
}