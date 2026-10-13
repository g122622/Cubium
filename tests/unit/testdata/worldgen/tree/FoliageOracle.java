import java.util.Random;
public class FoliageOracle {
 public static void main(String[] args){
  Random r=new Random(42);int count=0;
  for(int y=0;y>=-3;y--){int radius=Math.max(2-1-y/2,0),layer=0;
   for(int x=-radius;x<=radius;x++)for(int z=-radius;z<=radius;z++){
    boolean skip=Math.abs(x)==radius && Math.abs(z)==radius && (r.nextInt(2)==0 || y==0);
    if(!skip){layer++;count++;if(Math.abs(x)==radius && Math.abs(z)==radius)System.out.printf("corner %d %d %d%n",x,y,z);}
   }
   System.out.println("layer "+y+" count "+layer);
  }
  System.out.println("total "+count+" next "+r.nextInt());
 }
}
