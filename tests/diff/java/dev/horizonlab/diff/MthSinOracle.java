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
 * {@code Mth.sin} / {@code Mth.cos} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthSinOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#sin(double)} 与 {@link Mth#cos(double)}。表项单元通过反射读取
 * 原版私有的 {@code SIN} 表，用于定位「表构造本身」的差异（{@code Math.sin} 与 C 的
 * {@code std::sin} 在末位可能不同）。
 *
 * <p>单元表必须与 probes/mth_sin.cpp 逐项一致。
 */
public final class MthSinOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版 {@code Mth.SIN_SCALE}。 */
    private static final double SIN_SCALE = 10430.378350470453;

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "tab|0",
                    "tab|1",
                    "tab|16384",
                    "tab|32768",
                    "tab|49152",
                    "tab|65535",
                    "tab|100",
                    "tab|1000",
                    "tab|10000",
                    "tab|20000",
                    "tab|30000",
                    "tab|40000",
                    "tab|50000",
                    "tab|60000",
                    "0",
                    "1.5707963267948966",
                    "3.141592653589793",
                    "4.71238898038469",
                    "6.283185307179586",
                    "0.1",
                    "-0.1",
                    "-1.5707963267948966",
                    "-3.141592653589793",
                    "12.566370614359172",
                    "-12.566370614359172",
                    "100",
                    "-100",
                    "0.00001",
                    "0.0001",
                    "0.0002",
                    "0.7853981633974483",
                    "0.5235987755982988",
                    "1.0471975511965976");

    private MthSinOracle() {}

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
                "usage: MthSinOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 反射读取原版私有的 {@code SIN} 表。 */
    private static float[] sinTable() {
        try {
            java.lang.reflect.Field field = Mth.class.getDeclaredField("SIN");
            field.setAccessible(true);
            return (float[]) field.get(null);
        } catch (ReflectiveOperationException ex) {
            throw new IllegalStateException("无法读取 Mth.SIN", ex);
        }
    }

    private static JsonObject summarise(String unit) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (unit.length() > 4 && unit.startsWith("tab|")) {
            int index = Integer.parseInt(unit.substring(4));
            float[] table = sinTable();
            if (index < 0 || index >= table.length) {
                throw new IllegalArgumentException("mth_sin: 表下标越界: " + unit);
            }
            result.addProperty("kind", "tab");
            result.addProperty("value", formatNumber(table[index]));
            return result;
        }

        double angle = Double.parseDouble(unit);
        result.addProperty("kind", "angle");
        result.addProperty("sin", formatNumber(Mth.sin(angle)));
        result.addProperty("cos", formatNumber(Mth.cos(angle)));
        return result;
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