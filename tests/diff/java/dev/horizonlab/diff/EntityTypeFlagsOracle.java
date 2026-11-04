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
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.entity.EntityType;

/**
 * 实体类型的刷怪分类、网络追踪参数与环境免疫的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntityTypeFlagsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>五项：{@code MobCategory} / {@code clientTrackingRange} / {@code updateInterval} /
 * {@code fireImmune} / {@code immuneToLava}。
 *
 * <p>分类是重灾区：原版 8 类的生物上限完全不同
 * （monster 70 / creature 10 / ambient 15 / axolotls 5 / underground_water_creature 5 /
 * water_creature 5 / water_ambient 20 / misc -1）。把美西螈错归进水生生物，
 * 就会和鱿鱼抢名额，表现为「某种生物怎么都不刷」。
 */
public final class EntityTypeFlagsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EntityTypeFlagsOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.ENTITY_TYPE.keySet()) {
                units.add(id.toString());
            }
            java.util.Collections.sort(units);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EntityTypeFlagsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 探针侧 EntityType::name() 返回裸路径，补齐 minecraft: 前缀。 */
    private static Identifier toId(String unit) {
        return Identifier.tryParse(unit.indexOf(':') >= 0 ? unit : "minecraft:" + unit);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        Identifier id = toId(unit);
        EntityType<?> type = id == null
            ? null
            : BuiltInRegistries.ENTITY_TYPE.get(id).map(h -> h.value()).orElse(null);
        if (type == null) {
            out.addProperty("missing", true);
            return out;
        }

        // 原版分类名来自 MobCategory#getName（monster / creature / ambient / ...）
        out.addProperty("classification", type.getCategory().getName());
        out.addProperty("trackingRange", type.clientTrackingRange());
        out.addProperty("updateInterval", type.updateInterval());
        out.addProperty("fireImmune", type.fireImmune());

        return out;
    }
}
