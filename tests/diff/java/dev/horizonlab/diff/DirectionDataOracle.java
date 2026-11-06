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
 * Direction 数据值编码与按轴取分量的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DirectionDataOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 本能力不需要注册表。
 *
 * 原版 Direction 没有 NONE 常量；单元表里的 NONE 在探针侧映射为返回 -1，
 * 因此 oracle 也对 NONE 直接返回 -1，两侧语义一致。
 *
 * 单元表必须与 probes/direction_data.cpp 逐项一致。
 */
public final class DirectionDataOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final List<String> UNITS =
            List.of(
                    "v3|DOWN",
                    "v3|UP",
                    "v3|NORTH",
                    "v3|SOUTH",
                    "v3|WEST",
                    "v3|EAST",
                    "v3|NONE",
                    "v2|DOWN",
                    "v2|UP",
                    "v2|NORTH",
                    "v2|SOUTH",
                    "v2|WEST",
                    "v2|EAST",
                    "v2|NONE",
                    "choose|DOWN|11,22,33",
                    "choose|UP|11,22,33",
                    "choose|NORTH|11,22,33",
                    "choose|SOUTH|11,22,33",
                    "choose|WEST|11,22,33",
                    "choose|EAST|11,22,33",
                    "choose|EAST|-7,0,0",
                    "choose|UP|-7,0,0",
                    "choose|NORTH|-7,0,0");

    private DirectionDataOracle() {}

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
                "usage: DirectionDataOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 方向名转枚举；NONE 返回 null（原版没有这个常量）。 */
    private static Direction parseDirection(String name) {
        return switch (name) {
            case "DOWN" -> Direction.DOWN;
            case "UP" -> Direction.UP;
            case "NORTH" -> Direction.NORTH;
            case "SOUTH" -> Direction.SOUTH;
            case "WEST" -> Direction.WEST;
            case "EAST" -> Direction.EAST;
            case "NONE" -> null;
            default -> throw new IllegalArgumentException("direction_data: unknown direction: " + name);
        };
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("direction_data: unit needs at least two fields: " + unit);
        }

        String mode = parts[0];
        Direction dir = parseDirection(parts[1]);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        // NONE 在两侧都按 -1 处理：原版没有该常量，取不到数据值。
        if (dir == null) {
            out.addProperty("value", -1);
            return out;
        }

        switch (mode) {
            case "v3" -> out.addProperty("value", dir.get3DDataValue());
            case "v2" -> {
                // 原版 get2DDataValue 对垂直方向返回 -1。
                out.addProperty("value", dir.get2DDataValue());
            }
            case "choose" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException("direction_data: choose needs x,y,z: " + unit);
                }
                String[] coords = parts[2].split(",", -1);
                if (coords.length != 3) {
                    throw new IllegalArgumentException("direction_data: choose needs x,y,z: " + unit);
                }
                int x = Integer.parseInt(coords[0]);
                int y = Integer.parseInt(coords[1]);
                int z = Integer.parseInt(coords[2]);
                out.addProperty("value", dir.getAxis().choose(x, y, z));
            }
            default -> throw new IllegalArgumentException("direction_data: unrecognized unit: " + unit);
        }

        return out;
    }
}