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
 * {@code Mth.packDegrees} / {@code Mth.unpackDegrees} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthPackDegreesOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#packDegrees(float)} 与 {@link Mth#unpackDegrees(byte)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>`pack` 输出有符号十进制整数（`-128..127`）；`unpack` / `round` 输出按 `%.9g` 格式化的
 * `float` 字符串。
 *
 * <p>单元表必须与 probes/mth_pack_degrees.cpp 逐项一致。
 */
public final class MthPackDegreesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "pack|0",
                    "pack|90",
                    "pack|179",
                    "pack|180",
                    "pack|-179",
                    "pack|-180",
                    "pack|181",
                    "pack|360",
                    "pack|-360",
                    "pack|270",
                    "pack|1.40625",
                    "pack|2.8125",
                    "pack|-1.40625",
                    "pack|-0.5",
                    "pack|-2.8125",
                    "pack|0.5",
                    "unpack|0",
                    "unpack|1",
                    "unpack|127",
                    "unpack|-128",
                    "unpack|-1",
                    "unpack|64",
                    "round|0",
                    "round|90",
                    "round|179",
                    "round|180",
                    "round|-180",
                    "round|100",
                    "round|-100");

    private MthPackDegreesOracle() {}

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
        System.err.println("usage: MthPackDegreesOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 2) {
            throw new IllegalArgumentException("mth_pack_degrees: 单元需要 2 段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "pack" -> out.addProperty("value", (int) Mth.packDegrees(parseF(parts[1])));
            case "unpack" -> out.addProperty("value",
                    formatNumber(Mth.unpackDegrees((byte) Integer.parseInt(parts[1]))));
            case "round" -> out.addProperty("value",
                    formatNumber(Mth.unpackDegrees(Mth.packDegrees(parseF(parts[1])))));
            default -> throw new IllegalArgumentException(
                    "mth_pack_degrees: 无法识别的单元: " + unit);
        }

        return out;
    }
}