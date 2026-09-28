"""Small mesh helpers shared by the two round-two layout prototypes (meters)."""
import bpy
import math
from mathutils import Vector


def mesh(name, vertices, faces, collection, material):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    collection.objects.link(obj)
    if material:
        data.materials.append(material)
    return obj


def box(name, center, size, collection, material, angle=0):
    x, y, z = (v / 2 for v in size)
    vertices = [(-x,-y,-z),(x,-y,-z),(x,y,-z),(-x,y,-z),
                (-x,-y,z),(x,-y,z),(x,y,z),(-x,y,z)]
    obj = mesh(name, vertices, [(0,3,2,1),(4,5,6,7),(0,1,5,4),
                               (1,2,6,5),(2,3,7,6),(3,0,4,7)], collection, material)
    obj.location = center
    obj.rotation_euler.z = angle
    return obj


def cylinder(name, center, radius, depth, collection, material, sides=48, top=None):
    top = radius if top is None else top
    vertices = [(r*math.cos(i*math.tau/sides), r*math.sin(i*math.tau/sides), z)
                for r,z in [(radius,-depth/2),(top,depth/2)] for i in range(sides)]
    faces = [(i,(i+1)%sides,(i+1)%sides+sides,i+sides) for i in range(sides)]
    faces += [tuple(reversed(range(sides))), tuple(range(sides,2*sides))]
    obj = mesh(name, vertices, faces, collection, material)
    obj.location = center
    return obj


def arc(name, center, r1, r2, z0, z1, start, end, collection, material, steps=None):
    steps = steps or max(1, math.ceil(abs(end-start)*20))
    vertices = [(center[0]+r*math.cos(start+(end-start)*i/steps),
                 center[1]+r*math.sin(start+(end-start)*i/steps), z)
                for z,r in [(z0,r1),(z0,r2),(z1,r1),(z1,r2)] for i in range(steps+1)]
    n = steps+1
    faces = []
    for i in range(steps):
        faces.extend([(i,i+1,n+i+1,n+i), (2*n+i,3*n+i,3*n+i+1,2*n+i+1),
                      (i,2*n+i,2*n+i+1,i+1), (n+i,n+i+1,3*n+i+1,3*n+i)])
    faces += [(0,n,3*n,2*n),(steps,2*n+steps,3*n+steps,n+steps)]
    return mesh(name, vertices, faces, collection, material)


def ring(name, center, r1, r2, z, collection, material, sides=96):
    vertices = [(center[0]+r*math.cos(i*math.tau/sides),center[1]+r*math.sin(i*math.tau/sides),z)
                for r in (r1,r2) for i in range(sides)]
    return mesh(name,vertices,[(i,(i+1)%sides,(i+1)%sides+sides,i+sides)
                              for i in range(sides)],collection,material)


def beam(name, a, b, radius, collection, material, sides=8):
    direction = Vector(b)-Vector(a)
    obj = cylinder(name, (Vector(a)+Vector(b))/2, radius, direction.length,
                   collection, material, sides)
    obj.rotation_euler = direction.to_track_quat('Z','Y').to_euler()
    return obj


def text_mesh(name, text, xy, size, collection, material, z=.065):
    data = bpy.data.curves.new(name, 'FONT')
    data.body = text
    data.size = size
    data.align_x = 'CENTER'
    data.align_y = 'CENTER'
    data.extrude = .002
    obj = bpy.data.objects.new(name,data)
    collection.objects.link(obj)
    obj.location = (*xy,z)
    data.materials.append(material)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.convert(target='MESH')
    return bpy.context.object


def camera(name, location, target, scale, collection):
    data = bpy.data.cameras.new(name)
    data.type = 'ORTHO'
    data.ortho_scale = scale
    data.clip_end = 1000
    obj = bpy.data.objects.new(name,data)
    collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
    return obj
