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
import net.minecraft.util.Mth;

/**
 * {@code Mth.lerpInt} / {@code Mth.lerpDiscrete} / {@code Mth.binarySearch} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthMapLerpOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/mth_map_lerp.cpp 逐项一致。
 */
public final class MthMapLerpOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "lerpInt|0|0|10",
                    "lerpInt|1|0|10",
                    "lerpInt|0.5|0|10",
                    "lerpInt|0.25|0|10",
                    "lerpInt|0.75|0|10",
                    "lerpInt|0.999|0|10",
                    "lerpInt|0.001|0|10",
                    "lerpInt|0|10|0",
                    "lerpInt|1|10|0",
                    "lerpInt|0.5|10|0",
                    "lerpInt|0.5|5|5",
                    "lerpDiscrete|0|0|10",
                    "lerpDiscrete|1|0|10",
                    "lerpDiscrete|0.5|0|10",
                    "lerpDiscrete|0.25|0|10",
                    "lerpDiscrete|0.75|0|10",
                    "lerpDiscrete|0.999|0|10",
                    "lerpDiscrete|0.001|0|10",
                    "lerpDiscrete|0.0001|0|10",
                    "lerpDiscrete|0|10|0",
                    "lerpDiscrete|1|10|0",
                    "lerpDiscrete|0.5|10|0",
                    "lerpDiscrete|0|5|6",
                    "lerpDiscrete|0.5|5|6",
                    "lerpDiscrete|1|5|6",
                    "lerpDiscrete|0.5|5|5",
                    "search|0|10|5",
                    "search|0|10|0",
                    "search|0|10|9",
                    "search|0|10|-1",
                    "search|0|10|10",
                    "search|0|10|99",
                    "search|5|5|3",
                    "search|-10|10|0",
                    "search|-10|10|-10");

    private MthMapLerpOracle() {}

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
                "usage: MthMapLerpOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_map_lerp: unit needs at least two fields: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "lerpInt" -> out.addProperty("value",
                    Mth.lerpInt(Float.parseFloat(parts[1]), Integer.parseInt(parts[2]), Integer.parseInt(parts[3])));
            case "lerpDiscrete" -> out.addProperty("value",
                    Mth.lerpDiscrete(Float.parseFloat(parts[1]), Integer.parseInt(parts[2]), Integer.parseInt(parts[3])));
            case "search" -> {
                int from = Integer.parseInt(parts[1]);
                int to = Integer.parseInt(parts[2]);
                int threshold = Integer.parseInt(parts[3]);
                out.addProperty("value", Mth.binarySearch(from, to, value -> value >= threshold));
            }
            default -> throw new IllegalArgumentException("mth_map_lerp: unrecognized unit: " + unit);
        }

        return out;
    }
}
