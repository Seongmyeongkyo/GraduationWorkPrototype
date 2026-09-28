"""Original procedural PBR surfaces and reusable foliage; no external assets."""
import bpy
import math
import random
import numpy as np
from mathutils import Vector


def noise2(n,grid,seed):
    rng=np.random.default_rng(seed)
    values=rng.random((grid,grid)).astype(np.float32)
    p=np.arange(n,dtype=np.float32)*grid/n
    cell=np.floor(p).astype(np.int32)
    t=p-cell;t=t*t*(3-2*t)
    a=values[cell[:,None]%grid,cell[None,:]%grid]
    b=values[cell[:,None]%grid,(cell[None,:]+1)%grid]
    c=values[(cell[:,None]+1)%grid,cell[None,:]%grid]
    d=values[(cell[:,None]+1)%grid,(cell[None,:]+1)%grid]
    return (a*(1-t[None,:])+b*t[None,:])*(1-t[:,None])+(c*(1-t[None,:])+d*t[None,:])*t[:,None]


def textures(out,n=1024):
    out.mkdir(parents=True,exist_ok=True)
    y,x=np.mgrid[0:n,0:n].astype(np.float32)/n
    f=sum(noise2(n,g,610+g)*w for g,w in [(4,.35),(12,.3),(32,.2),(96,.1),(256,.05)])
    grain=noise2(n,384,72)
    stratum=.5+.5*np.sin((y*18+noise2(n,9,12)*.28)*math.tau)
    barkgroove=np.maximum(0,np.cos((x*22+noise2(n,7,17)*.4)*math.tau))**12
    vein=np.exp(-abs(y-.5)*180)
    lateral=np.maximum(0,np.cos((x*13+abs(y-.5)*17)*math.tau))**25
    data={
        'Ground':(.78+.32*f+.12*grain, .70+.16*grain, (.84,.81,.74)),
        'Rock':(.45+.38*f+.07*stratum+.10*grain, .73+.18*grain, (.30,.32,.29)),
        'Bark':(.35+.5*f-.22*barkgroove+.15*grain, .77+.16*grain, (.27,.17,.095)),
        'Moss':(.36+.36*f+.28*grain, .87+.1*grain, (.20,.29,.10)),
        'Leaf':(.55+.25*f+.13*vein+.07*lateral, .46+.15*f, (.16,.32,.075)),
        'Canvas':(.72+.18*f+.10*(np.sin(x*n*math.pi)**2*np.sin(y*n*math.pi)**2), .88+.08*grain, (.66,.56,.39))}
    paths={}
    for kind,(height,rough,base) in data.items():
        color=np.clip(height[...,None]*np.array(base,dtype=np.float32)[None,None,:]*(1.4 if kind!='Ground' else 1.1),0,1)
        if kind=='Ground': color=np.repeat(np.clip(height,.65,1.0)[...,None],3,axis=2)
        gx=(np.roll(height,-1,axis=1)-np.roll(height,1,axis=1))*5
        gy=(np.roll(height,-1,axis=0)-np.roll(height,1,axis=0))*5
        normal=np.stack((-gx,-gy,np.ones_like(gx)),axis=2)
        normal/=np.linalg.norm(normal,axis=2)[...,None]
        paths[kind]={}
        for channel,rgb in [('BaseColor',color),('Normal',normal*.5+.5),('Roughness',np.repeat(rough[...,None],3,axis=2))]:
            image=bpy.data.images.new('T_HQ_'+kind+'_'+channel,width=n,height=n,alpha=False)
            if channel!='BaseColor': image.colorspace_settings.name='Non-Color'
            rgba=np.concatenate((rgb,np.ones((n,n,1),dtype=np.float32)),axis=2).astype(np.float32)
            image.pixels.foreach_set(rgba.ravel())
            image.filepath_raw=str(out/(image.name+'.png'));image.file_format='PNG';image.save()
            paths[kind][channel]=image
    return paths


