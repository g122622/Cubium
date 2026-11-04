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
 * 实体类型在和平难度下是否允许生成/召唤的原版 oracle
 * （{@code EntityType#isAllowedInPeaceful}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntityPeacefulOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>原版靠构建器开关决定，名字容易看反：默认<b>允许</b>，
 * 需要显式 {@code notInPeaceful()} 才禁止。
 *
 * <p>反直觉之处：大多数 {@code MONSTER} 类别实体确实禁止，但并非全部 ——
 * 潜影贝、僵尸猪灵、猪灵、猪灵蛮兵、疣猪兽等都不带 {@code notInPeaceful}。
 *
 * <p>探针侧传入的单元名可能带 {@code minecraft:} 前缀，这里统一处理。
 */
public final class EntityPeacefulOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EntityPeacefulOracle() {}

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
        System.err.println("usage: EntityPeacefulOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        out.addProperty("allowedInPeaceful", type.isAllowedInPeaceful());
        return out;
    }
}
