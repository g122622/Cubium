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
import net.minecraft.util.RandomSource;

/**
 * 怪物生成光照判定（{@code Monster.isDarkEnoughToSpawn}）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MobSpawnLightLevelOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么不直接调原版方法</h2>
 *
 * {@code Monster.isDarkEnoughToSpawn(ServerLevelAccessor, BlockPos, RandomSource)} 需要真实
 * 世界来读光照。本能力把该方法的**三段判定逻辑**完整搬到参数上，逐行对应原版
 * {@code Monster.java:86-99}：
 *
 * <pre>
 *   if (getBrightness(SKY,   pos) > random.nextInt(32))            return false;
 *   int limit = dimensionType.monsterSpawnBlockLightLimit();
 *   if (limit < 15 && getBrightness(BLOCK, pos) > limit)           return false;
 *   int j = level.isThundering()
 *       ? getMaxLocalRawBrightness(pos, 10)
 *       : getMaxLocalRawBrightness(pos);
 *   return j <= dimensionType.monsterSpawnLightTest().sample(random);
 * </pre>
 *
 * 随机源用 {@link RandomSource#create} 固定种子，保证与 C++ 侧同流。
 */
public final class MobSpawnLightLevelOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与 C++ 探针 writeList 完全同构的场景表。 */
    private static final String[] SCENES = {
        "0|0|0|0|0|0|7|1",
        "0|0|0|0|0|0|7|2",
        "0|1|1|1|0|0|7|3",
        "5|0|5|5|0|0|7|4",
        "15|0|15|15|0|0|7|5",
        "31|0|15|15|0|0|7|6",
        "0|0|7|7|0|0|7|7",
        "0|0|8|8|0|0|7|8",
        "0|15|15|15|15|7|7|9",
        "0|15|7|7|15|7|7|10",
        "0|0|7|7|15|7|7|11",
        "0|0|15|15|0|15|15|12",
        "0|0|0|0|0|15|15|13",
        "0|14|14|14|0|15|15|14",
        "0|0|0|15|0|0|7|15",
        "0|0|15|0|0|0|7|16",
        "0|14|14|14|14|14|14|17",
        "0|15|15|15|14|14|14|18",
        "0|15|15|15|15|14|14|19",
    };

    private MobSpawnLightLevelOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            Files.write(Path.of(args[1]), new ArrayList<>(List.of(SCENES)), StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: MobSpawnLightLevelOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            results.add(GSON.toJson(solve(obj.get("unit").getAsString())));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String[] parts = unit.split("\\|", -1);
        int sky = Integer.parseInt(parts[0]);
        int block = Integer.parseInt(parts[1]);
        int combined = Integer.parseInt(parts[2]);
        int thunderCombined = Integer.parseInt(parts[3]);
        int limit = Integer.parseInt(parts[4]);
        int lightMin = Integer.parseInt(parts[5]);
        int lightMax = Integer.parseInt(parts[6]);
        long seed = Long.parseLong(parts[7]);

        RandomSource random = RandomSource.create(seed);

        // 第 1 段：天空光与 nextInt(32) 比。
        if (sky > random.nextInt(32)) {
            out.addProperty("dark", false);
            out.addProperty("lightTestSample", -1);
            return out;
        }

        // 第 2 段：方块光上限，仅在 limit < 15 时检查。
        if (limit < 15 && block > limit) {
            out.addProperty("dark", false);
            out.addProperty("lightTestSample", -1);
            return out;
        }

        // 第 3 段：综合亮度与 lightTest 采样值比。
        boolean thunder = thunderCombined != combined;
        int value = thunder ? thunderCombined : combined;
        // UniformInt.sample(random) = min + random.nextInt(max - min + 1)
        int sample = lightMin + random.nextInt(lightMax - lightMin + 1);
        out.addProperty("dark", value <= sample);

        // 同口径重放，输出采样值便于区分「随机流不同」与「判定不同」。
        RandomSource replay = RandomSource.create(seed);
        replay.nextInt(32);
        out.addProperty("lightTestSample", lightMin + replay.nextInt(lightMax - lightMin + 1));
        return out;
    }
}
