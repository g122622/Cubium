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
import net.minecraft.world.level.gameevent.GameEvent;
import net.minecraft.world.level.block.state.properties.NoteBlockInstrument;

/**
 * 音符盒响不响、发不发事件的原版 oracle（{@code NoteBlock#playNote}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.NoteBlockEventOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code harp:false}（乐器:上方是否被方块堵住）。
 *
 * <p>原版规则：
 *
 * <pre>
 *   if (乐器.worksAboveNoteBlock() || 上方是空气) {
 *       播放音符
 *       gameEvent(NOTE_BLOCK_PLAY)
 *   }
 * </pre>
 *
 * <p>头颅类乐器 {@code worksAboveNoteBlock()} 为真，不受上方方块影响；
 * 其余乐器上方必须为空。被堵住时<b>既不出声也不发事件</b>。
 */
public final class NoteBlockEventOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private NoteBlockEventOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (NoteBlockInstrument instrument : NoteBlockInstrument.values()) {
                units.add(instrument.getSerializedName() + ":false");
                units.add(instrument.getSerializedName() + ":true");
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: NoteBlockEventOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int colon = unit.lastIndexOf(':');
        if (colon < 0) {
            out.addProperty("missing", true);
            return out;
        }
        String instrumentName = unit.substring(0, colon);
        boolean blocked = unit.substring(colon + 1).equals("true");

        NoteBlockInstrument instrument = null;
        for (NoteBlockInstrument candidate : NoteBlockInstrument.values()) {
            if (candidate.getSerializedName().equals(instrumentName)) {
                instrument = candidate;
                break;
            }
        }
        if (instrument == null) {
            out.addProperty("missing", true);
            return out;
        }

        // 原版 NoteBlock#playNote 的前置条件
        boolean sounded = instrument.worksAboveNoteBlock() || !blocked;

        out.addProperty("sounded", sounded);
        if (sounded) {
            out.addProperty("gameEvent",
                BuiltInRegistries.GAME_EVENT.getKey(GameEvent.NOTE_BLOCK_PLAY.value()).toString());
        } else {
            out.addProperty("gameEvent", (String) null);
        }
        return out;
    }
}
