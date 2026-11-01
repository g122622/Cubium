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
 * {@code Mth} 整数除法与 2 的幂工具的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthIntDivOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#floorDiv(int, int)} / {@link Mth#isPowerOfTwo(int)} /
 * {@link Mth#smallestEncompassingPowerOfTwo(int)} / {@link Mth#ceillog2(int)} /
 * {@link Mth#log2(int)} / {@link Mth#positiveCeilDiv(int, int)} /
 * {@link Mth#roundToward(int, int)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/mth_int_div.cpp 逐项一致。
 */
public final class MthIntDivOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "floorDiv|7|2",
                    "floorDiv|-7|2",
                    "floorDiv|7|-2",
                    "floorDiv|-7|-2",
                    "floorDiv|6|2",
                    "floorDiv|-6|2",
                    "floorDiv|0|5",
                    "floorDiv|-1|5",
                    "isPow2|0",
                    "isPow2|1",
                    "isPow2|2",
                    "isPow2|3",
                    "isPow2|4",
                    "isPow2|-4",
                    "isPow2|16384",
                    "isPow2|16383",
                    "smallest|0",
                    "smallest|1",
                    "smallest|2",
                    "smallest|3",
                    "smallest|4",
                    "smallest|5",
                    "smallest|17",
                    "smallest|-1",
                    "ceillog2|1",
                    "ceillog2|2",
                    "ceillog2|3",
                    "ceillog2|4",
                    "ceillog2|5",
                    "ceillog2|16",
                    "ceillog2|17",
                    "ceillog2|1024",
                    "log2|1",
                    "log2|2",
                    "log2|3",
                    "log2|4",
                    "log2|5",
                    "log2|16",
                    "log2|17",
                    "posCeilDiv|7|2",
                    "posCeilDiv|6|2",
                    "posCeilDiv|1|5",
                    "posCeilDiv|0|5",
                    "posCeilDiv|-1|5",
                    "roundToward|7|2",
                    "roundToward|6|2",
                    "roundToward|1|5",
                    "roundToward|0|5",
                    "roundToward|-1|5",
                    "roundToward|15|16",
                    "roundToward|16|16");

    private MthIntDivOracle() {}

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
                "usage: MthIntDivOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static int parseInt(String text) {
        return switch (text) {
            case "MIN" -> Integer.MIN_VALUE;
            case "MAX" -> Integer.MAX_VALUE;
            default -> Integer.parseInt(text);
        };
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_int_div: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "floorDiv" -> out.addProperty("value",
                    Mth.floorDiv(parseInt(parts[1]), parseInt(parts[2])));
            case "isPow2" -> out.addProperty("value",
                    Mth.isPowerOfTwo(parseInt(parts[1])) ? "true" : "false");
            case "smallest" -> out.addProperty("value",
                    Mth.smallestEncompassingPowerOfTwo(parseInt(parts[1])));
            case "ceillog2" -> out.addProperty("value", Mth.ceillog2(parseInt(parts[1])));
            case "log2" -> out.addProperty("value", Mth.log2(parseInt(parts[1])));
            case "posCeilDiv" -> out.addProperty("value",
                    Mth.positiveCeilDiv(parseInt(parts[1]), parseInt(parts[2])));
            case "roundToward" -> out.addProperty("value",
                    Mth.roundToward(parseInt(parts[1]), parseInt(parts[2])));
            default -> throw new IllegalArgumentException("mth_int_div: 无法识别的单元: " + unit);
        }

        return out;
    }
}