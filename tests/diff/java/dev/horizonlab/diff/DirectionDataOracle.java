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
import net.minecraft.core.Direction;
import net.minecraft.util.Mth;
import net.minecraft.core.Vec3i;
import net.minecraft.world.phys.Vec3;
import org.joml.Vector3fc;

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
                    "choose|NORTH|-7,0,0",
                    "u3i|DOWN",
                    "u3i|UP",
                    "u3i|NORTH",
                    "u3i|SOUTH",
                    "u3i|WEST",
                    "u3i|EAST",
                    "u3f|DOWN",
                    "u3f|NORTH",
                    "u3f|WEST",
                    "u3f|EAST",
                    "u3|DOWN",
                    "u3|UP",
                    "u3|SOUTH",
                    "u3|EAST",
                    "ord|0|0",
                    "ord|90|0",
                    "ord|180|0",
                    "ord|-90|0",
                    "ord|45|0",
                    "ord|-45|0",
                    "ord|135|0",
                    "ord|0|90",
                    "ord|0|-90",
                    "ord|0|45",
                    "ord|0|-45",
                    "ord|0|89",
                    "ord|45|30",
                    "ord|-135|-60",
                    "ord|30|-20",
                    "ord|200|70",
                    "ord|270|0",
                    "ord|359|0",
                    "vd3|0",
                    "vd3|1",
                    "vd3|2",
                    "vd3|3",
                    "vd3|4",
                    "vd3|5",
                    "vd3|6",
                    "vd3|7",
                    "vd3|11",
                    "vd3|12",
                    "vd3|-1",
                    "vd3|-2",
                    "vd3|-6",
                    "vd3|-7",
                    "vd2|0",
                    "vd2|1",
                    "vd2|2",
                    "vd2|3",
                    "vd2|4",
                    "vd2|5",
                    "vd2|7",
                    "vd2|8",
                    "vd2|-1",
                    "vd2|-2",
                    "vd2|-4",
                    "vd2|-5",
                    "yrot|NORTH",
                    "yrot|SOUTH",
                    "yrot|WEST",
                    "yrot|EAST",
                    "torot|NORTH",
                    "torot|SOUTH",
                    "torot|WEST",
                    "torot|EAST");

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

    /**
     * 按 yaw/pitch 复算六个方向的排序。
     *
     * 原版 `Direction.orderedByNearest` 只接受 Entity，无法脱离实体调用，
     * 因此这里照原版公式用给定角度复算（与探针侧同一个算法）。
     */
    private static Direction[] orderedByNearest(float yawDeg, float pitchDeg) {
        float f = pitchDeg * (float) (Math.PI / 180.0);
        float f1 = -yawDeg * (float) (Math.PI / 180.0);
        float f2 = Mth.sin(f);
        float f3 = Mth.cos(f);
        float f4 = Mth.sin(f1);
        float f5 = Mth.cos(f1);
        boolean flag = f4 > 0.0F;
        boolean flag1 = f2 < 0.0F;
        boolean flag2 = f5 > 0.0F;
        float f6 = flag ? f4 : -f4;
        float f7 = flag1 ? -f2 : f2;
        float f8 = flag2 ? f5 : -f5;
        float f9 = f6 * f3;
        float f10 = f8 * f3;
        Direction direction = flag ? Direction.EAST : Direction.WEST;
        Direction direction1 = flag1 ? Direction.UP : Direction.DOWN;
        Direction direction2 = flag2 ? Direction.SOUTH : Direction.NORTH;
        if (f6 > f8) {
            if (f7 > f9) {
                return makeDirectionArray(direction1, direction, direction2);
            }
            return f10 > f7 ? makeDirectionArray(direction, direction2, direction1) : makeDirectionArray(direction, direction1, direction2);
        }
        if (f7 > f10) {
            return makeDirectionArray(direction1, direction2, direction);
        }
        return f9 > f7 ? makeDirectionArray(direction2, direction, direction1) : makeDirectionArray(direction2, direction1, direction);
    }

    private static Direction[] makeDirectionArray(Direction a, Direction b, Direction c) {
        return new Direction[] {a, b, c, c.getOpposite(), b.getOpposite(), a.getOpposite()};
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("direction_data: unit needs at least two fields: " + unit);
        }

        String mode = parts[0];
        
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        // ord 单元只有角度参数，没有方向名，先分流再解析方向。
        if (mode.equals("yrot") || mode.equals("torot")) {
            Direction parsed = parseDirection(parts[1]);
            if (parsed == null) {
                out.addProperty("value", "none");
                return out;
            }
            float rot = mode.equals("yrot") ? Direction.getYRot(parsed) : parsed.toYRot();
            out.addProperty("value", String.format(Locale.ROOT, "%.1f", rot));
            return out;
        }
        
        if (mode.equals("vd3") || mode.equals("vd2")) {
            int raw = Integer.parseInt(parts[1]);
            Direction result = mode.equals("vd3") ? Direction.from3DDataValue(raw) : Direction.from2DDataValue(raw);
            out.addProperty("value", result.getName());
            return out;
        }
        
        if (mode.equals("ord")) {
            if (parts.length != 3) {
                throw new IllegalArgumentException("direction_data: ord needs yaw,pitch: " + unit);
            }
            Direction[] ordered = orderedByNearest(Float.parseFloat(parts[1]), Float.parseFloat(parts[2]));
            StringBuilder joined = new StringBuilder();
            for (int i = 0; i < ordered.length; i++) {
                if (i > 0) {
                    joined.append(',');
                }
                joined.append(ordered[i].getName());
            }
            out.addProperty("value", joined.toString());
            return out;
        }
        
        Direction dir = parseDirection(parts[1]);

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
            case "u3i" -> {
                Vec3i v = dir.getUnitVec3i();
                out.addProperty("value", v.getX() + "," + v.getY() + "," + v.getZ());
            }
            case "u3f" -> {
                Vector3fc v = dir.getUnitVec3f();
                out.addProperty("value", String.format(Locale.ROOT, "%.1f,%.1f,%.1f", v.x(), v.y(), v.z()));
            }
            case "u3" -> {
                Vec3 v = dir.getUnitVec3();
                out.addProperty("value", String.format(Locale.ROOT, "%.1f,%.1f,%.1f", v.x, v.y, v.z));
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