class Builder:
    def __init__(self):
        self.vertices=[];self.faces=[];self.uvs=[];self.colors=[];self.materials=[];self.smooth=[]

    def face(self,points,uvs,color,material=0,smooth=False):
        offset=len(self.vertices)
        self.vertices.extend(points);self.uvs.extend(uvs)
        self.colors.extend([(*color,1)]*len(points))
        self.faces.append(tuple(range(offset,offset+len(points))))
        self.materials.append(material);self.smooth.append(smooth)

    def tube(self,a,b,r1,r2,color=(1,1,1),sides=9):
        a,b=Vector(a),Vector(b)
        rotation=(b-a).to_track_quat('Z','Y')
        length=(b-a).length
        for i in range(sides):
            t1=i*math.tau/sides;t2=(i+1)*math.tau/sides
            points=[a+rotation@Vector((r1*math.cos(t1),r1*math.sin(t1),0)),
                    a+rotation@Vector((r1*math.cos(t2),r1*math.sin(t2),0)),
                    b+rotation@Vector((r2*math.cos(t2),r2*math.sin(t2),0)),
                    b+rotation@Vector((r2*math.cos(t1),r2*math.sin(t1),0))]
            self.face(points,[(i/sides,0),((i+1)/sides,0),((i+1)/sides,length),
                              (i/sides,length)],color,0,True)

    def leaf(self,center,length,width,angle,tilt,color,material=1):
        axis=Vector((math.cos(angle)*math.cos(tilt),math.sin(angle)*math.cos(tilt),math.sin(tilt)))
        side=Vector((-math.sin(angle),math.cos(angle),0))
        center=Vector(center)
        points=[center-axis*length/2,center-axis*length*.26-side*width/2,
                center+axis*length*.26-side*width/2,center+axis*length/2,
                center+axis*length*.26+side*width/2,center-axis*length*.26+side*width/2,
                center+Vector((0,0,width*.10))]
        uv=[(0,.5),(.25,0),(.75,0),(1,.5),(.75,1),(.25,1),(.5,.5)]
        for i in range(6):
            self.face([points[i],points[(i+1)%6],points[6]],[uv[i],uv[(i+1)%6],uv[6]],color,material)

    def object(self,name,collection,materials):
        data=bpy.data.meshes.new(name)
        data.from_pydata(self.vertices,[],self.faces)
        for m in materials:data.materials.append(m)
        data.update()
        uv=data.uv_layers.new(name='UVMap')
        colors=data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
        for i,color in enumerate(self.colors):colors.data[i].color=color
        for p,mi,smooth in zip(data.polygons,self.materials,self.smooth):
            p.material_index=mi;p.use_smooth=smooth
            for li in p.loop_indices:uv.data[li].uv=self.uvs[data.loops[li].vertex_index]
        obj=bpy.data.objects.new(name,data);collection.objects.link(obj)
        return obj


