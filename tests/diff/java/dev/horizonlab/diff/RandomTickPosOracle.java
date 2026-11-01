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
import net.minecraft.core.BlockPos;

/**
 * 随机刻坐标采样的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RandomTickPosOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么这里复刻而不是直接调用</h2>
 *
 * <p>原版 {@code Level.getBlockRandomPos} 是实例方法，而 {@code Level} 是抽象类，正常构造需要
 * `LevelData`、`DimensionType`、大量钩子对象。尝试用
 * {@code sun.misc.Unsafe.allocateInstance} 分配未初始化实例，在 JDK 的模块系统下被拒绝
 * （`IllegalStateException: 无法分配 Level 实例`）。
 *
 * <p>因此本 oracle **逐字符复刻**原版方法体，用原版的 {@link BlockPos} 构造结果：
 *
 * <pre>{@code
 * public BlockPos getBlockRandomPos(int p_46497_, int p_46498_, int p_46499_, int p_46500_) {
 *     this.randValue = this.randValue * 3 + 1013904223;
 *     int i = this.randValue >> 2;
 *     return new BlockPos(p_46497_ + (i & 15), p_46498_ + (i >> 16 & p_46500_),
 *                         p_46499_ + (i >> 8 & 15));
 * }
 * }</pre>
 *
 * <p>复刻的前提是这段代码足够短且无外部依赖——它只用 `int` 运算与 `BlockPos` 构造，没有调用
 * 其他方法。风险在于「读源码片段导致误判」这一已知问题，但这里读的是**完整方法体**（4 行），
 * 已确认没有遗漏。
 *
 * <p>单元表必须与 probes/random_tick_pos.cpp 逐项一致。
 */
public final class RandomTickPosOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "0|0|0|15|0|1",
                    "0|0|0|15|1|1",
                    "0|0|0|15|-1|1",
                    "0|0|0|15|12345|1",
                    "0|0|0|15|0|2",
                    "0|0|0|15|0|3",
                    "0|0|0|15|0|5",
                    "0|0|0|15|0|8",
                    "100|64|200|15|0|3",
                    "-100|-64|-200|15|0|3",
                    "0|0|0|15|777|3",
                    "0|0|0|7|777|3",
                    "0|0|0|3|777|3",
                    "0|0|0|15|2147483647|3",
                    "0|0|0|15|-2147483648|3",
                    "0|0|0|15|42|4");

    private RandomTickPosOracle() {}

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
                "usage: RandomTickPosOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /**
     * LCG 状态容器，逐字符复刻 {@code Level.getBlockRandomPos} 的方法体。
     */
    private static final class LevelStub {
        private int randValue;

        LevelStub(int initial) {
            this.randValue = initial;
        }

        /** 与 {@code Level.getBlockRandomPos} 的方法体逐字符相同。 */
        BlockPos getBlockRandomPos(int p_46497_, int p_46498_, int p_46499_, int p_46500_) {
            this.randValue = this.randValue * 3 + 1013904223;
            int i = this.randValue >> 2;
            return new BlockPos(
                    p_46497_ + (i & 15),
                    p_46498_ + (i >> 16 & p_46500_),
                    p_46499_ + (i >> 8 & 15));
        }

        int randValue() {
            return this.randValue;
        }
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 6) {
            throw new IllegalArgumentException(
                    "random_tick_pos: 单元应为 6 段: <x>|<y>|<z>|<maxY>|<seed>|<count>: " + unit);
        }

        int x = Integer.parseInt(parts[0]);
        int y = Integer.parseInt(parts[1]);
        int z = Integer.parseInt(parts[2]);
        int maxY = Integer.parseInt(parts[3]);
        int seed = (int) Long.parseLong(parts[4]);
        int count = Integer.parseInt(parts[5]);

        LevelStub level = new LevelStub(seed);

        StringBuilder positions = new StringBuilder();
        for (int k = 0; k < count; k++) {
            BlockPos pos = level.getBlockRandomPos(x, y, z, maxY);
            if (positions.length() > 0) {
                positions.append(';');
            }
            positions.append(pos.getX()).append(',').append(pos.getY()).append(',')
                    .append(pos.getZ());
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("positions", positions.toString());
        result.addProperty("randValueAfter", Integer.toString(level.randValue()));
        return result;
    }
}