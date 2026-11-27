class Map:
    def __init__(self, value, **fixed):
        self.value = value
        self.fixed = fixed


class Either:
    def __init__(self, *options):
        self.options = options


VEC3 = [float]
NAMES = [str]

ASSET = {
    "parts": NAMES,
    "shapes": NAMES,
    "colors": Map(Map(Either(str, NAMES), shapes=NAMES)),
    "materials?": Map({"color": str}),
}

PALETTE = Map({"color": str, "linear": VEC3, "emission?": VEC3})

SCENE = {
    "shading": str,
    "material?": {"specular": VEC3, "shininess": float},
    "camera?": {"far": float},
    "start": {"look": str, "viewpoint": str, "state": str},
}

LAYOUT = {"objects": Map({"asset": str, "position": VEC3, "rotation": VEC3, "group": str})}

LOOK = {"objects": Map({"shape?": str, "color?": str, "scale?": VEC3})}

GLOW = {"strength": float, "color": VEC3, "base?": VEC3}

LIGHT = {
    "type": str,
    "attach?": str,
    "glowPart?": str,
    "position?": VEC3,
    "direction?": VEC3,
    "angle?": float,
    "penumbra?": float,
    "softSize?": float,
    "shadow?": bool,
}

LIGHT_SETTING = {"intensity?": float, "color?": VEC3, "direction?": VEC3, "glow?": GLOW}

STATE = {
    "ambient": VEC3, "sky?": {"zenith": VEC3, "horizon": VEC3}, "toon?": {"shade": VEC3},
    "lights": Map(LIGHT_SETTING),
    "glow?": Map(Map(GLOW)),
}

LIGHTING = {"lights": Map(LIGHT), "states": Map(STATE)}

VIEW = {"position": VEC3, "look": VEC3, "up": VEC3, "heightAngle": float}

VIEWPOINTS = Map(VIEW)

PATH = [{"time": float, "position": VEC3, "look": VEC3, "up": VEC3, "heightAngle": float}]

MOTIONS = Map({"pivot": str, "spin?": VEC3, "sway?": VEC3, "period?": float})


def problems(value, spec, where="root"):
    if spec is float:
        return [] if isinstance(value, (int, float)) and not isinstance(value, bool) else ["%s: expected a number" % where]
    if spec in (str, bool):
        return [] if isinstance(value, spec) else ["%s: expected %s" % (where, spec.__name__)]
    if isinstance(spec, Either):
        found = [problems(value, option, where) for option in spec.options]
        return [] if any(not f for f in found) else found[0]
    if isinstance(spec, list):
        if not isinstance(value, list):
            return ["%s: expected a list" % where]
        return [p for i, item in enumerate(value) for p in problems(item, spec[0], "%s[%d]" % (where, i))]
    if isinstance(spec, Map):
        if not isinstance(value, dict):
            return ["%s: expected an object" % where]
        return [p for key, item in value.items()
                for p in problems(item, spec.fixed.get(key, spec.value), "%s.%s" % (where, key))]
    if not isinstance(value, dict):
        return ["%s: expected an object" % where]
    found = []
    keys = {key.rstrip("?"): key.endswith("?") for key in spec}
    for key, optional in keys.items():
        if key not in value:
            if not optional:
                found.append("%s: missing %s" % (where, key))
            continue
        found += problems(value[key], spec[key + "?" if optional else key], "%s.%s" % (where, key))
    found += ["%s: unknown key %s" % (where, key) for key in value if key not in keys]
    return found


def check(value, spec, where):
    found = problems(value, spec, where)
    if found:
        raise ValueError("\n".join(found))
    return value
