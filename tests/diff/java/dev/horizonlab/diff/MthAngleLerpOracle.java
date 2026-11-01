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
 * {@code Mth} 角度插值与 Catmull-Rom 样条的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthAngleLerpOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#wrapDegrees(float)} / {@link Mth#wrapDegrees(int)} /
 * {@link Mth#wrapDegrees(double)} / {@link Mth#rotLerp(float, float, float)} /
 * {@link Mth#rotLerpRad(float, float, float)} / {@link Mth#catmullrom}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>浮点按 `%.9g` 格式化后去掉尾随零；`NaN` / `Infinity` 以字符串形式给出。
 *
 * <p>单元表必须与 probes/mth_angle_lerp.cpp 逐项一致。
 */
public final class MthAngleLerpOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "wrap|0",
                    "wrap|90",
                    "wrap|179",
                    "wrap|180",
                    "wrap|-180",
                    "wrap|181",
                    "wrap|360",
                    "wrap|-360",
                    "wrap|540",
                    "wrap|-540",
                    "wrapInt|180",
                    "wrapInt|-180",
                    "rotLerp|0|0|90",
                    "rotLerp|1|0|90",
                    "rotLerp|0.5|0|90",
                    "rotLerp|0.5|170|-170",
                    "rotLerp|0.5|-170|170",
                    "rotLerp|0|170|-170",
                    "rotLerp|1|170|-170",
                    "rotLerp|1.5|0|90",
                    "rotLerp|-0.5|0|90",
                    "rotLerpRad|0.5|0|1.5707964",
                    "rotLerpRad|0.5|3.0|-3.0",
                    "rotLerpRad|0.5|-3.0|3.0",
                    "rotLerpRad|0|3.0|-3.0",
                    "catmull|0|1|2|3|4",
                    "catmull|1|1|2|3|4",
                    "catmull|0.5|1|2|3|4",
                    "catmull|0.5|0|1|2|3",
                    "catmull|0.25|0|1|2|3",
                    "catmull|0.5|0|0|1|1",
                    "catmull|0.25|0|0|1|1",
                    "catmull|1.5|1|2|3|4",
                    "catmull|-0.5|1|2|3|4");

    private MthAngleLerpOracle() {}

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
                "usage: MthAngleLerpOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            throw new IllegalArgumentException("mth_angle_lerp: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "wrap" -> out.addProperty("value",
                    formatNumber(Mth.wrapDegrees(parseF(parts[1]))));
            case "wrapInt" -> out.addProperty("value",
                    formatNumber(Mth.wrapDegrees(Integer.parseInt(parts[1]))));
            case "rotLerp" -> out.addProperty("value", formatNumber(
                    Mth.rotLerp(parseF(parts[1]), parseF(parts[2]), parseF(parts[3]))));
            case "rotLerpRad" -> out.addProperty("value", formatNumber(
                    Mth.rotLerpRad(parseF(parts[1]), parseF(parts[2]), parseF(parts[3]))));
            case "catmull" -> out.addProperty("value", formatNumber(Mth.catmullrom(
                    parseF(parts[1]), parseF(parts[2]), parseF(parts[3]),
                    parseF(parts[4]), parseF(parts[5]))));
            default -> throw new IllegalArgumentException(
                    "mth_angle_lerp: 无法识别的单元: " + unit);
        }

        return out;
    }
}