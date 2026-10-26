package dev.horizonlab.diff;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.util.random.Weighted;
import net.minecraft.util.random.WeightedList;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.MobCategory;
import net.minecraft.world.level.biome.Biome;
import net.minecraft.world.level.biome.MobSpawnSettings;

/**
 * 生物群系自然生成配置的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BiomeSpawnsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/biome_spawns.cpp 同构：输出 {@code {"biome": ..., "categories": {...}}}，
 * 每个分类给出生成条目（生物类型、权重、成组数量）与每区块上限，另有
 * {@code creature_spawn_probability} 与带生成成本的实体。
 */
public final class BiomeSpawnsOracle {
    private BiomeSpawnsOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String id : ids()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BiomeSpawnsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    /** 服务端加载到的全部生物群系 ID（字典序）。 */
    private static List<String> ids() {
        var biomes = LootServerData.get().resources.fullRegistries()
            .lookup()
            .lookupOrThrow(Registries.BIOME);
        List<String> ids = new ArrayList<>();
        biomes.listElementIds().forEach(key -> ids.add(key.identifier().toString()));
        ids.sort(Comparator.naturalOrder());
        return ids;
    }

    private static void run(Path input, Path output) throws IOException {
        var biomes = LootServerData.get().resources.fullRegistries()
            .lookup()
            .lookupOrThrow(Registries.BIOME);

        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        StringBuilder out = new StringBuilder();
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String id = request.get("biome").getAsString();
            ResourceKey<Biome> key = ResourceKey.create(Registries.BIOME, Identifier.parse(id));
            Biome biome = biomes.get(key).map(holder -> holder.value()).orElse(null);
            out.append(summarise(id, biome)).append('\n');
        }
        Files.writeString(output, out.toString(), StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String id, Biome biome) {
        JsonObject result = new JsonObject();
        result.addProperty("biome", id);
        if (biome == null) {
            result.addProperty("missing", true);
            return result;
        }
        MobSpawnSettings settings = biome.getMobSettings();

        JsonObject categories = new JsonObject();
        for (MobCategory category : MobCategory.values()) {
            JsonArray entries = new JsonArray();
            WeightedList<MobSpawnSettings.SpawnerData> list = settings.getMobs(category);
            for (Weighted<MobSpawnSettings.SpawnerData> weighted : list.unwrap()) {
                MobSpawnSettings.SpawnerData data = weighted.value();
                JsonObject entry = new JsonObject();
                entry.addProperty("type", BuiltInRegistries.ENTITY_TYPE.getKey(data.type()).toString());
                entry.addProperty("weight", weighted.weight());
                entry.addProperty("minCount", data.minCount());
                entry.addProperty("maxCount", data.maxCount());
                entries.add(entry);
            }
            sortEntries(entries);

            JsonObject categoryJson = new JsonObject();
            categoryJson.addProperty("maxInstances", category.getMaxInstancesPerChunk());
            categoryJson.add("spawns", entries);
            categories.add(category.getName(), categoryJson);
        }
        result.add("categories", categories);
        // float 按 double 提升后写出：探针侧也是 f32 → double，两侧比的是同一个位模式
        // （0.1f 就是 0.10000000149011612，而不是字面量常见的 0.1）。
        result.addProperty("creature_spawn_probability", (double) settings.getCreatureProbability());

        // spawn_costs 没有公开的映射视图，只能按实体类型逐个查询：遍历内置实体注册表。
        JsonObject costs = new JsonObject();
        for (var entityId : BuiltInRegistries.ENTITY_TYPE.keySet()) {
            EntityType<?> type = BuiltInRegistries.ENTITY_TYPE.getValue(entityId);
            var cost = settings.getMobSpawnCost(type);
            if (cost == null) {
                continue;
            }
            JsonObject costJson = new JsonObject();
            costJson.addProperty("charge", cost.charge());
            costJson.addProperty("energy_budget", cost.energyBudget());
            costs.add(BuiltInRegistries.ENTITY_TYPE.getKey(type).toString(), costJson);
        }
        result.add("spawn_costs", costs);
        return result;
    }

    private static void sortEntries(JsonArray entries) {
        List<JsonObject> sorted = new ArrayList<>();
        entries.forEach(element -> sorted.add(element.getAsJsonObject()));
        sorted.sort(Comparator.comparing((JsonObject e) -> e.get("type").getAsString())
                .thenComparingInt(e -> e.get("weight").getAsInt())
                .thenComparingInt(e -> e.get("minCount").getAsInt())
                .thenComparingInt(e -> e.get("maxCount").getAsInt()));
        while (entries.size() > 0) {
            entries.remove(0);
        }
        sorted.forEach(entries::add);
    }
}