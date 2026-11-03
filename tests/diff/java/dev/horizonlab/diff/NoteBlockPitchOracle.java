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
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.NoteBlock;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.BlockStateProperties;

/**
 * 音符盒调音的原版 oracle（{@code NoteBlock#getPitchFromNote} 与 {@code NOTE_0_24} 的循环）。
 *
 * <pre>
 *   java dev.horizonlab.diff.NoteBlockPitchOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 单元名形如 {@code note=<0..24>}。输出 {@code pitch} 为 f32 的位模式（{@code 0x%08x}），
 * 只有位级比对才能抓到 1 ulp 的浮点差别。
 */
public final class NoteBlockPitchOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();
    private static final int NOTE_COUNT = 25;

    private NoteBlockPitchOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (int note = 0; note < NOTE_COUNT; note++) {
                units.add("note=" + note);
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: NoteBlockPitchOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        if (!unit.startsWith("note=")) {
            out.addProperty("missing", true);
            return out;
        }
        int note = Integer.parseInt(unit.substring(5));
        if (note < 0 || note >= NOTE_COUNT) {
            out.addProperty("missing", true);
            return out;
        }

        BlockState base = Blocks.NOTE_BLOCK.defaultBlockState();
        BlockState withNote = base.setValue(BlockStateProperties.NOTE, note);
        BlockState cycled = withNote.cycle(BlockStateProperties.NOTE);

        float pitch = NoteBlock.getPitchFromNote(note);
        String bits = String.format("0x%08x", Float.floatToRawIntBits(pitch));

        int cycledNote = cycled.getValue(BlockStateProperties.NOTE);

        out.addProperty("pitch", bits);
        out.addProperty("note", withNote.getValue(BlockStateProperties.NOTE));
        out.addProperty("cycled", cycledNote);
        out.addProperty("wrapped", cycledNote == 0);
        return out;
    }
}
