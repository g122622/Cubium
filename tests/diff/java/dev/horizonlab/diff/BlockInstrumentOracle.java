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
import net.minecraft.world.level.block.Block;

/**
 * 方块音符盒乐器的原版 oracle（{@code BlockStateBase#instrument()}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockInstrumentOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 输出的是原版 {@code NoteBlockInstrument} 枚举的**序列化名**（如 {@code basedrum}、
 * {@code iron_xylophone}），而不是整数下标——两侧枚举声明顺序未必一致，按下标比会产生伪差异。
 */
public final class BlockInstrumentOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockInstrumentOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                units.add(id.toString());
            }
            units.sort(String::compareTo);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockInstrumentOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        Identifier id = Identifier.tryParse(unit);
        Block block = id == null
            ? null
            : BuiltInRegistries.BLOCK.get(id).map(h -> h.value()).orElse(null);
        if (block == null) {
            out.addProperty("missing", true);
            return out;
        }

        out.addProperty("instrument", block.defaultBlockState().instrument().getSerializedName());
        return out;
    }
}
