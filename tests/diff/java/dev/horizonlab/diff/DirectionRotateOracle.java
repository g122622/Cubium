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
 * {@code Direction} 旋转与 Y 轴角度换算的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DirectionRotateOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版的 {@link Direction#getClockWise(Direction.Axis)} /
 * {@link Direction#getCounterClockWise(Direction.Axis)} / {@link Direction#getClockWise()} /
 * {@link Direction#getCounterClockWise()} / {@link Direction#toYRot()} /
 * {@link Direction#fromYRot(double)}，不重写公式。
 *
 * <p>原版对不合法输入抛 {@code IllegalStateException}，本 oracle 捕获后输出
 * {@code {"error": true}}，与探针侧保持一致。
 *
 * <p>单元表必须与 probes/direction_rotate.cpp 逐项一致。
 */
public final class DirectionRotateOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "cw|north",
                    "cw|east",
                    "cw|south",
                    "cw|west",
                    "ccw|north",
                    "ccw|east",
                    "ccw|south",
                    "ccw|west",
                    "cw|up",
                    "cw|down",
                    "cwaxis|east|x",
                    "cwaxis|west|x",
                    "cwaxis|up|y",
                    "cwaxis|down|y",
                    "cwaxis|north|z",
                    "cwaxis|south|z",
                    "cwaxis|up|x",
                    "cwaxis|north|y",
                    "cwaxis|up|z",
                    "cwaxis|north|x",
                    "cwaxis|east|z",
                    "ccwaxis|east|x",
                    "ccwaxis|up|y",
                    "ccwaxis|up|z",
                    "ccwaxis|north|x",
                    "toyrot|south",
                    "toyrot|west",
                    "toyrot|north",
                    "toyrot|east",
                    "toyrot|up",
                    "toyrot|down",
                    "fromyrot|0",
                    "fromyrot|90",
                    "fromyrot|180",
                    "fromyrot|270",
                    "fromyrot|45",
                    "fromyrot|44.9",
                    "fromyrot|45.1",
                    "fromyrot|-45",
                    "fromyrot|-44.9",
                    "fromyrot|360",
                    "fromyrot|-90",
                    "fromyrot|720");

    private DirectionRotateOracle() {}

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
                "usage: DirectionRotateOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static Direction parseDir(String text) {
        return switch (text) {
            case "down" -> Direction.DOWN;
            case "up" -> Direction.UP;
            case "north" -> Direction.NORTH;
            case "south" -> Direction.SOUTH;
            case "west" -> Direction.WEST;
            case "east" -> Direction.EAST;
            default -> throw new IllegalArgumentException("direction_rotate: 未知方向 " + text);
        };
    }

    private static Direction.Axis parseAxis(String text) {
        return switch (text) {
            case "x" -> Direction.Axis.X;
            case "y" -> Direction.Axis.Y;
            case "z" -> Direction.Axis.Z;
            default -> throw new IllegalArgumentException("direction_rotate: 未知轴 " + text);
        };
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|");
        if (parts.length == 0) {
            throw new IllegalArgumentException("direction_rotate: 空单元");
        }

        String mode = parts[0];
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("mode", mode);

        try {
            if ((mode.equals("cw") || mode.equals("ccw")) && parts.length == 2) {
                Direction dir = parseDir(parts[1]);
                Direction out = mode.equals("cw") ? dir.getClockWise() : dir.getCounterClockWise();
                result.addProperty("result", out.getName());
                return result;
            }
            if ((mode.equals("cwaxis") || mode.equals("ccwaxis")) && parts.length == 3) {
                Direction dir = parseDir(parts[1]);
                Direction.Axis axis = parseAxis(parts[2]);
                Direction out =
                        mode.equals("cwaxis")
                                ? dir.getClockWise(axis)
                                : dir.getCounterClockWise(axis);
                result.addProperty("result", out.getName());
                return result;
            }
            if (mode.equals("toyrot") && parts.length == 2) {
                Direction dir = parseDir(parts[1]);
                result.addProperty("result", (int) dir.toYRot());
                return result;
            }
            if (mode.equals("fromyrot") && parts.length == 2) {
                Direction out = Direction.fromYRot(Double.parseDouble(parts[1]));
                result.addProperty("result", out.getName());
                return result;
            }
        } catch (IllegalStateException ex) {
            result.addProperty("error", true);
            return result;
        }

        throw new IllegalArgumentException("direction_rotate: 无法识别的单元: " + unit);
    }
}
