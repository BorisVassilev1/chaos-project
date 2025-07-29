import bpy
import bmesh
import mathutils
import math

scene = bpy.context.scene

objects = set(o for o in scene.objects if o.type == 'MESH')
meshes = list(set(o.data for o in objects))
lights = [o for o in scene.objects if o.type == 'LIGHT']

camera = [o for o in scene.objects if o.type == 'CAMERA'][0]

materials = [o for o in bpy.data.materials if o.use_nodes and (o.node_tree.nodes.get('Principled BSDF')
                                                               or o.node_tree.nodes.get('Glass BSDF'))]
print("Materials found: ", len(materials))
for m in materials:
    print("Material: ", m.name)

image_textures = [n for m in materials for n in m.node_tree.nodes if n.type == 'TEX_IMAGE']
textures = set(n.image for n in image_textures if n.image)

print("fov: ",camera.data.angle)
print("matrix: ", camera.matrix_world)

frame_current = scene.frame_current
cameraPositions = [];
for f in range(scene.frame_start, scene.frame_end + 1):
    scene.frame_set(f)
    cameraPositions.append(camera.matrix_world.copy())
scene.frame_set(frame_current)
print("frames: ", len(cameraPositions))

with open("export.json", "w") as f:
    f.write("{\n")
    f.write(f"""  
	"settings": {{
        "background_color": [
			0.6, 0.6, 0.6
		],		
		"image_settings": {{
			"width": 1920,
			"height": 1080
		}},
        "frames": {len(cameraPositions)}
	}},\n""")
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
    f.write('  ],\n')
    f.write('  "animation": [')
    for i, mat in enumerate(cameraPositions):
        f.write(f"""[
      {mat[0][0]}, {mat[0][1]}, {mat[0][2]}, {mat[0][3]},
      {mat[1][0]}, {mat[1][1]}, {mat[1][2]}, {mat[1][3]},
      {mat[2][0]}, {mat[2][1]}, {mat[2][2]}, {mat[2][3]},
      {mat[3][0]}, {mat[3][1]}, {mat[3][2]}, {mat[3][3]}
            ]""")
        if i != len(cameraPositions)-1:
            f.write(',');
        f.write('\n')
    f.write(']\n')
    f.write('},\n')
    f.write('  "materials": [\n')
    for m in materials:
        print("Material: ", m.name)
        f.write('    {\n')
        f.write(f'      "name": "{m.name}",\n')

        if m.name == "CHECKER":
            print("  Checker material found, using diffuse type.")
            f.write('      "type": "diffuse",\n')
            f.write('      "albedo": "Black White Checker"\n')
            f.write('}')
            if m != materials[-1]:
                f.write(',')
            f.write('\n')
            continue;
        bsdf = m.node_tree.nodes.get('Principled BSDF')
        if bsdf:
            if m.name == "REFLECTIVE":
                print("  Reflective material found, using reflective type.")
                f.write('      "type": "reflective",\n')
                f.write('      "smooth_shading": true,\n')
            else:
                f.write('      "type": "diffuse",\n')
            base_color_node = bsdf.inputs.get('Base Color')
            print("  Base Color Input:")
            if base_color_node:
                print(f"    Type: {base_color_node.type}")
                if base_color_node.is_linked and base_color_node.links[0].from_node.type == 'TEX_IMAGE':
                    linked_node = base_color_node.links[0].from_node
                    print(f"    Linked to: {linked_node.name} ({linked_node.type})")
                    print(f"    Image: {linked_node.image.name}")
                    f.write(f'    "albedo": "{linked_node.image.name}"\n')
                else:
                    print("    Not linked")
                    f.write(f'    "albedo": [{base_color_node.default_value[0]}, {base_color_node.default_value[1]}, {base_color_node.default_value[2]}]\n')
        bsdf = m.node_tree.nodes.get('Glass BSDF')
        if bsdf:
            print("  Glass BSDF found in material.")
            f.write('    "type": "refractive",\n')
            f.write('    "absorbtion" : [\
				2., 0.4, 0.1\
			],\n')
            f.write('    "ior": 1.5,\n')
            f.write('    "smooth_shading": true\n')
        f.write('    }');
        if m != materials[-1]:
            f.write('    ,')
        f.write('\n')
    f.write('  ],\n')

    f.write('  "textures": [\
        {\
			"name": "Black White Checker",\
			"type": "checker",\
			"color_A": [ \
				0, 0, 0 \
			],\
			"color_B": [\
				1, 1, 1\
			],\
			"square_size": 0.125		\
		} \n')
    if len(textures) > 0: f.write(',')
    for i, t in enumerate(textures):
        f.write('    {\n')
        f.write(f'      "type": "bitmap",\n')
        f.write(f'      "name": "{t.name}",\n')
        f.write(f'      "file_path": "{bpy.path.abspath(t.filepath)}"\n')
        f.write('    }')
        if i < len(textures) - 1:
            f.write(',')
        f.write('\n')

    f.write('  ],\n')

            
    f.write('  "meshes": [\n')
    for l, mesh in enumerate(meshes):
        f.write('    {\n')
        print("Exporting mesh:", mesh.name)
        f.write(f'      "name": "{mesh.name}",\n')
        f.write('      "vertices": [\n')
        for i, v in enumerate(mesh.vertices):
            f.write(f'        {v.co.x}, {v.co.y}, {v.co.z}')
            if i < len(mesh.vertices) - 1:
                f.write(',')
            f.write('\n')
        f.write('      ],\n')
        f.write('      "normals": [\n')
        for i, v in enumerate(mesh.vertices):
            f.write(f'        {v.normal.x}, {v.normal.y}, {v.normal.z}')
            if i < len(mesh.vertices) - 1:
                f.write(',')
            f.write('\n')
        f.write('      ],\n')

        if mesh.uv_layers.active is not None:
            uvs = [(0.0, 0.0)] * len(mesh.vertices)
            for lp in mesh.loops:
                # access uv loop:
                uv_loop = mesh.uv_layers.active.data[lp.index]
                uv_coords = uv_loop.uv
                uvs[lp.vertex_index] = (uv_coords.x, uv_coords.y)
            f.write('      "uvs": [\n')
            for i, uv in enumerate(uvs):
                f.write(f'        {uv[0]}, {uv[1]}, 0')
                if i < len(uvs) - 1:
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
        if l < len(meshes) - 1:
            f.write(',\n')
        else:
            f.write('\n')
    f.write('  ],\n')

    f.write('  "objects": [')
    for l, obj in enumerate(objects):
        if obj.hide_get():
            continue
        f.write('{');
        mesh = obj.data;
        matrix = obj.matrix_world
        location = obj.location
        if len(obj.material_slots) > 1: 
            #print("Warning: Object has multiple materials, only the first will be exported.")
            pass
        if obj.material_slots[0].material in materials:
            f.write(f'    "material_index": {materials.index(obj.material_slots[0].material)},\n')
        f.write(
f'"ref":{meshes.index(mesh)},"transform":[\
{matrix[0][0]},{matrix[0][1]},{matrix[0][2]},{matrix[0][3]},\
{matrix[1][0]},{matrix[1][1]},{matrix[1][2]},{matrix[1][3]},\
{matrix[2][0]},{matrix[2][1]},{matrix[2][2]},{matrix[2][3]},\
0,0,0,1\
]')

        f.write('}')
        if l < len(objects) - 1:
            f.write(',')
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
