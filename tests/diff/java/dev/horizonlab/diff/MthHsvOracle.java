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
 * {@code Mth.hsvToRgb} / {@code Mth.hsvToArgb} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthHsvOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#hsvToRgb(float, float, float)} 与
 * {@link Mth#hsvToArgb(float, float, float, int)}。
 *
 * <p>输出是**有符号 32 位整数**（与 Java `int` 一致）。`hsvToRgb` 的 alpha 恒为 0，
 * 因此结果非负；`hsvToArgb` 传入 `alpha = 255` 时结果为负数。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/mth_hsv.cpp 逐项一致。
 */
public final class MthHsvOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "rgb|0|1|1",
                    "rgb|0.1666667|1|1",
                    "rgb|0.3333333|1|1",
                    "rgb|0.5|1|1",
                    "rgb|0.6666667|1|1",
                    "rgb|0.8333333|1|1",
                    "rgb|1|1|1",
                    "rgb|0|0|1",
                    "rgb|0|0|0.5",
                    "rgb|0|0|0",
                    "rgb|0|0.5|1",
                    "rgb|0.25|1|1",
                    "rgb|0.75|1|1",
                    "argb|0|1|1|255",
                    "argb|0|1|1|128",
                    "argb|0|0|0|0",
                    "argb|0.5|1|1|255");

    private MthHsvOracle() {}

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
                "usage: MthHsvOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static float parseF(String text) {
        return (float) Double.parseDouble(text);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_hsv: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "rgb" -> out.addProperty("value", Mth.hsvToRgb(
                    parseF(parts[1]), parseF(parts[2]), parseF(parts[3])));
            case "argb" -> out.addProperty("value", Mth.hsvToArgb(
                    parseF(parts[1]), parseF(parts[2]), parseF(parts[3]),
                    Integer.parseInt(parts[4])));
            default -> throw new IllegalArgumentException("mth_hsv: 无法识别的单元: " + unit);
        }

        return out;
    }
}