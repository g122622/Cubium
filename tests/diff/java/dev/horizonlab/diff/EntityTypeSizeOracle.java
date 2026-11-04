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
import net.minecraft.world.entity.EntityDimensions;
import net.minecraft.world.entity.EntityType;

/**
 * 实体类型注册尺寸（宽 / 高 / 眼睛高度 / 是否固定）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntityTypeSizeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 输出浮点的位模式（{@code Float.floatToRawIntBits}），与 C++ 侧 {@code floatBits} 同构，
 * 避免两侧十进制最短表示的格式差异制造伪差异。
 */
public final class EntityTypeSizeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EntityTypeSizeOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            BuiltInRegistries.ENTITY_TYPE.keySet().stream().map(Object::toString).sorted().forEach(units::add);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EntityTypeSizeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            String unit = obj.get("unit").getAsString();
            results.add(GSON.toJson(solve(unit)));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        // 本版本里资源位置类型叫 Identifier（不是旧名 ResourceLocation）。
        var id = net.minecraft.resources.Identifier.tryParse(unit);
        EntityType<?> type = id == null ? null : BuiltInRegistries.ENTITY_TYPE.get(id).map(h -> h.value()).orElse(null);
        if (type == null) {
            out.addProperty("missing", true);
            return out;
        }

        EntityDimensions dims = type.getDimensions();
        out.addProperty("width", bits(dims.width()));
        out.addProperty("height", bits(dims.height()));
        out.addProperty("eyeHeight", bits(dims.eyeHeight()));
        out.addProperty("fixed", dims.fixed());
        return out;
    }

    /** 浮点位模式，格式与 C++ 侧 {@code "0x%08x"} 完全一致。 */
    private static String bits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }
}
