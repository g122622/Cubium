import java.util.*;
// Java 1.21.11 精美树干算法在平坦无障碍世界中的独立参考。
public class FancyTrunkOracle {
 record Pos(int x,int y,int z){}
 record Branch(Pos end,int base){}
 static Set<Pos> logs=new HashSet<>();
 static void limb(Pos s,Pos e){int dx=e.x-s.x,dy=e.y-s.y,dz=e.z-s.z,n=Math.max(Math.abs(dx),Math.max(Math.abs(dy),Math.abs(dz)));
  float x=(float)dx/n,y=(float)dy/n,z=(float)dz/n;
  for(int j=0;j<=n;j++)logs.add(new Pos(s.x+(int)Math.floor(.5F+j*x),s.y+(int)Math.floor(.5F+j*y),s.z+(int)Math.floor(.5F+j*z)));
 }
 public static void main(String[] args){Random r=new Random(42);Pos origin=new Pos(8,1,8);int height=10,k=(int)Math.floor(height*.618),base=origin.y+k,top=height-5;List<Branch> branches=new ArrayList<>();branches.add(new Branch(new Pos(8,1+top,8),base));
  for(int j=top;j>=0;j--){if(j<height*.3F)continue;float half=height/2F,diff=half-j,len=(float)Math.sqrt(half*half-diff*diff)*.5F;
   double length=len*(r.nextFloat()+.328),angle=r.nextFloat()*2F*Math.PI;
   Pos end=new Pos(8+(int)Math.floor(length*Math.sin(angle)+.5),1+j-1,8+(int)Math.floor(length*Math.cos(angle)+.5));
   int dx=8-end.x,dz=8-end.z;double d=end.y-Math.sqrt(dx*dx+dz*dz)*.381;int join=d>base?base:(int)d;branches.add(new Branch(end,join));
  }
  limb(origin,new Pos(8,1+k,8));for(Branch b:branches){Pos s=new Pos(8,b.base,8);if(!s.equals(b.end)&&b.base-1>=height*.2)limb(s,b.end);}
  var sorted=new ArrayList<>(logs);sorted.sort(Comparator.comparingInt(Pos::x).thenComparingInt(Pos::y).thenComparingInt(Pos::z));
  System.out.println("logs="+sorted);System.out.println("attachments="+branches.stream().filter(b->b.base-1>=height*.2).map(b->b.end).toList());System.out.println("next="+r.nextInt());
 }
}
