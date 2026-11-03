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
import net.minecraft.sounds.SoundEvents;
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 活塞缩回时音效与游戏事件的原版 oracle（{@code PistonBaseBlock#move}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.PistonEventOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code piston:north}（活塞种类:朝向）。
 *
 * <p>原版 {@code move} 在缩回分支末尾：
 *
 * <pre>
 *   playSound(null, pos, SoundEvents.PISTON_CONTRACT, SoundSource.BLOCKS, 0.5F,
 *             random.nextFloat() * 0.15F + 0.6F);
 *   gameEvent(GameEvent.BLOCK_DEACTIVATE, pos, GameEvent.Context.of(blockstate1));
 * </pre>
 *
 * <p>音高带随机，所以这里只输出音效 id 与音量 —— 它们与朝向、活塞种类都无关。
 */
public final class PistonEventOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] PISTON_KINDS = {"piston", "sticky_piston"};
    private static final String[] FACINGS = {"north", "south", "west", "east", "up", "down"};

    private PistonEventOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String kind : PISTON_KINDS) {
                for (String facing : FACINGS) {
                    units.add(kind + ":" + facing);
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: PistonEventOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        String[] parts = unit.split(":");
        if (parts.length != 2) {
            out.addProperty("missing", true);
            return out;
        }
        boolean known = false;
        for (String kind : PISTON_KINDS) {
            if (kind.equals(parts[0])) {
                known = true;
                break;
            }
        }
        boolean knownFacing = false;
        for (String facing : FACINGS) {
            if (facing.equals(parts[1])) {
                knownFacing = true;
                break;
            }
        }
        if (!known || !knownFacing) {
            out.addProperty("missing", true);
            return out;
        }

        // 缩回：音效与事件都与朝向、活塞种类无关
        out.addProperty("sound", BuiltInRegistries.SOUND_EVENT.getKey(SoundEvents.PISTON_CONTRACT).toString());
        out.addProperty("volume", 0.5f);
        out.addProperty("gameEvent",
            BuiltInRegistries.GAME_EVENT.getKey(GameEvent.BLOCK_DEACTIVATE.value()).toString());
        out.addProperty("retracted", true);
        return out;
    }
}
