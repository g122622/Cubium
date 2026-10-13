import java.util.Random;
// 独立采用 Java 1.21.11 的 CaveWorldCarver 算法，固定所有配置 provider 为常数。
public class CarverOracle {
  static float[] SIN=new float[65536];
  static { for(int i=0;i<SIN.length;i++)SIN[i]=(float)Math.sin(i/10430.378350470453); }
  static float sin(double x){return SIN[(int)((long)(x*10430.378350470453)&65535L)];}
  static float cos(double x){return SIN[(int)((long)(x*10430.378350470453+16384.0)&65535L)];}
  boolean[] mask=new boolean[16*16*384]; float[] widths; int cx,cz;
  CarverOracle(int x,int z){cx=x;cz=z;}
  void ellipse(double x,double y,double z,double hr,double vr){
    if(Math.abs(x-(cx*16+8))>16.0+hr*2 || Math.abs(z-(cz*16+8))>16.0+hr*2)return;
    int xmin=Math.max((int)Math.floor(x-hr)-cx*16-1,0),xmax=Math.min((int)Math.floor(x+hr)-cx*16,15);
    int zmin=Math.max((int)Math.floor(z-hr)-cz*16-1,0),zmax=Math.min((int)Math.floor(z+hr)-cz*16,15);
    int ymin=Math.max((int)Math.floor(y-vr)-1,-63),ymax=Math.min((int)Math.floor(y+vr)+1,312);
    for(int a=xmin;a<=xmax;a++)for(int b=zmin;b<=zmax;b++){
      double dx=(cx*16+a+.5-x)/hr,dz=(cz*16+b+.5-z)/hr;
      if(dx*dx+dz*dz>=1)continue;
      for(int c=ymax;c>ymin;c--){double dy=(c-.5-y)/vr;
        if(widths==null ? (dy<=-1 || dx*dx+dy*dy+dz*dz>=1) : ((dx*dx+dz*dz)*widths[c+63]+dy*dy/6.0>=1))continue;
        mask[(a*16+b)*384+c+64]=true;
      }
    }
  }
  boolean reach(double x,double z,int i,int end,float r){double dx=x-(cx*16+8),dz=z-(cz*16+8),n=end-i,m=r+2F+16F;return dx*dx+dz*dz-n*n<=m*m;}
  void tunnel(long seed,double x,double y,double z,float thick,float yaw,float pitch,int start,int end,double scale){
    Random r=new Random(seed);int split=r.nextInt(end/2)+end/4;boolean flat=r.nextInt(6)==0;float ym=0,pm=0;
    for(int i=start;i<end;i++){
      double hr=1.5+sin((float)Math.PI*i/end)*thick,vr=hr*scale;
      float cp=cos(pitch);x+=cos(yaw)*cp;y+=sin(pitch);z+=sin(yaw)*cp;
      pitch*=flat?.92F:.7F;pitch+=pm*.1F;yaw+=ym*.1F;pm*=.9F;ym*=.75F;
      pm+=(r.nextFloat()-r.nextFloat())*r.nextFloat()*2F;ym+=(r.nextFloat()-r.nextFloat())*r.nextFloat()*4F;
      if(i==split && thick>1F){
        tunnel(r.nextLong(),x,y,z,r.nextFloat()*.5F+.5F,yaw-(float)(Math.PI/2),pitch/3F,i,end,1);
        tunnel(r.nextLong(),x,y,z,r.nextFloat()*.5F+.5F,yaw+(float)(Math.PI/2),pitch/3F,i,end,1);return;
      }
      if(r.nextInt(4)!=0){if(!reach(x,z,i,end,thick))return;ellipse(x,y,z,hr,vr);}
    }
  }
  void carve(long seed){
    Random r=new Random(seed);int n=r.nextInt(r.nextInt(r.nextInt(15)+1)+1);
    for(int k=0;k<n;k++){
      double x=r.nextInt(16),y=64,z=r.nextInt(16);int tunnels=1;
      if(r.nextInt(4)==0){float radius=1F+r.nextFloat()*6F;ellipse(x+1,y,z,1.5+sin((float)(Math.PI/2))*radius,1.5+sin((float)(Math.PI/2))*radius);tunnels+=r.nextInt(4);}
      for(int t=0;t<tunnels;t++){
        float yaw=r.nextFloat()*(float)(Math.PI*2),pitch=(r.nextFloat()-.5F)/4F;
        float thick=r.nextFloat()*2F+r.nextFloat();if(r.nextInt(10)==0)thick*=r.nextFloat()*r.nextFloat()*3F+1F;
        int len=112-r.nextInt(112/4);tunnel(r.nextLong(),x,y,z,thick,yaw,pitch,0,len,1);
      }
    }
  }
  void canyon(long seed){
    Random outer=new Random(seed);double x=outer.nextInt(16),y=64,z=outer.nextInt(16);float yaw=outer.nextFloat()*(float)(Math.PI*2),pitch=0;
    Random r=new Random(outer.nextLong());widths=new float[384];float width=1;
    for(int j=0;j<384;j++){if(j==0 || r.nextInt(3)==0)width=1F+r.nextFloat()*r.nextFloat();widths[j]=width*width;}
    float ym=0,pm=0;
    for(int i=0;i<112;i++){
      double hr=1.5+sin(i*(float)Math.PI/112)*2F,vr=hr*(r.nextFloat()*.25F+.75F);
      float cp=cos(pitch);x+=cos(yaw)*cp;y+=sin(pitch);z+=sin(yaw)*cp;
      pitch*=.7F;pitch+=pm*.05F;yaw+=ym*.05F;pm*=.8F;ym*=.5F;
      pm+=(r.nextFloat()-r.nextFloat())*r.nextFloat()*2F;ym+=(r.nextFloat()-r.nextFloat())*r.nextFloat()*4F;
      if(r.nextInt(4)!=0){if(!reach(x,z,i,112,2F))return;ellipse(x,y,z,hr,vr);}
    }
  }
  public static void main(String[] args){for(boolean canyon:new boolean[]{false,true})for(long seed:new long[]{2,4,42,12345})for(int cx:new int[]{0,-1}){
    CarverOracle o=new CarverOracle(cx,0);if(canyon)o.canyon(seed);else o.carve(seed);long hash=0xcbf29ce484222325L;int count=0;
    for(boolean bit:o.mask){hash^=bit?1:0;hash*=0x100000001b3L;if(bit)count++;}
    System.out.printf("%s %d %d count=%d hash=%016x%n",canyon?"canyon":"cave",seed,cx,count,hash);
  }}
}
