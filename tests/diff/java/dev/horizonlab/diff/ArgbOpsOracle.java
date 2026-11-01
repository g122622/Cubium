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

/**
 * {@code ARGB} 通道与混合方法的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ArgbOpsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ARGB#alpha(int)} / {@link ARGB#red(int)} / {@link ARGB#green(int)} /
 * {@link ARGB#blue(int)} / {@link ARGB#color(int, int, int, int)} /
 * {@link ARGB#color(int, int, int)} / {@link ARGB#multiply(int, int)} /
 * {@link ARGB#greyscale(int)} / {@link ARGB#addRgb(int, int)} /
 * {@link ARGB#scaleRGB(int, float)} / {@link ARGB#scaleRGB(int, float, float, float)} /
 * {@link ARGB#scaleRGB(int, int)} / {@link ARGB#multiplyAlpha(int, float)} /
 * {@link ARGB#alphaBlend(int, int)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/argb_ops.cpp 逐项一致。
 */
public final class ArgbOpsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "ch|0x00000000",
                    "ch|0xFFFFFFFF",
                    "ch|0xFF0000",
                    "ch|0x00FF00",
                    "ch|0x0000FF",
                    "ch|0x80FF8040",
                    "ch|-1",
                    "ch|-65536",
                    "pack|255|255|0|0",
                    "pack|0|255|0|0",
                    "pack|128|0|0|0",
                    "pack3|255|0|0",
                    "pack3|0|0|255",
                    "mul|0xFFFFFFFF|0x8000FF00",
                    "mul|0x8000FF00|0xFFFFFFFF",
                    "mul|0xFFFFFFFF|0xFFFFFFFF",
                    "mul|0x80808080|0x80808080",
                    "grey|0x00FF0000",
                    "grey|0x0000FF00",
                    "grey|0x000000FF",
                    "grey|0xFFFFFFFF",
                    "grey|0x00123456",
                    "add|0x00FF0000|0x0000FF00",
                    "add|0x00FF0000|0x00FF0000",
                    "add|0x80000000|0x00123456",
                    "scaleF|0x00FF8040|0.5",
                    "scaleF|0x00FF8040|2",
                    "scaleF3|0x00FF8040|1|0.5|0",
                    "scaleI|0x00FF8040|127",
                    "scaleI|0x00FF8040|255",
                    "scaleI|0x00FF8040|0",
                    "mulAlpha|0xFF0000FF|0.5",
                    "mulAlpha|0xFF0000FF|0",
                    "mulAlpha|0xFF0000FF|1",
                    "blend|0x00333333|0x80FF0000",
                    "blend|0x00333333|0xFFFFFFFF",
                    "blend|0x00333333|0x00FFFFFF");

    private ArgbOpsOracle() {}

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
                "usage: ArgbOpsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 解析颜色：支持十进制与 `0x` 十六进制。 */
    private static int parseColor(String text) {
        if (text.length() > 2 && text.charAt(0) == '0'
                && (text.charAt(1) == 'x' || text.charAt(1) == 'X')) {
            return (int) Long.parseUnsignedLong(text.substring(2), 16);
        }
        return Integer.parseInt(text);
    }

    private static float parseF(String text) {
        return (float) Double.parseDouble(text);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("argb_ops: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "ch" -> {
                int color = parseColor(parts[1]);
                out.addProperty("value", ARGB.alpha(color) + "," + ARGB.red(color) + ","
                        + ARGB.green(color) + "," + ARGB.blue(color));
            }
            case "pack" -> out.addProperty("value", ARGB.color(
                    Integer.parseInt(parts[1]), Integer.parseInt(parts[2]),
                    Integer.parseInt(parts[3]), Integer.parseInt(parts[4])));
            case "pack3" -> out.addProperty("value", ARGB.color(
                    Integer.parseInt(parts[1]), Integer.parseInt(parts[2]),
                    Integer.parseInt(parts[3])));
            case "mul" -> out.addProperty("value",
                    ARGB.multiply(parseColor(parts[1]), parseColor(parts[2])));
            case "grey" -> out.addProperty("value", ARGB.greyscale(parseColor(parts[1])));
            case "add" -> out.addProperty("value",
                    ARGB.addRgb(parseColor(parts[1]), parseColor(parts[2])));
            case "scaleF" -> out.addProperty("value",
                    ARGB.scaleRGB(parseColor(parts[1]), parseF(parts[2])));
            case "scaleF3" -> out.addProperty("value", ARGB.scaleRGB(
                    parseColor(parts[1]), parseF(parts[2]), parseF(parts[3]), parseF(parts[4])));
            case "scaleI" -> out.addProperty("value",
                    ARGB.scaleRGB(parseColor(parts[1]), Integer.parseInt(parts[2])));
            case "mulAlpha" -> out.addProperty("value",
                    ARGB.multiplyAlpha(parseColor(parts[1]), parseF(parts[2])));
            case "blend" -> out.addProperty("value",
                    ARGB.alphaBlend(parseColor(parts[1]), parseColor(parts[2])));
            default -> throw new IllegalArgumentException("argb_ops: 无法识别的单元: " + unit);
        }

        return out;
    }
}