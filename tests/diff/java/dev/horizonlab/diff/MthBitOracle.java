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
 * {@code Mth} 整数位运算工具的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthBitOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#smallestEncompassingPowerOfTwo(int)} /
 * {@link Mth#isPowerOfTwo(int)} / {@link Mth#ceillog2(int)} / {@link Mth#log2(int)}，
 * 不重写公式。
 *
 * <p>单元表必须与 probes/mth_bit.cpp 逐项一致。
 */
public final class MthBitOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "sep2|1",
                    "sep2|2",
                    "sep2|4",
                    "sep2|8",
                    "sep2|16",
                    "sep2|3",
                    "sep2|5",
                    "sep2|6",
                    "sep2|7",
                    "sep2|9",
                    "sep2|17",
                    "sep2|31",
                    "sep2|33",
                    "sep2|1000",
                    "sep2|30000000",
                    "sep2|1073741824",
                    "sep2|0",
                    "sep2|-1",
                    "sep2|-2",
                    "sep2|-5",
                    "ispot|1",
                    "ispot|2",
                    "ispot|3",
                    "ispot|4",
                    "ispot|0",
                    "ispot|-1",
                    "ispot|-2",
                    "log2|1",
                    "log2|2",
                    "log2|4",
                    "log2|8",
                    "log2|1024",
                    "log2|3",
                    "log2|5",
                    "log2|7",
                    "log2|9",
                    "log2|31",
                    "log2|1023",
                    "log2|1025",
                    "ceillog2|1",
                    "ceillog2|2",
                    "ceillog2|3",
                    "ceillog2|5",
                    "ceillog2|9",
                    "ceillog2|1025");

    private MthBitOracle() {}

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
                "usage: MthBitOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 2) {
            throw new IllegalArgumentException("mth_bit: 单元应为两段 '模式|参数': " + unit);
        }

        String mode = parts[0];
        int value = Integer.parseInt(parts[1]);

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        switch (mode) {
            case "sep2" -> result.addProperty("value", Mth.smallestEncompassingPowerOfTwo(value));
            case "ispot" -> result.addProperty("result", Mth.isPowerOfTwo(value));
            case "log2" -> result.addProperty("value", Mth.log2(value));
            case "ceillog2" -> result.addProperty("value", Mth.ceillog2(value));
            default -> throw new IllegalArgumentException("mth_bit: 无法识别的单元: " + unit);
        }

        return result;
    }
}