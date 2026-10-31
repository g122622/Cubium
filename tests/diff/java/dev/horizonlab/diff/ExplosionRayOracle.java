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
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import net.minecraft.util.RandomSource;

/**
 * 爆炸射线破坏判定的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ExplosionRayOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>复刻 {@code ServerExplosion.calculateExplodedPositions()} 的射线步进，用「以中心为
 * 原点的均匀抗性体素」代替真实世界。场景表与 probes/explosion_ray.cpp 必须逐项一致。
 */
public final class ExplosionRayOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 一层同心壳：距离小于 limit 的方块使用该抗性。 */
    private record Shell(float limit, float resistance) {}

    /** 一个爆炸场景。 */
    private record Scene(String name, float radius, List<Shell> shells) {}

    /** 场景表，必须与 C++ 侧 SCENES 完全一致。 */
    private static final List<Scene> SCENES =
            List.of(
                    new Scene("tnt", 4.0F, List.of(new Shell(64.0F, 0.3F))),
                    new Scene("tnt_air", 4.0F, List.of(new Shell(64.0F, 0.0F))),
                    new Scene("creeper", 3.0F, List.of(new Shell(2.0F, 0.0F), new Shell(64.0F, 0.3F))),
                    new Scene("obsidian", 6.0F, List.of(new Shell(4.0F, 0.0F), new Shell(64.0F, 1200.0F))),
                    new Scene(
                            "mixed",
                            5.0F,
                            List.of(
                                    new Shell(2.0F, 0.0F),
                                    new Shell(3.0F, 1.5F),
                                    new Shell(6.0F, 30.0F),
                                    new Shell(64.0F, 1200.0F))),
                    new Scene(
                            "charged",
                            8.0F,
                            List.of(
                                    new Shell(3.0F, 0.0F),
                                    new Shell(10.0F, 2.0F),
                                    new Shell(64.0F, 500.0F))));

    /** 原版常量。 */
    private static final float RAY_STEP_SIZE = 0.3F;

    private static final float STRENGTH_DECAY = 0.22500001F;
    private static final float RESISTANCE_COEFFICIENT = 0.3F;

    private ExplosionRayOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (Scene scene : SCENES) {
                text.append(scene.name()).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: ExplosionRayOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            var obj = JsonParser.parseString(line).getAsJsonObject();
            String name = obj.get("scene").getAsString();
            String seedText = obj.get("seed").getAsString();
            text.append(GSON.toJson(summarise(name, Long.parseLong(seedText)))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String name, long seed) {
        JsonObject result = new JsonObject();
        result.addProperty("scene", name);

        Scene scene =
                SCENES.stream().filter(s -> s.name().equals(name)).findFirst().orElse(null);
        if (scene == null) {
            result.addProperty("missing", true);
            return result;
        }

        var destroyed = calculateExplodedPositions(scene, seed);

        result.addProperty("seed", Long.toString(seed));
        result.addProperty("radius", formatFloat(scene.radius()));

        // 初始强度单独算一次（与射线无关），方便定位问题。
        RandomSource probeRandom = RandomSource.create(seed);
        float firstStrength = scene.radius() * (0.7F + probeRandom.nextFloat() * 0.6F);
        result.addProperty("firstStrength", formatFloat(firstStrength));
        result.addProperty("destroyedCount", destroyed.size());

        JsonArray first = new JsonArray();
        for (int i = 0; i < destroyed.size() && i < 8; i++) {
            first.add(toArray(destroyed.get(i)));
        }
        result.add("firstDestroyed", first);

        JsonArray last = new JsonArray();
        for (int i = Math.max(0, destroyed.size() - 8); i < destroyed.size(); i++) {
            last.add(toArray(destroyed.get(i)));
        }
        result.add("lastDestroyed", last);
        return result;
    }

    private static JsonArray toArray(int[] pos) {
        JsonArray array = new JsonArray();
        array.add(pos[0]);
        array.add(pos[1]);
        array.add(pos[2]);
        return array;
    }

    private static float resistanceAt(Scene scene, double x, double y, double z) {
        float distance = (float) Math.sqrt(x * x + y * y + z * z);
        for (Shell shell : scene.shells()) {
            if (distance < shell.limit()) {
                return shell.resistance();
            }
        }
        return 10000.0F;
    }

    /** 复刻 {@code ServerExplosion.calculateExplodedPositions()}。 */
    private static List<int[]> calculateExplodedPositions(Scene scene, long seed) {
        RandomSource random = RandomSource.create(seed);
        Set<List<Integer>> set = new HashSet<>();

        for (int j = 0; j < 16; j++) {
            for (int k = 0; k < 16; k++) {
                for (int l = 0; l < 16; l++) {
                    if (j != 0 && j != 15 && k != 0 && k != 15 && l != 0 && l != 15) {
                        continue;
                    }

                    double d0 = j / 15.0F * 2.0F - 1.0F;
                    double d1 = k / 15.0F * 2.0F - 1.0F;
                    double d2 = l / 15.0F * 2.0F - 1.0F;
                    double d3 = Math.sqrt(d0 * d0 + d1 * d1 + d2 * d2);
                    d0 /= d3;
                    d1 /= d3;
                    d2 /= d3;

                    float f = scene.radius() * (0.7F + random.nextFloat() * 0.6F);
                    double x = 0.0;
                    double y = 0.0;
                    double z = 0.0;

                    for (float f1 = 0.3F; f > 0.0F; f -= STRENGTH_DECAY) {
                        int bx = (int) Math.floor(x);
                        int by = (int) Math.floor(y);
                        int bz = (int) Math.floor(z);

                        float resistance = resistanceAt(scene, bx + 0.5, by + 0.5, bz + 0.5);
                        if (resistance > 0.0F) {
                            f -= (resistance + RESISTANCE_COEFFICIENT) * RESISTANCE_COEFFICIENT;
                        }

                        if (f > 0.0F && resistance > 0.0F && resistance < 10000.0F) {
                            set.add(List.of(bx, by, bz));
                        }

                        x += d0 * RAY_STEP_SIZE;
                        y += d1 * RAY_STEP_SIZE;
                        z += d2 * RAY_STEP_SIZE;
                    }
                }
            }
        }

        List<int[]> sorted = new ArrayList<>();
        for (List<Integer> entry : set) {
            sorted.add(new int[] {entry.get(0), entry.get(1), entry.get(2)});
        }
        sorted.sort(
                Comparator.<int[]>comparingInt(a -> a[0])
                        .thenComparingInt(a -> a[1])
                        .thenComparingInt(a -> a[2]));
        return sorted;
    }

    private static String formatFloat(float value) {
        String text = String.format("%.6g", (double) value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }
}
