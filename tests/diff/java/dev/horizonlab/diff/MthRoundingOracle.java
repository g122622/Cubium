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
 * {@code Mth.floor} / {@code Mth.ceil} 取整语义的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthRoundingOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#floor(float)} / {@link Mth#floor(double)} /
 * {@link Mth#ceil(float)} / {@link Mth#ceil(double)}，不重写公式。
 *
 * <p>单元表必须与 probes/mth_rounding.cpp 逐项一致。
 */
public final class MthRoundingOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表：待取整的数值。 */
    private static final List<String> UNITS =
            List.of(
                    "0",
                    "1",
                    "-1",
                    "0.5",
                    "-0.5",
                    "1.5",
                    "-1.5",
                    "2.9",
                    "-2.9",
                    "0.1",
                    "-0.1",
                    "100.0",
                    "-100.0",
                    "123456.789",
                    "-123456.789",
                    "0.0001",
                    "-0.0001",
                    "2147483520",
                    "-2147483648",
                    "3000000000",
                    "-3000000000");

    private MthRoundingOracle() {}

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
                "usage: MthRoundingOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        double valueD = Double.parseDouble(unit);
        float valueF = (float) valueD;

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("floorF", Mth.floor(valueF));
        result.addProperty("floorD", Mth.floor(valueD));
        result.addProperty("ceilF", Mth.ceil(valueF));
        result.addProperty("ceilD", Mth.ceil(valueD));
        return result;
    }
}