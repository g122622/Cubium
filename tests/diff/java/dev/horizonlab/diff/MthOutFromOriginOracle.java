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

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.StringJoiner;
import net.minecraft.util.Mth;

/**
 * {@code Mth.outFromOrigin} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthOutFromOriginOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/mth_out_from_origin.cpp 逐项一致。
 */
public final class MthOutFromOriginOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "0|-2|2|1",
                    "0|-3|3|1",
                    "0|-2|2|2",
                    "0|-3|3|2",
                    "0|-3|3|3",
                    "0|0|0|1",
                    "0|0|2|1",
                    "0|0|4|2",
                    "0|-2|0|1",
                    "0|-4|0|2",
                    "5|-2|2|1",
                    "-5|-2|2|1",
                    "100|0|3|1",
                    "-100|-3|0|2",
                    "0|-1|5|1",
                    "0|-5|1|1",
                    "2|-1|5|2",
                    "0|-2|3|2",
                    "0|-3|2|2",
                    "0|-1|1|5",
                    "0|0|1|10");

    private MthOutFromOriginOracle() {}

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
                "usage: MthOutFromOriginOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 4) {
            throw new IllegalArgumentException("mth_out_from_origin: unit needs four fields: " + unit);
        }

        int origin = Integer.parseInt(parts[0]);
        int lower = Integer.parseInt(parts[1]);
        int upper = Integer.parseInt(parts[2]);
        int step = Integer.parseInt(parts[3]);

        int[] sequence = Mth.outFromOrigin(origin, lower, upper, step).toArray();

        StringJoiner joiner = new StringJoiner(",");
        for (int value : sequence) {
            joiner.add(Integer.toString(value));
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        out.addProperty("value", joiner.toString());
        out.addProperty("size", sequence.length);
        return out;
    }
}