def tree_mesh(name,collection,materials,seed,pine=False):
    rng=random.Random(seed);b=Builder()
    h=7.0 if not pine else 8.0
    bend=rng.uniform(-.35,.35)
    points=[Vector((bend*(i/6)**1.4,math.sin(i*.7)*.12,h*i/6)) for i in range(7)]
    for i in range(6):b.tube(points[i],points[i+1],.34*(1-i/7)**1.2,.34*(1-(i+1)/7)**1.2,sides=12)
    for j in range(7):
        a=j*math.tau/7
        b.tube((math.cos(a)*.70,math.sin(a)*.70,.02),(0,0,.8),.07,.15,sides=7)
    if not pine:
        for j in range(15):
            a=j*2.399+rng.uniform(-.2,.2)
            start=Vector((bend*.45,0,2.3+j*.17))
            end=Vector((math.cos(a)*rng.uniform(1.0,1.65),math.sin(a)*rng.uniform(1.0,1.65),4.8+j*.115))
            mid=start.lerp(end,.55)+Vector((0,0,.25))
            b.tube(start,mid,.115,.07,sides=8);b.tube(mid,end,.07,.025,sides=7)
            for k in range(5):
                ta=a+rng.uniform(-1.5,1.5)
                tip=end+Vector((math.cos(ta)*.7,math.sin(ta)*.7,rng.uniform(-.1,.7)))
                b.tube(mid.lerp(end,.6),tip,.035,.008,sides=5)
            for k in range(135):
                az=rng.random()*math.tau; zz=rng.uniform(-1,1);rr=rng.random()**(1/3)
                center=end+Vector((math.cos(az)*math.sqrt(1-zz*zz)*rr*.95,math.sin(az)*math.sqrt(1-zz*zz)*rr*.95,zz*rr*.7))
                if math.hypot(center.x,center.y)>2.4: continue
                tone=rng.uniform(.65,1.35)
                b.leaf(center,rng.uniform(.22,.36),rng.uniform(.11,.19),rng.random()*math.tau,rng.uniform(-.8,.8),(tone,tone,rng.uniform(.7,1.05)))
    else:
        for tier in range(9):
            z=1.8+tier*.64;reach=2.1*(1-tier/11)
            for j in range(8):
                a=j*math.tau/8+tier*.65
                start=Vector((0,0,z));end=Vector((math.cos(a)*reach,math.sin(a)*reach,z-.28))
                b.tube(start,end,.085*(1-tier/12),.01,sides=7)
                for k in range(32):
                    t=.18+.82*k/32
                    spread=.43*(1-t)+.12
                    for sign in (-1,1):
                        center=start.lerp(end,t)+Vector((-math.sin(a)*spread*sign,math.cos(a)*spread*sign,rng.uniform(-.08,.15)))
                        tone=rng.uniform(.45,.85)
                        b.leaf(center,.30*(1-t/2),.10,a+sign*.75,rng.uniform(-.3,.3),(tone,tone*.92,tone),1)
    return b.object(name,collection,materials)


def fern_mesh(name,collection,materials):
    b=Builder();rng=random.Random(713)
    for j in range(9):
        a=j*math.tau/9
        start=Vector((0,0,.04))
        end=Vector((math.cos(a)*.60,math.sin(a)*.60,.32))
        mid=Vector((math.cos(a)*.28,math.sin(a)*.28,.62))
        b.tube(start,mid,.012,.007,sides=4);b.tube(mid,end,.007,.001,sides=4)
        for k in range(12):
            t=(k+1)/13
            center=start*(1-t)**2+mid*2*(1-t)*t+end*t*t
            for sign in (-1,1):
                direction=a+sign*.9
                size=.22*math.sin(t*math.pi)**.7
                p=center+Vector((math.cos(direction)*size*.35,math.sin(direction)*size*.35,0))
                b.leaf(p,size,.045,direction,.12,(.8+rng.random()*.3,1,.85),1)
    return b.object(name,collection,materials)


def grass_mesh(name,collection,materials):
    b=Builder();rng=random.Random(821)
    for j in range(24):
        a=rng.random()*math.tau;radius=rng.random()*.22
        root=Vector((math.cos(a)*radius,math.sin(a)*radius,0))
        height=rng.uniform(.13,.42);side=Vector((math.cos(a),math.sin(a),0))*.018
        middle=root+Vector((math.cos(a)*.065,math.sin(a)*.065,height*.65))
        tip=root+Vector((math.cos(a)*.15,math.sin(a)*.15,height))
        color=(rng.uniform(.65,1.15),rng.uniform(.85,1.15),.7)
        b.face([root-side,root+side,middle+side*.6,middle-side*.6],[(0,0),(1,0),(1,.6),(0,.6)],color,1)
        b.face([middle-side*.6,middle+side*.6,tip],[(0,.6),(1,.6),(.5,1)],color,1)
    return b.object(name,collection,materials)
