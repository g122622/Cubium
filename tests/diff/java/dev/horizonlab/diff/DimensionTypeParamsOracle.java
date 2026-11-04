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
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.dimension.DimensionType;
import net.minecraft.world.level.biome.Climate;

/**
 * 维度类型参数（{@code DimensionType}）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DimensionTypeParamsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 直接从原版注册表取 {@code DimensionType} 实例，逐字段输出。字段名与原版 record
 * 组件一致；{@code monster_spawn_light_level} 是 IntProvider，拆成 min/max 两项，
 * 常数 IntProvider 的 min == max。
 */
public final class DimensionTypeParamsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] DIMENSIONS = {
        "minecraft:overworld", "minecraft:the_nether", "minecraft:the_end"
    };

    private static final String[] FIELDS = {
        "has_fixed_time",
        "has_skylight",
        "has_ceiling",
        "coordinate_scale",
        "min_y",
        "height",
        "logical_height",
        "infiniburn",
        "ambient_light",
        "monster_spawn_light_level_min",
        "monster_spawn_light_level_max",
        "monster_spawn_block_light_limit",
        "skybox",
        "cardinal_light_type",
    };

    private DimensionTypeParamsOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String dimension : DIMENSIONS) {
                for (String field : FIELDS) {
                    units.add(dimension + "|" + field);
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: DimensionTypeParamsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        DimensionType type = lookup(parts[0]);
        if (type == null) {
            out.addProperty("missing", true);
            return out;
        }

        switch (parts[1]) {
            case "has_fixed_time" -> out.addProperty("value", type.hasFixedTime());
            case "has_skylight" -> out.addProperty("value", type.hasSkyLight());
            case "has_ceiling" -> out.addProperty("value", type.hasCeiling());
            case "coordinate_scale" -> out.addProperty("value", bits((float) type.coordinateScale()));
            case "min_y" -> out.addProperty("value", type.minY());
            case "height" -> out.addProperty("value", type.height());
            case "logical_height" -> out.addProperty("value", type.logicalHeight());
            case "infiniburn" -> out.addProperty("value", "#" + type.infiniburn().location());
            case "ambient_light" -> out.addProperty("value", bits(type.ambientLight()));
            case "monster_spawn_light_level_min" -> out.addProperty("value", sampleMin(type));
            case "monster_spawn_light_level_max" -> out.addProperty("value", sampleMax(type));
            case "monster_spawn_block_light_limit" ->
                out.addProperty("value", type.monsterSpawnBlockLightLimit());
            case "skybox" -> out.addProperty("value", type.skybox().getSerializedName());
            case "cardinal_light_type" ->
                out.addProperty("value", type.cardinalLightType().getSerializedName());
            default -> throw new IllegalArgumentException("unknown field: " + parts[1]);
        }
        return out;
    }

    /**
     * IntProvider 的取值区间。常数 IntProvider 的 min 与 max 相同。
     *
     * <p>探针跑 1000 次采样取极值——这是与实现无关的黑盒取值方式，避免依赖
     * 具体 IntProvider 子类的内部字段。
     */
    private static int sampleMin(DimensionType type) {
        return sampleRange(type)[0];
    }

    private static int sampleMax(DimensionType type) {
        return sampleRange(type)[1];
    }

    private static int[] sampleRange(DimensionType type) {
        net.minecraft.util.RandomSource random = net.minecraft.util.RandomSource.create(12345L);
        int min = Integer.MAX_VALUE;
        int max = Integer.MIN_VALUE;
        for (int i = 0; i < 1000; i++) {
            int value = type.monsterSpawnLightTest().sample(random);
            min = Math.min(min, value);
            max = Math.max(max, value);
        }
        return new int[] {min, max};
    }

    /**
     * 从内置数据包加载的注册表里取 {@code dimension_type}。
     *
     * <p>{@code dimension_type} 是**数据包注册表**，不在 BuiltInRegistries 里，
     * 必须经 {@link DataPackRegistries} 加载。
     */
    private static DimensionType lookup(String name) {
        Identifier id = Identifier.tryParse(name);
        if (id == null) {
            return null;
        }
        if (REGISTRIES == null) {
            REGISTRIES = DataPackRegistries.load().access;
        }
        var registry = REGISTRIES.lookup(Registries.DIMENSION_TYPE).orElse(null);
        if (registry == null) {
            return null;
        }
        return registry.get(net.minecraft.resources.ResourceKey.create(Registries.DIMENSION_TYPE, id))
            .map(h -> h.value())
            .orElse(null);
    }

    private static net.minecraft.core.RegistryAccess REGISTRIES;

    private static String bits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }
}
