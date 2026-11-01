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
import java.util.Locale;
import net.minecraft.core.Direction;

/**
 * 流体扩散方向选择的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.FluidSpreadDirOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>方向遍历顺序**直接用原版 {@link Direction.Plane#HORIZONTAL}**，而不是手写常量——
 * 这是本能力的核心：`Plane.HORIZONTAL` 的数组顺序（`NORTH, EAST, SOUTH, WEST`）与
 * `Direction` 的枚举声明顺序（`DOWN, UP, NORTH, SOUTH, WEST, EAST`）不同。
 *
 * <p>选择逻辑复刻 {@code FlowingFluid.getSpread} 里的 `best` 比较：
 * <pre>{@code
 * int best = 1000;
 * for (Direction dir : Direction.Plane.HORIZONTAL) {
 *     int dist = ...;
 *     if (dist < best) { map.clear(); }
 *     if (dist <= best) { map.put(dir, ...); }
 *     best = dist;
 * }
 * }</pre>
 *
 * <p>单元表必须与 probes/fluid_spread_dir.cpp 逐项一致。
 */
public final class FluidSpreadDirOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "water|3,3,3,3",
                    "water|1,5,5,5",
                    "water|5,1,5,5",
                    "water|5,5,1,5",
                    "water|5,5,5,1",
                    "water|1,1,5,5",
                    "water|1,5,1,5",
                    "water|5,1,5,1",
                    "water|5,5,1,1",
                    "water|1,1,1,5",
                    "water|5,1,1,1",
                    "water|0,5,5,5",
                    "water|5,0,5,5",
                    "water|0,0,5,5",
                    "water|5,5,5,0",
                    "water|0,0,0,0",
                    "water|-1,5,5,5",
                    "water|-1,-1,5,5",
                    "water|-1,-1,-1,5",
                    "water|-1,-1,-1,-1",
                    "water|-1,0,-1,-1",
                    "water|1000,1000,1000,1000",
                    "water|10,5,3,1",
                    "water|1,3,5,10",
    // 遍历顺序敏感性：NORTH 最远、WEST 最近。若按枚举声明顺序（N,S,W,E）遍历，
    // best 被抬高的时机不同，保留集合会变。
    "water|5,1,1,1",
    // 顺序：让 best 在中间被抬高一次
    "water|1,9,1,9",
    // 两个不同距离交替
    "water|2,2,7,7",
    "water|7,7,2,2",
    // 递增 / 递减（最佳在首尾）
    "water|1,2,3,4",
                    "water|4,3,2,1");
    private FluidSpreadDirOracle() {}

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
                "usage: FluidSpreadDirOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", 2);
        if (parts.length != 2) {
            throw new IllegalArgumentException("fluid_spread_dir: 单元应为 '<流体>|n,e,s,w': " + unit);
        }

        String[] raw = parts[1].split(",", -1);
        if (raw.length != 4) {
            throw new IllegalArgumentException("fluid_spread_dir: 需要 4 个方向距离: " + unit);
        }
        int[] distances = new int[4];
        for (int i = 0; i < 4; i++) {
            distances[i] = Integer.parseInt(raw[i]);
        }

        // 用原版 Plane.HORIZONTAL 的遍历顺序，把 4 个距离映射到方向上。
        // 单元的 n,e,s,w 顺序键到原版的方向名。
        int best = 1000;
        List<String> kept = new ArrayList<>();

        int index = 0;
        for (Direction dir : Direction.Plane.HORIZONTAL) {
            int slot = slotOf(dir);
            int dist = distances[slot];
            index++;
            if (dist < 0) {
                continue; // 不可通行
            }
            if (dist < best) {
                kept.clear();
            }
            if (dist <= best) {
                kept.add(dir.getName().toLowerCase(Locale.ROOT));
            }
            best = dist;
        }
        if (index != 4) {
            throw new IllegalStateException("fluid_spread_dir: Plane.HORIZONTAL 应有 4 个方向");
        }

        String summary = kept.isEmpty() ? "-" : String.join(",", kept);

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("value", summary);
        result.addProperty("count", kept.size());
        return result;
    }

    /** 把方向映射到单元的 `n,e,s,w` 槽位。 */
    private static int slotOf(Direction dir) {
        switch (dir) {
            case NORTH:
                return 0;
            case EAST:
                return 1;
            case SOUTH:
                return 2;
            case WEST:
                return 3;
            default:
                throw new IllegalArgumentException("fluid_spread_dir: 非水平方向 " + dir);
        }
    }
}
