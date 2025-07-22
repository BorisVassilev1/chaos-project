import bpy
import bmesh
import mathutils
import math

scene = bpy.context.scene

objects = set(o for o in scene.objects if o.type == 'MESH')
lights = [o for o in scene.objects if o.type == 'LIGHT']

camera = [o for o in scene.objects if o.type == 'CAMERA'][0]

print("fov: ",camera.data.angle)
print("matrix: ", camera.matrix_world)

with open("export.json", "w") as f:
    f.write("{\n")
    f.write("""  
	"settings": {
		"background_color": [
			0, 0.5, 0
		],		
		"image_settings": {
			"width": 1920,
			"height": 1080
		}
	},\n""")
    f.write('  "camera": {\n')
    f.write(f'    "fov": {camera.data.angle * 180 / math.pi},\n')
    f.write('    "matrix": [\n')
    mat = mathutils.Matrix(camera.matrix_world)
    mat.transpose();
    f.write(f'      {mat[0][0]}, {mat[0][1]}, {mat[0][2]},\n')
    f.write(f'      {mat[1][0]}, {mat[1][1]}, {mat[1][2]},\n')
    f.write(f'      {mat[2][0]}, {mat[2][1]}, {mat[2][2]}\n')
    f.write('    ],\n')
    f.write('    "position": [\n')
    f.write(f'      {camera.location.x}, {camera.location.y}, {camera.location.z}\n')
    f.write('  ]\n},\n')
            
    f.write('  "objects": [\n')
    for l, obj in enumerate(objects):
        mesh = obj.data
        location = obj.location
        matrix = obj.matrix_world
        print("Exporting object:", obj.name)
        print("location:", location)
        f.write('    {\n')
        f.write(f'      "name": "{mesh.name}",\n')
        f.write('      "vertices": [\n')
        for i, v in enumerate(mesh.vertices):
            w = matrix @ v.co
            f.write(f'        {w.x}, {w.y}, {w.z}')
            if i < len(mesh.vertices) - 1:
                f.write(',')
            f.write('\n')
        f.write('      ],\n')
        f.write('      "triangles": [\n')
        for i, face in enumerate(mesh.polygons):
            for j,v in enumerate(face.vertices):
                f.write(f'{v}')
                if j < len(face.vertices) - 1 or i < len(mesh.polygons) - 1:
                    f.write(',')
            f.write('\n')
        f.write('      ]\n')
        f.write('    }')
        if l < len(objects) - 1:
            f.write(',\n')
        else:
            f.write('\n')
    f.write('  ],\n')
    f.write('  "lights": [\n')
    for l, light in enumerate(lights):
        f.write('    {\n')
        f.write(f'      "name": "{light.name}",\n')
        f.write('      "type": "point",\n')
        f.write('      "position": [\n')
        f.write(f'        {light.location.x}, {light.location.y}, {light.location.z}\n')
        f.write('      ],\n')
        f.write(f'      "intensity": {light.data.energy / math.pi}\n')
        f.write('    }')
        if l < len(lights) - 1:
            f.write(',\n')
        else:
            f.write('\n')
    f.write('  ]\n')
    f.write('}\n')
