#!/usr/bin/end python3

import json
import random
from copy import deepcopy
import numpy as np
from argparse import ArgumentParser

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


def gen_sphere():
    return {'type': 'sphere', 'radius': 2.0 * random.random()}


def gen_box():
    return {'type': 'box', 'dim': [2.0 * random.random(), 2.0 * random.random(), 2.0 * random.random()]}


def gen_cylinder():
    return {'type': 'cylinder', 'height': 2.0 * random.random(), 'radius': 1.0 * random.random()}


def gen_torus():
    return {'type': 'torus', 'radiusRevolve': 2.0 * random.random() + 1.0, 'radius': 1.0 * random.random()}


PRIMITIVES = {
    "sphere": gen_sphere,
    "box": gen_box,
    "cylinder": gen_cylinder,
    "torus": gen_torus,
}


def gen_union(a, b):
    return {'type': 'union', 'a': a, 'b': b}


def gen_subtraction(a, b):
    return {'type': 'subtraction', 'a': a, 'b': b}


def gen_intersection(a, b):
    return {'type': 'intersection', 'a': a, 'b': b}


def gen_smooth_union(a, b):
    return {'type': 'smoothUnion', 'a': a, 'b': b, 'radius': 0.5 * random.random() + 0.2}


def gen_smooth_subtraction(a, b):
    return {'type': 'smoothSubtraction', 'a': a, 'b': b, 'radius': 0.5 * random.random() + 0.2}


def gen_smooth_intersection(a, b):
    return {'type': 'smoothIntersection', 'a': a, 'b': b, 'radius': 0.5 * random.random() + 0.2}


OPERATIONS = {
    "union": gen_union,
    "subtraction": gen_subtraction,
    "intersection": gen_intersection,
    "smoothUnion": gen_smooth_union,
    "smoothSubtraction": gen_smooth_subtraction,
    "smoothIntersection": gen_smooth_intersection
}
CORE = {
    "spp": 32,
    "resolution": [1000, 1000],
    "camera": {
        "fov": 1.5707,
        "center": [0.0, 0.0, 0.0],
        "up": [0.0, 1.0, 0.0],
        "pos": [0.0, 0.0, -6.0]
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


def generate_box(JSON, size=14, colored=True, light=True, bright=False):
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


def generate_primitive(JSON, shape=None, material=True, rotate=True, translate=False):
    if shape is None:
        shape = random.choice(list(PRIMITIVES.keys()))
    base = PRIMITIVES[shape]()
    key = '{}-{}'.format(shape, random.randint(0, 9999))
    if material:
        base['material'] = select_material(
            JSON, options=material if isinstance(material, str) else 'dgml')
    if rotate:
        base['rotation'] = [2 * np.pi * random.random(), 2 * np.pi *
                            random.random(), 2 * np.pi * random.random()]
    if translate:
        base['position'] = [2 * random.random(), 2 *
                            random.random(), 2 * random.random()]
    JSON['objects'][key] = base
    return key, JSON


def generate_joint(JSON, count, material=True, rotate=True, translate=False, children=[]):
    while count > 0:
        key, JSON = generate_primitive(
            JSON, material=False, rotate=True, translate=True)
        children.append(key)
        count -= 1
    shape = random.choice(list(OPERATIONS.keys()))
    key = '{}-{}'.format(shape, random.randint(0, 9999))
    base = OPERATIONS[shape](children[0], children[1])
    if rotate:
        base['rotation'] = [2 * np.pi * random.random(), 2 * np.pi *
                            random.random(), 2 * np.pi * random.random()]
    if translate:
        base['position'] = [2 * random.random(), 2 *
                            random.random(), 2 * random.random()]
    JSON['objects'][key] = base
    children = children[2:]
    children.append(key)
    if len(children) > 1:
        JSON = generate_joint(JSON, 0, material, rotate=True,
                              translate=True, children=children)
    else:
        JSON['objects'][key]['material'] = select_material(
            JSON, options=material)
    return JSON


def main():
    parser = ArgumentParser('trm-showcase')
    parser.add_argument('-l', '--light', action='store_true',
                        help='enables emissive materials')
    parser.add_argument('-p', '--primitive', action='store_true',
                        help='renders only a single primitives')
    parser.add_argument('-n', '--num', default=3, type=int,
                        help='number of primitives')
    parser.add_argument('-c', '--count', default=1, type=int,
                        help='number of scenes to generate')
    parser.add_argument('-s', '--start', default=0, type=int,
                        help='starting index for saving')
    opts = parser.parse_args()
    output = generate_materials(deepcopy(CORE))
    output = generate_box(output, light=not opts.light)
    with open("../example/{:04}.json".format(opts.start), "w") as file:
        json.dump(output, file)
    for i in range(opts.start + 1, opts.start + opts.count):
        output = generate_materials(deepcopy(CORE))
        output = generate_box(output, light=not opts.light)
        if opts.primitive:
            _, output = generate_primitive(
                output, material='l' if opts.light else 'dgm')
        else:
            output = generate_joint(
                output, opts.num, material='l' if opts.light else 'dgm')
        with open("../example/{:04}.json".format(i), "w") as file:
            json.dump(output, file)


if __name__ == "__main__":
    main()
