#!/usr/bin/end python3

import json
import random
import numpy as np

COLORS = {
    "White": "#FFFFFF",
    "Red": "#F44336",
    "Pink": "#E91E63",
    "Purple": "#9C27B0",
    "DeepPurple": "#673AB7",
    "Indigo": "#3F51B5",
    "Blue": "#2196F3",
    "PaleBlue": "#03A9F4",
    "Cyan": "#00BCD4",
    "Teal": "#57C7B8",
    "Green": "#4CAF50",
    "PaleGreen": "#8BC34A",
    "Lime": "#CDDC39",
    "Yellow": "#FFEB3B",
    "Amber": "#FFC107",
    "Orange": "#FF9800",
    "DeepOrange": "#FF5722",
    "Brown": "#795548"
}
PRIMATIVES = {
    "sphere": {
        "type": "sphere",
        "radius": "float"
    },
    "box": {
        "type": "box",
        "dim": ["float", "float", "float"]
    },
    "cylinder": {
        "type": "cylinder",
        "height": "float",
        "radius": "float"
    },
    "torus": {
        "type": "torus",
        "radiusRevolve": "float",
        "radius": "float"
    }
}
OPERATIONS = {
    "elongate": {
        "scale": ["float", "float", "float"],
        "object": "obj"
    },
    "round": {
        "radius": "float",
        "object": "obj"
    },
    "onion": {
        "thickness": "float",
        "object": "obj"
    }
}
BOOLEAN = {
    "union": {
        "a": "obj",
        "b": "obj"
    },
    "subtraction": {
        "a": "obj",
        "b": "obj"
    },
    "intersection": {
        "a": "obj",
        "b": "obj"
    },
    "smoothUnion": {
        "radius": "float",
        "a": "obj",
        "b": "obj"
    },
    "smoothSubtraction": {
        "radius": "float",
        "a": "obj",
        "b": "obj"
    },
    "smoothIntersection": {
        "radius": "float",
        "a": "obj",
        "b": "obj"
    },
}
CORE = {
    "spp": 32,
    "resolution": [1000, 1000],
    "camera": {
        "fov": 1.5707,
        "center": [0.0, 0.0, 0.0],
        "up": [0.0, 1.0, 0.0],
        "pos": [0.0, 0.0, -10.0]
    },
    "materials": {},
    "objects": {}
}


def generate_light(color, emission):
    return {
        "shading": 'emissive',
        'color': list(int(color[i:i + 2], 16) / 255 for i in (1, 3, 5)),
        'emission': float(emission)
    }


def generate_diffuse(color):
    return {
        'shading': 'diffuse',
        "color": list(int(color[i:i + 2], 16) / 255 for i in (1, 3, 5))
    }


def generate_specular():
    return {'shading': 'specular'}


def generate_refractive(ior):
    return {'shading': 'refractive', "ior": float(ior)}


def generate_materials(JSON):
    JSON['materials']['mirror'] = generate_specular()
    for ior in np.arange(1.0, 2.1, 0.1):
        JSON['materials']['glass{:3.1f}'.format(ior)] = generate_refractive(
            ior)
    for key, value in COLORS.items():
        JSON['materials']['{}{}'.format(key[0].lower(),
                                        key[1:])] = generate_diffuse(value)
        for em in np.arange(5, 55, 5):
            JSON['materials']['light{}{}'.format(key, em)] = generate_light(
                value, em)
    return JSON


def generate_box(JSON, size=30, colored=True, light=True, bright=False):
    JSON['objects']['boxFloor'] = {
        'type': 'plane',
        'normal': [0.0, 1.0, 0.0, -size / 4],
        'material': 'white'
    }
    JSON['objects']['boxCeiling'] = {
        'type':
        'plane',
        'normal': [0.0, -1.0, 0.0, -3 * size / 4],
        'material':
        ('lightWhite25' if bright else 'lightWhite10') if light else 'white'
    }
    JSON['objects']['boxWallLeft'] = {
        'type': 'plane',
        'normal': [-1.0, .0, 0.0, -size / 2],
        'material': 'red' if colored else 'white'
    }
    JSON['objects']['boxWallRight'] = {
        'type': 'plane',
        'normal': [1.0, .0, 0.0, -size / 2],
        'material': 'blue' if colored else 'white'
    }
    JSON['objects']['boxWallBack'] = {
        'type': 'plane',
        'normal': [0.0, .0, -1.0, -size / 2],
        'material': 'green' if colored else 'white'
    }
    JSON['objects']['boxWallFront'] = {
        'type': 'plane',
        'normal': [0.0, .0, 01.0, -size / 2],
        'material': 'white' if colored else 'white'
    }
    return JSON


def gen_float(obj, values=None):
    if isinstance(obj, dict):
        for key, val in obj.items():
            obj[key], values = gen_float(val, values)
    elif isinstance(obj, list):
        for i, val in enumerate(obj):
            obj[i], values = gen_float(val, values)
    elif isinstance(obj, str) and obj == "float":
        minimum = 0
        maximum = 1
        if values is not None:
            if isinstance(values, list):
                if isinstance(values[0], list):
                    minimum = values[0][0]
                    maximum = values[0][1]
                elif values[0] is not None:
                    minimum = 0
                    maximum = values[0]
            else:
                maximum = values
        obj = (maximum - minimum) * random.random() + minimum
    return obj, values[1:] if values and isinstance(values, list) else values


def select_material(JSON, options='dgml'):
    shading = random.choice(options)
    if shading == 'd':
        return random.choice([
            x for x in JSON['materials'].keys() if not x.startswith('light')
            and not x.startswith('glass') and not x.startswith('mirror')
        ])
    elif shading == 'm':
        return random.choice(
            [x for x in JSON['materials'].keys() if x.startswith('mirror')])
    elif shading == 'g':
        return random.choice(
            [x for x in JSON['materials'].keys() if x.startswith('glass')])
    elif shading == 'l':
        return random.choice(
            [x for x in JSON['materials'].keys() if x.startswith('light')])


def generate_primative(JSON, shape=None, material=True, values=None):
    if shape is None:
        shape = random.choice(list(PRIMATIVES.keys()))
    base, _ = gen_float(PRIMATIVES[shape], values)
    if material:
        base['material'] = select_material(
            JSON, options=material if isinstance(material, str) else 'dgml')
    JSON['objects']['{}-{}'.format(shape, random.randint(0, 9999))] = base
    return JSON


def main():
    output = generate_materials(CORE)
    output = generate_box(output)
    output = generate_primative(output, values=2)
    with open("box.json", "w") as file:
        json.dump(output, file)


if __name__ == "__main__":
    main()
