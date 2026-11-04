import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.WorldgenRandom;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.levelgen.structure.StructurePieceAccessor;
import net.minecraft.world.level.levelgen.structure.structures.MineshaftPieces;
import net.minecraft.world.level.levelgen.structure.structures.MineshaftStructure;

/**
 * 直接驱动原版的 MineshaftPieces，导出完整的构件清单与随机抽取序列。
 *
 * 用途：差分排查。它不建世界，只构造一个 MineShaftRoom 然后调 addChildren，
 * 因此能拿到与 oracle 完全一致、但带有「每次抽取属于哪个构件」的细粒度序列。
 *
 * 抽取归属用**深度栈**推断：addChildren 是同步 DFS，
 * 任何时候正在展开的构件链就是「已登记的、深度严格递增的最近若干个」。
 * 因此把已登记构件按深度压栈，取值时弹掉深度 >= 当前深度的，栈顶就是归属。
 *
 * 注意：nextBoolean() 走 BitRandomSource 的默认方法，内部调 next(1)，
 * 所以必须覆写 next(int) 才能捕获，覆写 nextBoolean() 是无效的。
 */
public final class MsDrive {
    /** 已登记构件，按登记顺序。 */
    static final List<StructurePiece> PIECES = new ArrayList<>();

    /** 当前展开链：深度严格递增，栈顶是正在展开的构件序号。 */
    static final Deque<int[]> ACTIVE = new ArrayDeque<>();

    static final List<String> LOG = new ArrayList<>();

    /** 取当前正在展开的构件序号；无则 -1。 */
    static int owner() {
        return ACTIVE.isEmpty() ? -1 : ACTIVE.peek()[0];
    }

    static final class Tracing extends WorldgenRandom {
        Tracing() {
            super(new LegacyRandomSource(0L));
        }

        @Override
        public int next(int bits) {
            int value = super.next(bits);
            if (bits == 1) {
                LOG.add(owner() + "|nextBoolean()=" + (value != 0));
            }
            return value;
        }

        @Override
        public int nextInt(int bound) {
            int value = super.nextInt(bound);
            LOG.add(owner() + "|nextInt(" + bound + ")=" + value);
            return value;
        }

        @Override
        public double nextDouble() {
            double value = super.nextDouble();
            LOG.add(owner() + "|nextDouble()");
            return value;
        }
    }

    /** 交叉点的 direction 是 private 字段，用反射读出来。 */
    static String crossDir(StructurePiece piece) {
        try {
            var field = piece.getClass().getDeclaredField("direction");
            field.setAccessible(true);
            return String.valueOf(field.get(piece));
        } catch (ReflectiveOperationException e) {
            return "-";
        }
    }

    /** 返回调用 findCollisionPiece 的最近一个 find* 方法名。 */
    static String caller() {
        StackTraceElement[] frames = Thread.currentThread().getStackTrace();
        for (StackTraceElement frame : frames) {
            String name = frame.getMethodName();
            if (name.startsWith("find") && !name.equals("findCollisionPiece")) {
                return name;
            }
        }
        return "?";
    }

    static final class Accessor implements StructurePieceAccessor {
        @Override
        public void addPiece(StructurePiece piece) {
            int depth = piece.getGenDepth();
            // 弹掉深度 >= 当前深度的，栈顶就是父构件
            while (!ACTIVE.isEmpty() && ACTIVE.peek()[1] >= depth) {
                ACTIVE.pop();
            }
            int parent = owner();
            int index = PIECES.size();
            PIECES.add(piece);
            ACTIVE.push(new int[] {index, depth});
            LOG.add("-1|ADD[" + index + "] " + piece.getClass().getSimpleName() + " depth=" + depth + " parent=" + parent + " dir=" + piece.getOrientation()
                    + " crossDir=" + crossDir(piece) + " " + piece.getBoundingBox());
        }

        @Override
        public StructurePiece findCollisionPiece(BoundingBox box) {
            for (StructurePiece piece : PIECES) {
                if (piece.getBoundingBox().intersects(box)) {
                    LOG.add("-1|COLLIDE " + box + " <- [" + PIECES.indexOf(piece) + "] d=" + piece.getGenDepth() + " @" + caller());
                    return piece;
                }
            }
            LOG.add("-1|NOCULL " + box + " @" + caller());
            return null;
        }
    }

    public static void main(String[] args) {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        long worldSeed = Long.parseLong(args[0]);
        int chunkX = Integer.parseInt(args[1]);
        int chunkZ = Integer.parseInt(args[2]);

        Tracing rng = new Tracing();
        rng.setLargeFeatureSeed(worldSeed, chunkX, chunkZ);
        rng.nextDouble();
        LOG.clear();

        ChunkPos pos = new ChunkPos(chunkX, chunkZ);
        Accessor accessor = new Accessor();
        MineshaftPieces.MineShaftRoom room = new MineshaftPieces.MineShaftRoom(
                0, rng, pos.getBlockX(2), pos.getBlockZ(2), MineshaftStructure.Type.NORMAL);
        LOG.clear();

        PIECES.add(room);
        ACTIVE.push(new int[] {0, 0});
        System.out.println("PIECE 0 depth=0 parent=-1 " + room.getBoundingBox());
        room.addChildren(room, accessor, rng);

        System.out.println("COUNT " + PIECES.size());
        for (int i = 0; i < LOG.size(); i++) {
            System.out.println("DRAW " + i + " " + LOG.get(i));
        }
    }

}
