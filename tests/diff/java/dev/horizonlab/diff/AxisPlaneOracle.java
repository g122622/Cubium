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
 * {@code Direction.Axis} 与 {@code Direction.Plane} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AxisPlaneOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Direction.Axis#choose(int, int, int)} /
 * {@link Direction.Axis#getPositive()} / {@link Direction.Axis#getNegative()} /
 * {@link Direction.Axis#getPlane()} / {@link Direction#getAxis()} /
 * {@link Direction.Plane#test(Direction)}，不重写公式。
 *
 * <p>单元表必须与 probes/axis_plane.cpp 逐项一致。
 */
public final class AxisPlaneOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "choosei|x",
                    "choosei|y",
                    "choosei|z",
                    "positive|x",
                    "positive|y",
                    "positive|z",
                    "negative|x",
                    "negative|y",
                    "negative|z",
                    "planeof|x",
                    "planeof|y",
                    "planeof|z",
                    "axisof|down",
                    "axisof|up",
                    "axisof|north",
                    "axisof|south",
                    "axisof|west",
                    "axisof|east",
                    "test|horizontal|north",
                    "test|horizontal|east",
                    "test|horizontal|south",
                    "test|horizontal|west",
                    "test|horizontal|up",
                    "test|horizontal|down",
                    "test|vertical|up",
                    "test|vertical|down",
                    "test|vertical|north",
                    "test|vertical|east",
                    "test|horizontal|null",
                    "test|vertical|null");

    private AxisPlaneOracle() {}

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
                "usage: AxisPlaneOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            default -> throw new IllegalArgumentException("axis_plane: 未知方向 " + text);
        };
    }

    private static Direction.Axis parseAxis(String text) {
        return switch (text) {
            case "x" -> Direction.Axis.X;
            case "y" -> Direction.Axis.Y;
            case "z" -> Direction.Axis.Z;
            default -> throw new IllegalArgumentException("axis_plane: 未知轴 " + text);
        };
    }

    private static Direction.Plane parsePlane(String text) {
        return switch (text) {
            case "horizontal" -> Direction.Plane.HORIZONTAL;
            case "vertical" -> Direction.Plane.VERTICAL;
            default -> throw new IllegalArgumentException("axis_plane: 未知平面 " + text);
        };
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|");
        if (parts.length < 2) {
            throw new IllegalArgumentException("axis_plane: 单元至少两段: " + unit);
        }

        String mode = parts[0];
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (mode.equals("choosei") && parts.length == 2) {
            Direction.Axis axis = parseAxis(parts[1]);
            // 固定三分量 (11, 22, 33)，用返回值区分取了哪一个。
            result.addProperty("value", axis.choose(11, 22, 33));
            return result;
        }
        if ((mode.equals("positive") || mode.equals("negative")) && parts.length == 2) {
            Direction.Axis axis = parseAxis(parts[1]);
            Direction dir = mode.equals("positive") ? axis.getPositive() : axis.getNegative();
            result.addProperty("value", dir.getName());
            return result;
        }
        if (mode.equals("planeof") && parts.length == 2) {
            Direction.Axis axis = parseAxis(parts[1]);
            result.addProperty(
                    "value",
                    axis.getPlane() == Direction.Plane.VERTICAL ? "vertical" : "horizontal");
            return result;
        }
        if (mode.equals("axisof") && parts.length == 2) {
            Direction dir = parseDir(parts[1]);
            result.addProperty("value", dir.getAxis().getName());
            return result;
        }
        if (mode.equals("test") && parts.length == 3) {
            Direction.Plane plane = parsePlane(parts[1]);
            Direction dir = parts[2].equals("null") ? null : parseDir(parts[2]);
            result.addProperty("value", plane.test(dir));
            return result;
        }

        throw new IllegalArgumentException("axis_plane: 无法识别的单元: " + unit);
    }
}