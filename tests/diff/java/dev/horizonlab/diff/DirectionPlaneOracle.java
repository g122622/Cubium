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
 * {@code Direction} 数据值与平面判定的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DirectionPlaneOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Direction#values()} 及其实例方法 `get3DDataValue` / `get2DDataValue` /
 * `getAxis` / `getAxisDirection` / `getUnitVec3i` / `getOpposite`，以及
 * {@link Direction#from3DDataValue(int)} / {@link Direction#from2DDataValue(int)} /
 * {@link Direction.Plane}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/direction_plane.cpp 逐项一致。
 */
public final class DirectionPlaneOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "info|DOWN",
                    "info|UP",
                    "info|NORTH",
                    "info|SOUTH",
                    "info|WEST",
                    "info|EAST",
                    "plane|HORIZONTAL",
                    "plane|VERTICAL",
                    "from3d|0",
                    "from3d|1",
                    "from3d|2",
                    "from3d|5",
                    "from3d|6",
                    "from3d|7",
                    "from3d|-1",
                    "from3d|-2",
                    "from3d|12",
                    "from2d|0",
                    "from2d|1",
                    "from2d|2",
                    "from2d|3",
                    "from2d|4",
                    "from2d|-1",
                    "from2d|-2");

    private DirectionPlaneOracle() {}

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
                "usage: DirectionPlaneOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static Direction findDirection(String name) {
        for (Direction dir : Direction.values()) {
            if (dir.name().equals(name)) {
                return dir;
            }
        }
        return null;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("direction_plane: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "info" -> {
                Direction dir = findDirection(parts[1]);
                if (dir == null) {
                    out.addProperty("value", "null");
                } else {
                    var normal = dir.getUnitVec3i();
                    out.addProperty("value",
                            dir.name() + "," + dir.get3DDataValue() + ","
                                    + dir.get2DDataValue() + "," + dir.getAxis().name() + ","
                                    + dir.getAxisDirection().name() + ","
                                    + normal.getX() + "," + normal.getY() + ","
                                    + normal.getZ() + ","
                                    + (dir.getAxis().isHorizontal() ? "true" : "false") + ","
                                    + dir.getOpposite().name());
                }
            }
            case "from3d" -> out.addProperty("value",
                    Direction.from3DDataValue(Integer.parseInt(parts[1])).name());
            case "from2d" -> out.addProperty("value",
                    Direction.from2DDataValue(Integer.parseInt(parts[1])).name());
            case "plane" -> {
                Direction.Plane plane = switch (parts[1]) {
                    case "HORIZONTAL" -> Direction.Plane.HORIZONTAL;
                    case "VERTICAL" -> Direction.Plane.VERTICAL;
                    default -> throw new IllegalArgumentException(
                            "direction_plane: 未知平面: " + parts[1]);
                };
                StringBuilder members = new StringBuilder();
                for (Direction dir : plane) {
                    if (members.length() > 0) {
                        members.append(',');
                    }
                    members.append(dir.name());
                }
                out.addProperty("value", members.toString());
            }
            case "axis" -> {
                Direction dir = findDirection(parts[1]);
                out.addProperty("value", dir == null ? "null" : dir.getAxis().name());
            }
            default -> throw new IllegalArgumentException(
                    "direction_plane: 无法识别的单元: " + unit);
        }

        return out;
    }
}