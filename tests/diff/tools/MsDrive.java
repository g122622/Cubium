/**
 * 直接驱动原版的 MineshaftPieces，导出完整的构件清单与随机抽取序列。
 *
 * 用途：差分排查。它不建世界，只构造一个 MineShaftRoom 然后调 addChildren，
 * 因此能拿到与 oracle 完全一致、但带有「每次抽取属于哪个构件」的细粒度序列。
 *
 * 输出：
 *   PIECE <序号> depth=<深度> parent=<父序号> BoundingBox{...}
 *   #<抽取序号> <所属构件序号> nextInt(<实参>)=<返回值>
 *
 * 注意：nextBoolean() 走 BitRandomSource 的默认方法，内部调 next(1)，
 * 所以必须覆写 next(int) 才能捕获，覆写 nextBoolean() 是无效的。
 */
public final class MsDrive {
    /** 当前展开的构件序号，-1 表示房间自身的 addChildren 尚未被任何 addPiece 更新。 */
    static int current = -1;
    static final int[] latestAtDepth = new int[16];
    static { java.util.Arrays.fill(latestAtDepth, -1); }

    static final class Tracing extends WorldgenRandom {
        final List<String> log = new ArrayList<>();

        Tracing() {
            super(new LegacyRandomSource(0L));
        }

        @Override
        public int nextInt(int bound) {
            int value = super.nextInt(bound);
            log.add(current + "|nextInt(" + bound + ")=" + value);
            return value;
        }

        @Override
        public int next(int bits) {
            int value = super.next(bits);
            if (bits == 1) {
                log.add(current + "|nextBoolean()=" + (value != 0));
            }
            return value;
        }

        @Override
        public boolean nextBoolean() {
            boolean value = super.nextBoolean();
            log.add(current + "|nextBoolean()=" + value);
            return value;
        }

        @Override
        public double nextDouble() {
            double value = super.nextDouble();
            log.add(current + "|nextDouble()");
            return value;
        }
    }

    static final class Accessor implements StructurePieceAccessor {
        final List<StructurePiece> pieces = new ArrayList<>();

        @Override
        public void addPiece(StructurePiece piece) {
            pieces.add(piece);
            current = pieces.size() - 1;
            int d = piece.getGenDepth();
            int parent = (d == 0) ? -1 : latestAtDepth[d - 1];
            latestAtDepth[d] = current;
            for (int k = d + 1; k < latestAtDepth.length; k++) { latestAtDepth[k] = -1; }
            System.out.println("PIECE " + current + " depth=" + d + " parent=" + parent + " " + piece.getBoundingBox());
        }

        @Override
        public StructurePiece findCollisionPiece(BoundingBox box) {
            for (StructurePiece piece : pieces) {
                if (piece.getBoundingBox().intersects(box)) {
                    return piece;
                }
            }
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
        rng.log.clear();

        ChunkPos pos = new ChunkPos(chunkX, chunkZ);
        Accessor accessor = new Accessor();
        MineshaftPieces.MineShaftRoom room = new MineshaftPieces.MineShaftRoom(
                0, rng, pos.getBlockX(2), pos.getBlockZ(2), MineshaftStructure.Type.NORMAL);
        rng.log.clear();

        accessor.addPiece(room);
        current = 0;
        room.addChildren(room, accessor, rng);

        System.out.println("  构件数 = " + accessor.pieces.size());
        int seq = 0;
        for (String line : rng.log) {
            int bar = line.indexOf('|');
            System.out.println("    #" + (seq++) + " " + line.substring(0, bar) + " " + line.substring(bar + 1));
        }
    }
}
