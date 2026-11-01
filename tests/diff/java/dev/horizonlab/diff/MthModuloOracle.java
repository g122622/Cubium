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
 * {@code Mth.positiveModulo} 与 {@code Mth.wrapDegrees} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthModuloOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#positiveModulo(int, int)} / {@link Mth#positiveModulo(float, float)}
 * / {@link Mth#positiveModulo(double, double)} / {@link Mth#wrapDegrees(int)} /
 * {@link Mth#wrapDegrees(float)} / {@link Mth#wrapDegrees(double)}，不重写公式。
 *
 * <p>单元表必须与 probes/mth_modulo.cpp 逐项一致。
 */
public final class MthModuloOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "modi|7|3",
                    "modi|-7|3",
                    "modi|7|-3",
                    "modi|-7|-3",
                    "modi|0|5",
                    "modi|-1|5",
                    "modi|-6|3",
                    "modi|6|3",
                    "modf|7.5|3",
                    "modf|-7.5|3",
                    "modf|-1|5",
                    "modf|0|5",
                    "modd|7.5|3",
                    "modd|-7.5|3",
                    "modd|-1|5",
                    "modd|0|5",
                    "wrap|0",
                    "wrap|180",
                    "wrap|-180",
                    "wrap|179.9",
                    "wrap|-179.9",
                    "wrap|359",
                    "wrap|-359",
                    "wrap|720",
                    "wrap|-720",
                    "wrap|900",
                    "wrap|-900",
                    "wrap|1000000",
                    "wrap|-1000000",
                    "wrap|45.5",
                    "wrap|-45.5");

    private MthModuloOracle() {}

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
                "usage: MthModuloOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|");
        if (parts.length == 0) {
            throw new IllegalArgumentException("mth_modulo: 空单元");
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (parts[0].equals("modi") && parts.length == 3) {
            result.addProperty("mode", "modi");
            result.addProperty(
                    "result",
                    String.valueOf(Mth.positiveModulo(
                            Integer.parseInt(parts[1]), Integer.parseInt(parts[2]))));
            return result;
        }
        if (parts[0].equals("modf") && parts.length == 3) {
            result.addProperty("mode", "modf");
            result.addProperty(
                    "result",
                    formatFloat(Mth.positiveModulo(
                            Float.parseFloat(parts[1]), Float.parseFloat(parts[2]))));
            return result;
        }
        if (parts[0].equals("modd") && parts.length == 3) {
            result.addProperty("mode", "modd");
            result.addProperty(
                    "result",
                    formatFloat(Mth.positiveModulo(
                            Double.parseDouble(parts[1]), Double.parseDouble(parts[2]))));
            return result;
        }
        if (parts[0].equals("wrap") && parts.length == 2) {
            double value = Double.parseDouble(parts[1]);
            result.addProperty("mode", "wrap");
            result.addProperty("resultD", formatFloat(Mth.wrapDegrees(value)));
            result.addProperty("resultF", formatFloat(Mth.wrapDegrees((float) value)));
            result.addProperty("resultI", Mth.wrapDegrees((int) value));
            return result;
        }

        throw new IllegalArgumentException("mth_modulo: 无法识别的单元: " + unit);
    }

    private static String formatFloat(double value) {
        String text = String.format("%.6g", value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }
}