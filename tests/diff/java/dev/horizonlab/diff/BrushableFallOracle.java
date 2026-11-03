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
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 可疑沙/沙砾坠落碎开时效果与事件的原版 oracle（{@code BrushableBlock#onBrokenAfterFall}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BrushableFallOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>原版：
 *
 * <pre>
 *   Vec3 center = entity.getBoundingBox().getCenter();
 *   level.levelEvent(2001, BlockPos.containing(center), Block.getId(entity.getBlockState()));
 *   level.gameEvent(entity, GameEvent.BLOCK_DESTROY, center);
 * </pre>
 *
 * <p>两个动作缺一不可：只发粒子不发事件，坠落破碎在幽匿电路里隐形；
 * 只发事件不放粒子，玩家看不到「摔碎了」的视觉反馈。
 *
 * <p>注意原版 levelEvent 的 data 参数是 {@code Block.getId(state)}（方块状态的全局 id），
 * 这里探针固定下落状态为默认状态、data 固定 0，故忽略该项。
 */
public final class BrushableFallOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] BLOCKS = {"suspicious_sand", "suspicious_gravel"};

    private BrushableFallOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String name : BLOCKS) {
                units.add(name);
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BrushableFallOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        boolean isSand = unit.equals("suspicious_sand");
        boolean isGravel = unit.equals("suspicious_gravel");
        if (!isSand && !isGravel) {
            out.addProperty("missing", true);
            return out;
        }

        out.addProperty("block", isSand
            ? BuiltInRegistries.BLOCK.getKey(Blocks.SUSPICIOUS_SAND).toString()
            : BuiltInRegistries.BLOCK.getKey(Blocks.SUSPICIOUS_GRAVEL).toString());

        JsonArray levelEvents = new JsonArray();
        levelEvents.add(2001);
        out.add("levelEvents", levelEvents);

        JsonArray events = new JsonArray();
        events.add(BuiltInRegistries.GAME_EVENT.getKey(GameEvent.BLOCK_DESTROY.value()).toString());
        out.add("gameEvents", events);
        return out;
    }
}
