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
import java.util.ArrayList;
import java.util.List;
import net.minecraft.SharedConstants;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.level.levelgen.LegacyRandomSource;

/**
 * {@code LegacyRandomSource} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.LegacyRandomOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用 {@link LegacyRandomSource}，不重写 LCG。
 *
 * <p>单元表必须与 probes/legacy_random.cpp 逐项一致。
 */
public final class LegacyRandomOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] UNITS = {
        "0|i:10;i:10;i:10;i:10;i:10",
        "1|i:10;i:10;i:10;i:10;i:10",
        "12345|i:100;i:100;i:100;i:100;i:100",
        "42|i:8;i:8;i:8;i:16;i:32;i:64",
        "42|i:3;i:5;i:7;i:9;i:11",
        "7|i:1;i:1;i:1",
        "99|i:1000000;i:2147483647;i:1000000007",
        "2024|I;I;l;l;b;b;f;f;d;d",
        "555|c:10;i:100",
        "555|c:1;i:100",
        "-12345|i:100;i:100;i:100",
        "2024|t:1;t:1;t:1;d;d",
        "2024|T:1;T:1;T:1;f;f",
        "777|d;t:0.5;d;T:0.5;f",
        "42|n:5,5;n:5,5;n:5,5",
        "42|n:-10,-10;n:-10,-10",
        "42|n:-5,5;n:-5,5;n:-5,5;n:-5,5",
        "42|n:0,1;n:0,1;n:0,1;n:0,1",
        "9223372036854775807|i:1000;i:1000",
    };

    private LegacyRandomOracle() {}

    public static void main(String[] args) throws IOException {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
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
        System.err.println("usage: LegacyRandomOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            JsonObject row = new JsonObject();
            row.addProperty("result", evaluate(unit));
            row.addProperty("unit", unit);
            text.append(GSON.toJson(row)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /** 剥掉尾随零，与 C++ 的 %g 行为一致。 */
    private static String trimZeros(String text) {
        if (text.indexOf('e') >= 0 || text.indexOf('E') >= 0) {
            return text;
        }
        while (text.endsWith("0") && text.indexOf('.') >= 0) {
            text = text.substring(0, text.length() - 1);
        }
        if (text.endsWith(".")) {
            text = text.substring(0, text.length() - 1);
        }
        return text;
    }

    private static String evaluate(String unit) {
        int bar = unit.indexOf('|');
        if (bar < 0) {
            throw new IllegalArgumentException("legacy_random: 单元应为 <seed>|<ops>: " + unit);
        }
        long seed = Long.parseLong(unit.substring(0, bar));
        LegacyRandomSource random = new LegacyRandomSource(seed);

        List<String> parts = new ArrayList<>();
        for (String op : unit.substring(bar + 1).split(";", -1)) {
            if (op.isEmpty()) {
                continue;
            }
            char kind = op.charAt(0);
            switch (kind) {
                case 'i' -> parts.add(Integer.toString(random.nextInt(Integer.parseInt(op.substring(2)))));
                case 'I' -> parts.add(Integer.toString(random.nextInt()));
                case 'l' -> parts.add(Long.toString(random.nextLong()));
                case 'b' -> parts.add(random.nextBoolean() ? "1" : "0");
                case 'f' -> parts.add(trimZeros(String.format("%.9g", (double) random.nextFloat())));
                case 'd' -> parts.add(trimZeros(String.format("%.16g", random.nextDouble())));
                case 't' -> parts.add(trimZeros(String.format("%.16g", random.triangle(0.0, Double.parseDouble(op.substring(2))))));
                case 'T' -> parts.add(trimZeros(String.format("%.9g", (double) random.triangle(0.0f, Float.parseFloat(op.substring(2))))));
                case 'n' -> {
                    String args = op.substring(2);
                    int comma = args.indexOf(',');
                    if (comma < 0) {
                        throw new IllegalArgumentException("legacy_random: n needs <origin>,<bound>: " + op);
                    }
                    int origin = Integer.parseInt(args.substring(0, comma));
                    int bound = Integer.parseInt(args.substring(comma + 1));
                    parts.add(Integer.toString(random.nextIntBetweenInclusive(origin, bound)));
                }
                case 'c' -> random.consumeCount(Integer.parseInt(op.substring(2)));
                default -> throw new IllegalArgumentException("legacy_random: 未知操作: " + op);
            }
        }
        return String.join(",", parts);
    }
}