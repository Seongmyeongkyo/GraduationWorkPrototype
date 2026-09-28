"""Round-one convention: north=+X; Blender east=-Y, Unreal east=+Y."""
import math
import bpy
from mathutils import Matrix

FRAME='X_NORTH_Y_WEST'


def rotate_point(point, inverse=False):
    return ([-point[1],point[0]] if inverse else [point[1],-point[0]])+list(point[2:])


def rotate_scene(scene,inverse=False):
    bpy.context.view_layer.update()
    rotation=Matrix.Rotation(math.pi/2 if inverse else -math.pi/2,4,'Z')
    matrices=[(obj,rotation@obj.matrix_world) for obj in scene.objects if not obj.get('library_prototype')]
    for obj,matrix in matrices:
        obj.matrix_world=matrix


def rotate_layout(layout,inverse=False):
    transform=lambda p:rotate_point(p,inverse)
    delta=90 if inverse else -90
    for group in ('camps','spawns'):
        for entry in layout.get(group,[]): entry['xy']=transform(entry['xy'])
    for entry in layout.get('bushes',[]):
        entry['center_m']=transform(entry['center_m'])
        entry['rotation_degrees']+=delta
    for entry in layout.get('paths',[]): entry['points']=[transform(p) for p in entry['points']]
    for entry in layout.get('walls',[]):
        if 'center_m' in entry: entry['center_m']=transform(entry['center_m'])
        if 'polygon_m' in entry: entry['polygon_m']=[transform(p) for p in entry['polygon_m']]
    for entry in layout.get('markers',[]):
        entry['position_m']=transform(entry['position_m'])
        if 'rotation_degrees' in entry: entry['rotation_degrees']+=delta
    layout['coordinate_frame']='DESIGN_X_EAST_Y_NORTH' if inverse else FRAME
    layout['coordinate_convention']=('Blender: +X north/up, -Y east/right, Z up. '
        'Unreal: X=blender_x*100, Y=-blender_y*100, Z=blender_z*100; +X north/up, +Y east/right.')


def ensure_north(scene,layout):
    if scene.get('round01_coordinate_frame')!=FRAME:
        rotate_scene(scene)
        scene['round01_coordinate_frame']=FRAME
    if layout.get('coordinate_frame')!=FRAME: rotate_layout(layout)
