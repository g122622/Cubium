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
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.XoroshiroRandomSource;

/**
 * 随机数生成器逐位序列的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RandomSequenceOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元形如 {@code xoroshiro128pp:42:7}（算法:种子:预热次数）。调用顺序与
 * probes/random_sequence.cpp 完全一致——随机数是有状态的，只要有一侧少调一次，
 * 后面整段都会错位，这本身就是要比对的内容。
 */
public final class RandomSequenceOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    static final String[] ALGORITHMS = {"xoroshiro128pp", "java_legacy"};
    static final long[] SEEDS = {0L, 1L, 42L, 123456789L, -1L, 9007199254740993L};
    static final int[] WARMUPS = {0, 7};
    static final int[] INT_BOUNDS = {1, 2, 7, 16, 100, 1000, 2147483647};

    private RandomSequenceOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String algorithm : ALGORITHMS) {
                for (long seed : SEEDS) {
                    for (int warmup : WARMUPS) {
                        text.append(algorithm).append(':').append(seed).append(':').append(warmup).append('\n');
                    }
                }
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: RandomSequenceOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 原版 nextBits(bits)：Xoroshiro 取 nextLong() 的高 bits 位，Legacy 走 next(bits)。 */
    private static int nextBits(RandomSource random, int bits) {
        if (random instanceof XoroshiroRandomSource xoroshiro) {
            return (int) (xoroshiro.nextLong() >>> (64 - bits));
        }
        if (random instanceof LegacyRandomSource legacy) {
            // 原版 LegacyRandomSource.next(int bits)：next(bits) 是 nextInt(bits) 的一半精度，
            // java.util.Random 的 next(int) 是 protected，原版把它挪到了 private 方法里，
            // 这里用 nextInt(1 << bits) 走同一条「取高 bits 位」路径（2 的幂分支）。
            return legacy.nextInt(1 << bits);
        }
        throw new IllegalStateException("unknown random source: " + random);
    }

    private static RandomSource create(String algorithm, long seed) {
        if (algorithm.equals("xoroshiro128pp")) {
            // 原版：new XoroshiroRandomSource(seed)
            return new XoroshiroRandomSource(seed);
        }
        if (algorithm.equals("java_legacy")) {
            // 原版 LegacyRandomSource：内部持有 java.util.Random 的 LCG 状态。
            // 它的 nextInt(bound)/nextInt() 与 RandomSource 接口的其他方法共用同一个
            // 「next(bits)」原语，所以序列与 XoroshiroRandomSource 不可互换——这正是要分开测的原因。
            return new LegacyRandomSource(seed);
        }
        return null;
    }

    private static JsonObject summarise(String unit) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        int lastSep = unit.lastIndexOf(':');
        int firstSep = unit.indexOf(':');
        if (firstSep == lastSep) {
            result.addProperty("missing", true);
            return result;
        }
        String algorithm = unit.substring(0, firstSep);
        long seed = Long.parseLong(unit.substring(firstSep + 1, lastSep));
        int warmup = Integer.parseInt(unit.substring(lastSep + 1));

        RandomSource random = create(algorithm, seed);
        if (random == null) {
            result.addProperty("missing", true);
            return result;
        }
        for (int i = 0; i < warmup; ++i) {
            random.nextLong();
        }

        List<Long> longs = new ArrayList<>();
        List<Integer> ints = new ArrayList<>();
        List<List<Integer>> bounded = new ArrayList<>();
        for (int ignored : INT_BOUNDS) {
            bounded.add(new ArrayList<>());
        }
        List<String> floats = new ArrayList<>();
        List<String> doubles = new ArrayList<>();
        List<Integer> narrow = new ArrayList<>();

        for (int i = 0; i < 8; ++i) {
            longs.add(random.nextLong());
            ints.add(random.nextInt());
            for (int b = 0; b < INT_BOUNDS.length; ++b) {
                bounded.get(b).add(random.nextInt(INT_BOUNDS[b]));
            }
            floats.add(String.format("0x%08x", Float.floatToRawIntBits(random.nextFloat())));
            doubles.add(String.format("0x%016x", Double.doubleToRawLongBits(random.nextDouble())));
            // 窄位宽抽取：原版 nextBits(b) 对 Xoroshiro 是 (int)(nextLong() >>> 64 - b)，
            // 对 Legacy 是 next(b)；RandomSource 没有公开 nextBits，按各实现的特化手写。
            narrow.add(nextBits(random, (i % 8) + 1));
        }

        JsonArray boundedJson = new JsonArray();
        for (List<Integer> column : bounded) {
            JsonArray array = new JsonArray();
            for (int value : column) {
                array.add(value);
            }
            boundedJson.add(array);
        }

        result.add("long", GSON.toJsonTree(longs));
        result.add("int", GSON.toJsonTree(ints));
        result.add("bounded", boundedJson);
        result.add("narrow", GSON.toJsonTree(narrow));
        result.add("float", GSON.toJsonTree(floats));
        result.add("double", GSON.toJsonTree(doubles));
        return result;
    }
}