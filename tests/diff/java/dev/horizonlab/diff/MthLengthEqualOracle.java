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
 * Mth.length / Mth.lengthSquared / Mth.equal 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthLengthEqualOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 本能力不需要注册表。
 *
 * length 与 lengthSquared 输出 double 的原始位模式；equal 输出布尔值。
 *
 * 单元表必须与 probes/mth_length_equal.cpp 逐项一致。
 */
public final class MthLengthEqualOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final List<String> UNITS =
            List.of(
                    "len2sq|3|4",
                    "len2sq|1|1",
                    "len2sq|0|0",
                    "len2sq|-3|-4",
                    "len2sq|0.1|0.2",
                    "len3sq|1|2|2",
                    "len3sq|2|3|6",
                    "len3sq|0|0|0",
                    "len3sq|-2|3|-6",
                    "len3sq|0.1|0.2|0.3",
                    "len2|3|4",
                    "len2|1|1",
                    "len2|0|0",
                    "len2|-3|-4",
                    "len2|0.1|0.2",
                    "len2f|3|4",
                    "len2f|1|1",
                    "len2f|0.1|0.2",
                    "len2f|1e-20|1e-20",
                    "len2f|1e20|1e20",
                    "eqf|1|1",
                    "eqf|1|1.00001",
                    "eqf|1|1.0001",
                    "eqf|1|1.000001",
                    "eqf|0|0.000005",
                    "eqf|0|0.00000999",
                    "eqf|0|0.00001001",
                    "eqd|1|1",
                    "eqd|0|0.000005",
                    "eqd|0|0.000009999",
                    "eqd|0|0.0000099999",
                    "eqd|0|0.0000100001",
                    "eqd|0|0.00001",
                    "eqd|0|0.0001");

    private MthLengthEqualOracle() {}

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
                "usage: MthLengthEqualOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static String bitsOf(double value) {
        return String.format(Locale.ROOT, "0x%016x", Double.doubleToRawLongBits(value));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_length_equal: unit needs at least two fields: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "len2sq" -> out.addProperty("bits",
                    bitsOf(Mth.lengthSquared(Double.parseDouble(parts[1]), Double.parseDouble(parts[2]))));
            case "len3sq" -> out.addProperty("bits",
                    bitsOf(Mth.lengthSquared(Double.parseDouble(parts[1]), Double.parseDouble(parts[2]),
                            Double.parseDouble(parts[3]))));
            case "len2" -> out.addProperty("bits",
                    bitsOf(Mth.length(Double.parseDouble(parts[1]), Double.parseDouble(parts[2]))));
            case "len2f" -> out.addProperty("bits",
                    bitsOf((double) Mth.length(Float.parseFloat(parts[1]), Float.parseFloat(parts[2]))));
            case "eqf" -> out.addProperty("value",
                    Mth.equal(Float.parseFloat(parts[1]), Float.parseFloat(parts[2])));
            case "eqd" -> out.addProperty("value",
                    Mth.equal(Double.parseDouble(parts[1]), Double.parseDouble(parts[2])));
            default -> throw new IllegalArgumentException("mth_length_equal: unrecognized unit: " + unit);
        }

        return out;
    }
}