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
 * 实体类型存档序列化与召唤开关的原版 oracle
 * （{@code EntityType#canSerialize} / {@code EntityType#canSummon}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntitySerializeSummonOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <pre>
 *   serialize 默认 true，显式 noSave() 才关：leash_knot / lightning_bolt / player / fishing_bobber
 *   summon    默认 true，原版没有任何实体关闭它
 * </pre>
 *
 * <p>这两个都是「默认值写反」型缺陷：不会崩溃、不会报错，只是某些实体悄悄失去
 * 存档或召唤能力。
 */
public final class EntitySerializeSummonOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EntitySerializeSummonOracle() {}

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
        System.err.println("usage: EntitySerializeSummonOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        out.addProperty("serialize", type.canSerialize());
        out.addProperty("summon", type.canSummon());
        return out;
    }
}
