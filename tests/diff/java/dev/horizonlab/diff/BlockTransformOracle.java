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
import com.mojang.math.OctahedralGroup;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Locale;
import net.minecraft.core.Direction;
import net.minecraft.world.level.block.Mirror;
import net.minecraft.world.level.block.Rotation;
import org.joml.Vector3i;

/**
 * {@code Rotation} / {@code Mirror} 方向与坐标变换的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockTransformOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Rotation#rotate(Direction)} / {@link Mirror#mirror(Direction)} /
 * {@link Mirror#getRotation(Direction)}，以及 {@link OctahedralGroup#rotate(Vector3i)} /
 * {@link OctahedralGroup#compose} / {@link OctahedralGroup#inverse} /
 * {@link OctahedralGroup#inverts(Direction.Axis)}。
 *
 * <p>单元表必须与 probes/block_transform.cpp 逐项一致。
 */
public final class BlockTransformOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "dirrot|CLOCKWISE_90|up",
                    "dirrot|CLOCKWISE_90|down",
                    "dirrot|CLOCKWISE_180|up",
                    "dirrot|COUNTERCLOCKWISE_90|down",
                    "dirrot|NONE|north",
                    "dirrot|CLOCKWISE_90|north",
                    "dirrot|CLOCKWISE_90|east",
                    "dirrot|CLOCKWISE_90|south",
                    "dirrot|CLOCKWISE_90|west",
                    "dirrot|CLOCKWISE_180|north",
                    "dirrot|COUNTERCLOCKWISE_90|north",
                    "dirrot|COUNTERCLOCKWISE_90|east",
                    "dirmirror|NONE|east",
                    "dirmirror|NONE|north",
                    "dirmirror|FRONT_BACK|east",
                    "dirmirror|FRONT_BACK|west",
                    "dirmirror|FRONT_BACK|north",
                    "dirmirror|LEFT_RIGHT|north",
                    "dirmirror|LEFT_RIGHT|south",
                    "dirmirror|LEFT_RIGHT|east",
                    "dirmirror|FRONT_BACK|up",
                    "dirmirror|LEFT_RIGHT|down",
                    "mirrorrot|NONE|x",
                    "mirrorrot|LEFT_RIGHT|x",
                    "mirrorrot|LEFT_RIGHT|z",
                    "mirrorrot|FRONT_BACK|x",
                    "mirrorrot|FRONT_BACK|z",
                    "mirrorrot|LEFT_RIGHT|y",
                    "posrot|NONE|1|2|3",
                    "posrot|CLOCKWISE_90|1|2|3",
                    "posrot|CLOCKWISE_180|1|2|3",
                    "posrot|COUNTERCLOCKWISE_90|1|2|3",
                    "posrot|CLOCKWISE_90|-1|0|0",
                    "posrot|CLOCKWISE_90|0|0|1",
                    "posmirror|NONE|1|2|3",
                    "posmirror|LEFT_RIGHT|1|2|3",
                    "posmirror|FRONT_BACK|1|2|3",
                    "compose|ROT_90_Y_NEG|ROT_90_Y_NEG",
                    "compose|ROT_90_Y_NEG|IDENTITY",
                    "compose|IDENTITY|ROT_90_Y_NEG",
                    "compose|INVERT_X|INVERT_Z",
                    "inverse|ROT_90_Y_NEG",
                    "inverse|ROT_90_Y_POS",
                    "inverse|ROT_180_FACE_XZ",
                    "inverse|INVERT_X",
                    "inverts|INVERT_X|x",
                    "inverts|INVERT_X|y",
                    "inverts|ROT_180_FACE_XZ|z");

    private BlockTransformOracle() {}

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
                "usage: BlockTransformOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static Direction directionOf(String name) {
        return Direction.byName(name.toLowerCase(Locale.ROOT));
    }

    private static Direction.Axis axisOf(String name) {
        switch (name) {
            case "x":
                return Direction.Axis.X;
            case "y":
                return Direction.Axis.Y;
            case "z":
                return Direction.Axis.Z;
            default:
                throw new IllegalArgumentException("block_transform: 未知轴: " + name);
        }
    }

    /** 原版 `Rotation` 名 → `OctahedralGroup`。 */
    private static OctahedralGroup groupOfRotation(String rotation) {
        return Rotation.valueOf(rotation).rotation();
    }

    /** 原版 `Mirror` 名 → `OctahedralGroup`。 */
    private static OctahedralGroup groupOfMirror(String mirror) {
        return Mirror.valueOf(mirror).rotation();
    }

    private static String applyGroup(OctahedralGroup group, int x, int y, int z) {
        Vector3i vec = group.rotate(new Vector3i(x, y, z));
        return vec.x + "," + vec.y + "," + vec.z;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("block_transform: 单元至少三段: " + unit);
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        switch (parts[0]) {
            case "dirrot" -> {
                Rotation rotation = Rotation.valueOf(parts[1]);
                Direction direction = directionOf(parts[2]);
                result.addProperty("value", rotation.rotate(direction).getName());
            }
            case "dirmirror" -> {
                Mirror mirror = Mirror.valueOf(parts[1]);
                Direction direction = directionOf(parts[2]);
                result.addProperty("value", mirror.mirror(direction).getName());
            }
            case "mirrorrot" -> {
                Mirror mirror = Mirror.valueOf(parts[1]);
                // `Mirror.getRotation(BlockState)` 只取 state 的轴线。原版实现：
                //   return (this != LEFT_RIGHT || axis != Z) && (this != FRONT_BACK || axis != X)
                //          ? NONE : CLOCKWISE_180;
                // 这里直接按轴推导，避免为了拿一个 BlockState 去依赖注册表。
                String axis = parts[2];
                boolean flips = (mirror == Mirror.LEFT_RIGHT && axis.equals("z"))
                        || (mirror == Mirror.FRONT_BACK && axis.equals("x"));
                result.addProperty("value", (flips ? Rotation.CLOCKWISE_180 : Rotation.NONE).name());
            }
            case "posrot" -> {
                OctahedralGroup group = groupOfRotation(parts[1]);
                result.addProperty("value", applyGroup(group,
                        Integer.parseInt(parts[2]),
                        Integer.parseInt(parts[3]),
                        Integer.parseInt(parts[4])));
            }
            case "posmirror" -> {
                OctahedralGroup group = groupOfMirror(parts[1]);
                result.addProperty("value", applyGroup(group,
                        Integer.parseInt(parts[2]),
                        Integer.parseInt(parts[3]),
                        Integer.parseInt(parts[4])));
            }
            case "compose" -> {
                OctahedralGroup a = OctahedralGroup.valueOf(parts[1]);
                OctahedralGroup b = OctahedralGroup.valueOf(parts[2]);
                result.addProperty("value", applyGroup(a.compose(b), 1, 2, 3));
            }
            case "inverse" -> {
                OctahedralGroup group = OctahedralGroup.valueOf(parts[1]);
                result.addProperty("value", applyGroup(group.inverse(), 1, 2, 3));
            }
            case "inverts" -> {
                OctahedralGroup group = OctahedralGroup.valueOf(parts[1]);
                result.addProperty("value", group.inverts(axisOf(parts[2])) ? "true" : "false");
            }
            default -> throw new IllegalArgumentException("block_transform: 无法识别的单元: " + unit);
        }

        return result;
    }
}
