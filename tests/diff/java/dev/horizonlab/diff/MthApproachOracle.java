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
 * {@code Mth.approach} / {@code approachDegrees} / {@code clamp} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthApproachOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#approach(float, float, float)} /
 * {@link Mth#approachDegrees(float, float, float)} / {@link Mth#degreesDifference(float, float)} /
 * {@link Mth#clamp(float, float, float)}，不重写公式。
 *
 * <p>单元表必须与 probes/mth_approach.cpp 逐项一致。
 */
public final class MthApproachOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "approach|0|100|10",
                    "approach|95|100|10",
                    "approach|100|100|10",
                    "approach|100|0|10",
                    "approach|5|0|10",
                    "approach|0|100|-10",
                    "approach|100|0|-10",
                    "approach|0|100|0",
                    "approach|50|100|0",
                    "approach|0|100|1000",
                    "approach|0|100|100",
                    "approach|50|50|10",
                    "clamp|5|0|10",
                    "clamp|-5|0|10",
                    "clamp|15|0|10",
                    "clamp|0|0|10",
                    "clamp|10|0|10",
                    "clamp|5|10|0",
                    "clamp|15|10|0",
                    "clamp|0|10|0",
                    "degdiff|0|90",
                    "degdiff|0|180",
                    "degdiff|0|270",
                    "degdiff|350|10",
                    "degdiff|10|350",
                    "degdiff|0|540",
                    "appdeg|0|90|10",
                    "appdeg|85|90|10",
                    "appdeg|90|90|10",
                    "appdeg|350|10|5",
                    "appdeg|10|350|5",
                    "appdeg|0|90|1000",
                    "appdeg|350|10|100");

    private MthApproachOracle() {}

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
                "usage: MthApproachOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_approach: 单元至少两段: " + unit);
        }

        String mode = parts[0];
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (mode.equals("approach") && parts.length == 4) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.approach(
                            Float.parseFloat(parts[1]),
                            Float.parseFloat(parts[2]),
                            Float.parseFloat(parts[3]))));
            return result;
        }
        if (mode.equals("clamp") && parts.length == 4) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.clamp(
                            Float.parseFloat(parts[1]),
                            Float.parseFloat(parts[2]),
                            Float.parseFloat(parts[3]))));
            return result;
        }
        if (mode.equals("degdiff") && parts.length == 3) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.degreesDifference(
                            Float.parseFloat(parts[1]), Float.parseFloat(parts[2]))));
            return result;
        }
        if (mode.equals("appdeg") && parts.length == 4) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.approachDegrees(
                            Float.parseFloat(parts[1]),
                            Float.parseFloat(parts[2]),
                            Float.parseFloat(parts[3]))));
            return result;
        }

        throw new IllegalArgumentException("mth_approach: 无法识别的单元: " + unit);
    }

    private static String formatNumber(double value) {
        String text = String.format("%.9g", value);
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