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
 * {@code Mth} 旋转插值函数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthRotOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#rotLerp(double, double, double)} /
 * {@link Mth#rotLerp(float, float, float)} / {@link Mth#rotLerpRad(float, float, float)} /
 * {@link Mth#triangleWave(float, float)}，不重写公式。
 *
 * <p>单元表必须与 probes/mth_rot.cpp 逐项一致。
 */
public final class MthRotOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "rotlerpd|0.5|0|90",
                    "rotlerpd|0|0|90",
                    "rotlerpd|1|0|90",
                    "rotlerpd|0.5|0|180",
                    "rotlerpd|0.5|350|10",
                    "rotlerpd|0.5|10|350",
                    "rotlerpd|1|350|10",
                    "rotlerpd|0|350|10",
                    "rotlerpd|0.5|180|0",
                    "rotlerpd|0.5|0|730",
                    "rotlerpd|0.5|0|-730",
                    "rotlerpf|0.5|0|90",
                    "rotlerpf|0.5|350|10",
                    "rotlerpf|0.25|0|45",
                    "rotlerprad|0.5|0|1.5707963",
                    "rotlerprad|0|0|1.5707963",
                    "rotlerprad|1|0|1.5707963",
                    "rotlerprad|0.5|3.0|-3.0",
                    "rotlerprad|0.5|-3.0|3.0",
                    "rotlerprad|0.5|0|12.566370",
                    "rotlerprad|0.5|0|-12.566370",
                    "triwave|0|1",
                    "triwave|0.25|1",
                    "triwave|0.5|1",
                    "triwave|0.75|1",
                    "triwave|1|1",
                    "triwave|0|10",
                    "triwave|2.5|10",
                    "triwave|5|10",
                    "triwave|7.5|10",
                    "triwave|-0.25|1",
                    "triwave|-0.5|1",
                    "triwave|-2.5|10");

    private MthRotOracle() {}

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
                "usage: MthRotOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            throw new IllegalArgumentException("mth_rot: 单元至少两段: " + unit);
        }

        String mode = parts[0];
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (mode.equals("rotlerpd") && parts.length == 4) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.rotLerp(
                            Double.parseDouble(parts[1]),
                            Double.parseDouble(parts[2]),
                            Double.parseDouble(parts[3]))));
            return result;
        }
        if (mode.equals("rotlerpf") && parts.length == 4) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.rotLerp(
                            Float.parseFloat(parts[1]),
                            Float.parseFloat(parts[2]),
                            Float.parseFloat(parts[3]))));
            return result;
        }
        if (mode.equals("rotlerprad") && parts.length == 4) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.rotLerpRad(
                            Float.parseFloat(parts[1]),
                            Float.parseFloat(parts[2]),
                            Float.parseFloat(parts[3]))));
            return result;
        }
        if (mode.equals("triwave") && parts.length == 3) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.triangleWave(
                            Float.parseFloat(parts[1]), Float.parseFloat(parts[2]))));
            return result;
        }

        throw new IllegalArgumentException("mth_rot: 无法识别的单元: " + unit);
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