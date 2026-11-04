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
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 游戏事件通知半径（{@code GameEvent.notificationRadius}）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.GameEventRadiusOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 直接遍历 {@code BuiltInRegistries.GAME_EVENT}。60 个事件里 57 个是默认半径 16，
 * 只有 jukebox_play / jukebox_stop_play（10）与 shriek（32）不同。
 */
public final class GameEventRadiusOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private GameEventRadiusOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.GAME_EVENT.keySet()) {
                // 探针侧用不带命名空间的事件名（GameEvents.hpp 里的 "block_activate" 形式）。
                units.add(id.getPath());
            }
            units.sort(String::compareTo);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: GameEventRadiusOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        Identifier id = Identifier.withDefaultNamespace(unit);
        GameEvent event = BuiltInRegistries.GAME_EVENT.get(id).map(h -> h.value()).orElse(null);
        if (event == null) {
            out.addProperty("missing", true);
            return out;
        }

        out.addProperty("radius", event.notificationRadius());
        return out;
    }
}
