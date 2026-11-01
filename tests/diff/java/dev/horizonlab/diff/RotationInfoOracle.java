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
import net.minecraft.world.level.block.Rotation;

/**
 * {@code Rotation} 枚举语义与步进旋转的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RotationInfoOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Rotation#values()} 及其实例方法 `getSerializedName` /
 * `getRotated` / `rotate(Direction)` / `rotate(int, int)`，以及 {@link Rotation#BY_ID}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/rotation_info.cpp 逐项一致。
 */
public final class RotationInfoOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "info|NONE",
                    "info|CLOCKWISE_90",
                    "info|CLOCKWISE_180",
                    "info|COUNTERCLOCKWISE_90",
                    "byId|0",
                    "byId|1",
                    "byId|2",
                    "byId|3",
                    "byId|4",
                    "byId|5",
                    "byId|9",
                    "byId|-1",
                    "byId|-2",
                    "byId|-4",
                    "getRotated|NONE|NONE",
                    "getRotated|NONE|CLOCKWISE_90",
                    "getRotated|CLOCKWISE_90|CLOCKWISE_90",
                    "getRotated|CLOCKWISE_90|COUNTERCLOCKWISE_90",
                    "getRotated|CLOCKWISE_180|CLOCKWISE_180",
                    "getRotated|COUNTERCLOCKWISE_90|CLOCKWISE_90",
                    "rotateDir|CLOCKWISE_90|UP",
                    "rotateDir|CLOCKWISE_90|DOWN",
                    "rotateDir|CLOCKWISE_180|UP",
                    "rotateDir|COUNTERCLOCKWISE_90|DOWN",
                    "rotateDir|NONE|NORTH",
                    "rotateDir|CLOCKWISE_90|NORTH",
                    "rotateDir|CLOCKWISE_90|EAST",
                    "rotateDir|CLOCKWISE_90|SOUTH",
                    "rotateDir|CLOCKWISE_90|WEST",
                    "rotateDir|CLOCKWISE_180|NORTH",
                    "rotateDir|CLOCKWISE_180|EAST",
                    "rotateDir|COUNTERCLOCKWISE_90|NORTH",
                    "rotateDir|COUNTERCLOCKWISE_90|EAST",
                    "rotateInt|CLOCKWISE_90|0|4",
                    "rotateInt|CLOCKWISE_90|3|4",
                    "rotateInt|CLOCKWISE_180|0|4",
                    "rotateInt|CLOCKWISE_180|3|4",
                    "rotateInt|COUNTERCLOCKWISE_90|0|4",
                    "rotateInt|COUNTERCLOCKWISE_90|1|4",
                    "rotateInt|NONE|2|4",
                    "rotateInt|CLOCKWISE_90|0|6",
                    "rotateInt|COUNTERCLOCKWISE_90|0|6",
                    "rotateInt|CLOCKWISE_180|0|6",
                    "rotateInt|CLOCKWISE_90|-1|4",
                    "rotateInt|COUNTERCLOCKWISE_90|-1|6");

    private RotationInfoOracle() {}

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
                "usage: RotationInfoOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static Rotation findRotation(String name) {
        for (Rotation rot : Rotation.values()) {
            if (rot.name().equals(name)) {
                return rot;
            }
        }
        return null;
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
            throw new IllegalArgumentException("rotation_info: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "info" -> {
                Rotation rot = findRotation(parts[1]);
                if (rot == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value",
                            rot.ordinal() + "," + rot.getSerializedName());
                }
            }
            case "byId" -> out.addProperty("value",
                    Rotation.BY_ID.apply(Integer.parseInt(parts[1])).name());
            case "getRotated" -> {
                Rotation base = findRotation(parts[1]);
                Rotation other = findRotation(parts[2]);
                if (base == null || other == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value", base.getRotated(other).name());
                }
            }
            case "rotateDir" -> {
                Rotation rot = findRotation(parts[1]);
                Direction dir = findDirection(parts[2]);
                if (rot == null || dir == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value", rot.rotate(dir).name());
                }
            }
            case "rotateInt" -> {
                Rotation rot = findRotation(parts[1]);
                if (rot == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value", rot.rotate(
                            Integer.parseInt(parts[2]), Integer.parseInt(parts[3])));
                }
            }
            default -> throw new IllegalArgumentException("rotation_info: 无法识别的单元: " + unit);
        }

        return out;
    }
}