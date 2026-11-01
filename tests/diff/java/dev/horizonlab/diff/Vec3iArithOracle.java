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
import net.minecraft.core.Vec3i;

/**
 * {@code Vec3i} 整数算术的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.Vec3iArithOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Vec3i#compareTo(Vec3i)} / {@link Vec3i#distManhattan(Vec3i)} /
 * {@link Vec3i#distChessboard(Vec3i)} / {@link Vec3i#relative(Direction, int)} /
 * {@link Vec3i#multiply(int)} / {@link Vec3i#multiply(int, int, int)} /
 * {@link Vec3i#offset(int, int, int)} / {@link Vec3i#subtract(Vec3i)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/vec3i_arith.cpp 逐项一致。
 */
public final class Vec3iArithOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "compare|0,0,0|0,0,1",
                    "compare|0,0,1|0,0,0",
                    "compare|1,0,0|0,0,0",
                    "compare|0,0,0|1,0,0",
                    "compare|5,0,0|0,1,0",
                    "compare|0,1,0|5,0,0",
                    "compare|0,0,5|0,1,0",
                    "compare|1,2,3|1,2,3",
                    "compare|-1,-1,-1|0,0,0",
                    "compare|0,0,0|-1,-1,-1",
                    "manhattan|0,0,0|1,2,3",
                    "manhattan|0,0,0|-1,-2,-3",
                    "manhattan|5,3,7|2,8,4",
                    "manhattan|1,2,3|1,2,3",
                    "manhattan|0,0,0|8388608,0,0",
                    "manhattan|0,0,0|9000000,9000000,9000000",
                    "manhattan|0,0,0|16777216,0,0",
                    "manhattan|0,0,0|16777217,0,0",
                    "chessboard|0,0,0|1,2,3",
                    "chessboard|0,0,0|-1,-2,-3",
                    "chessboard|5,3,7|2,8,4",
                    "chessboard|0,0,0|9000000,9000000,9000000",
                    "relative|0,0,0|UP|1",
                    "relative|0,0,0|UP|0",
                    "relative|0,0,0|UP|-1",
                    "relative|5,3,7|NORTH|2",
                    "relative|5,3,7|EAST|-3",
                    "relative|0,0,0|DOWN|5",
                    "multiply|5,3,7|0",
                    "multiply|5,3,7|1",
                    "multiply|5,3,7|2",
                    "multiply|5,3,7|-1",
                    "multiply3|5,3,7|2,-1,0",
                    "offset|5,3,7|1,2,3",
                    "offset|5,3,7|-1,-2,-3",
                    "subtract|5,3,7|2,8,4",
                    "subtract|0,0,0|1,2,3");

    private Vec3iArithOracle() {}

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
                "usage: Vec3iArithOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static Vec3i parseVec(String text) {
        String[] parts = text.split(",", -1);
        if (parts.length != 3) {
            throw new IllegalArgumentException("vec3i_arith: 坐标必须三段: " + text);
        }
        return new Vec3i(Integer.parseInt(parts[0]), Integer.parseInt(parts[1]),
                Integer.parseInt(parts[2]));
    }

    private static String dumpVec(Vec3i v) {
        return v.getX() + "," + v.getY() + "," + v.getZ();
    }

    private static Direction findDirection(String name) {
        for (Direction dir : Direction.values()) {
            if (dir.name().equals(name)) {
                return dir;
            }
        }
        throw new IllegalArgumentException("vec3i_arith: 未知方向: " + name);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("vec3i_arith: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "compare" -> out.addProperty("value",
                    parseVec(parts[1]).compareTo(parseVec(parts[2])));
            case "manhattan" -> out.addProperty("value",
                    parseVec(parts[1]).distManhattan(parseVec(parts[2])));
            case "chessboard" -> out.addProperty("value",
                    parseVec(parts[1]).distChessboard(parseVec(parts[2])));
            case "relative" -> out.addProperty("value", dumpVec(
                    parseVec(parts[1]).relative(findDirection(parts[2]),
                            Integer.parseInt(parts[3]))));
            case "multiply" -> out.addProperty("value",
                    dumpVec(parseVec(parts[1]).multiply(Integer.parseInt(parts[2]))));
            case "multiply3" -> {
                Vec3i f = parseVec(parts[2]);
                out.addProperty("value", dumpVec(
                        parseVec(parts[1]).multiply(f.getX(), f.getY(), f.getZ())));
            }
            case "offset" -> {
                Vec3i d = parseVec(parts[2]);
                out.addProperty("value", dumpVec(
                        parseVec(parts[1]).offset(d.getX(), d.getY(), d.getZ())));
            }
            case "subtract" -> out.addProperty("value",
                    dumpVec(parseVec(parts[1]).subtract(parseVec(parts[2]))));
            default -> throw new IllegalArgumentException("vec3i_arith: 无法识别的单元: " + unit);
        }

        return out;
    }
}