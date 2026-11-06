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
import net.minecraft.core.Direction;

/**
 * {@code Direction.getApproximateNearest} 与 {@code Direction.getNearest} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DirectionNearestOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Direction#getApproximateNearest(float, float, float)} /
 * {@link Direction#getApproximateNearest(double, double, double)} /
 * {@link Direction#getNearest(int, int, int, Direction)}。
 *
 * <p>本能力不需要注册表——{@code Direction} 的枚举与法向量都是静态常量。
 *
 * <p>方向输出为小写枚举名；{@code null} 输出为字符串 {@code "null"}。
 *
 * <p>单元表必须与 probes/direction_nearest.cpp 逐项一致。
 */
public final class DirectionNearestOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                      "approxF|1|0|0",
                      "approxF|-1|0|0",
                      "approxF|0|1|0",
                      "approxF|0|-1|0",
                      "approxF|0|0|1",
                      "approxF|0|0|-1",
                      "approxF|0|0|0",
                      "approxF|1e-40|0|0",
                      "approxF|-1e-40|0|0",
                      "approxF|1e-46|1e-46|1e-46",
                      "approxF|1|1|0",
                      "approxF|1|0|1",
                      "approxF|0|1|1",
                      "approxF|1|1|1",
                      "approxF|-1|-1|-1",
                      "approxF|2|1|0",
                      "approxF|1|2|0",
                      "approxD|1|0|0",
                      "approxD|0|0|0",
                      "approxD|1e-300|0|0",
                      "nearest|5|1|1|north",
                      "nearest|-5|1|1|north",
                      "nearest|1|5|1|north",
                      "nearest|1|-5|1|north",
                      "nearest|1|1|5|north",
                      "nearest|1|1|-5|north",
                      "nearest|0|0|0|north",
                      "nearest|0|0|0|up",
                      "nearest|5|5|1|north",
                      "nearest|5|1|5|north",
                      "nearest|1|5|5|north",
                      "nearest|5|5|5|north",
                      "nearest|-5|-5|1|up",
                      "nearest|-5|1|1|north",
                      "nearest|5|-1|-1|north");

    private DirectionNearestOracle() {}

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
        System.err.println("usage: DirectionNearestOracle list <units.txt> "
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

    /** 方向名转小写；`null` 输出字符串 `"null"`。 */
    private static String nameOf(Direction direction) {
        return direction == null ? "null" : direction.getName();
    }

    /** 解析 `north` / `null` 等回退方向。 */
    private static Direction parseDirection(String text) {
        if (text.equals("null")) {
            return null;
        }
        for (Direction direction : Direction.values()) {
            if (direction.getName().equals(text)) {
                return direction;
            }
        }
        throw new IllegalArgumentException("direction_nearest: 未知方向: " + text);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("direction_nearest: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "approxF" -> out.addProperty("value", nameOf(Direction.getApproximateNearest(
                    (float) Double.parseDouble(parts[1]),
                    (float) Double.parseDouble(parts[2]),
                    (float) Double.parseDouble(parts[3]))));
            case "approxD" -> out.addProperty("value", nameOf(Direction.getApproximateNearest(
                    Double.parseDouble(parts[1]),
                    Double.parseDouble(parts[2]),
                    Double.parseDouble(parts[3]))));
            case "nearest" -> out.addProperty("value", nameOf(Direction.getNearest(
                    Integer.parseInt(parts[1]), Integer.parseInt(parts[2]),
                    Integer.parseInt(parts[3]), parseDirection(parts[4]))));
            default -> throw new IllegalArgumentException(
                    "direction_nearest: 无法识别的单元: " + unit);
        }

        return out;
    }
}