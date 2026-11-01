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
import net.minecraft.util.ARGB;
import net.minecraft.util.Mth;

/**
 * {@code Mth.hsvToArgb} 与 {@code Mth.murmurHash3Mixer} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthColorOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#hsvToRgb(float, float, float)} /
 * {@link Mth#hsvToArgb(float, float, float, int)} / {@link Mth#murmurHash3Mixer(int)}，
 * 通道分解用 {@link ARGB#alpha} / {@link ARGB#red} / {@link ARGB#green} / {@link ARGB#blue}。
 *
 * <p>单元表必须与 probes/mth_color.cpp 逐项一致。
 */
public final class MthColorOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "hsv|0|1|1",
                    "hsv|0.166667|1|1",
                    "hsv|0.333333|1|1",
                    "hsv|0.5|1|1",
                    "hsv|0.666667|1|1",
                    "hsv|0.833333|1|1",
                    "hsv|1|1|1",
                    "hsv|0|0|1",
                    "hsv|0|1|0",
                    "hsv|0|0|0",
                    "hsv|0.5|0|0.5",
                    "hsv|0.25|0.5|0.8",
                    "hsv|0.75|0.3|0.6",
                    "hsv|0|1|0.999",
                    "hsv|0|0.999|1",
                    "hsva|0|1|1|255",
                    "hsva|0.333333|1|1|128",
                    "hsva|0.5|1|1|0",
                    "murmur|0",
                    "murmur|1",
                    "murmur|-1",
                    "murmur|42",
                    "murmur|-2147483648",
                    "murmur|2147483647",
                    "murmur|100",
                    "murmur|101",
                    "murmur|102");

    private MthColorOracle() {}

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
                "usage: MthColorOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject packing(int argb, String unit) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("value", argb);
        result.addProperty("a", ARGB.alpha(argb));
        result.addProperty("r", ARGB.red(argb));
        result.addProperty("g", ARGB.green(argb));
        result.addProperty("b", ARGB.blue(argb));
        return result;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_color: 单元至少两段: " + unit);
        }

        switch (parts[0]) {
            case "murmur" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException("mth_color: murmur 需要 1 个参数: " + unit);
                }
                int value = (int) Long.parseLong(parts[1]);
                JsonObject result = new JsonObject();
                result.addProperty("unit", unit);
                result.addProperty("value", Mth.murmurHash3Mixer(value));
                return result;
            }
            case "hsv" -> {
                if (parts.length != 4) {
                    throw new IllegalArgumentException("mth_color: hsv 需要 3 个参数: " + unit);
                }
                float h = Float.parseFloat(parts[1]);
                float s = Float.parseFloat(parts[2]);
                float v = Float.parseFloat(parts[3]);
                return packing(Mth.hsvToRgb(h, s, v), unit);
            }
            case "hsva" -> {
                if (parts.length != 5) {
                    throw new IllegalArgumentException("mth_color: hsva 需要 4 个参数: " + unit);
                }
                float h = Float.parseFloat(parts[1]);
                float s = Float.parseFloat(parts[2]);
                float v = Float.parseFloat(parts[3]);
                int alpha = Integer.parseInt(parts[4]);
                return packing(Mth.hsvToArgb(h, s, v, alpha), unit);
            }
            default -> throw new IllegalArgumentException("mth_color: 无法识别的单元: " + unit);
        }
    }
}