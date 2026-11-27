import math

from mathutils import Euler, Matrix, Quaternion, Vector

Y_UP = Matrix(((1, 0, 0), (0, 0, 1), (0, -1, 0)))
Z_UP = Y_UP.transposed()
CAMERA_ASPECT = 16 / 9
SUN_SCALE = 1 / math.pi
LAMP_SCALE = 1 / (4 * math.pi ** 2)
FORWARD = Vector((0, 0, -1))
POSITION_DIGITS = 5
DIRECTION_DIGITS = 6
ANGLE_DIGITS = 4
INTENSITY_DIGITS = 6


def rounded(values, digits):
    return [round(v, digits) + 0.0 for v in values]


def to_engine_point(point):
    return rounded(Y_UP @ Vector(point), POSITION_DIGITS)


def to_blender_point(point):
    return Z_UP @ Vector(point)


def to_engine_direction(direction):
    return rounded((Y_UP @ Vector(direction)).normalized(), DIRECTION_DIGITS)


def to_blender_direction(direction):
    return (Z_UP @ Vector(direction)).normalized()


def to_engine_rotation(euler):
    turn = Y_UP @ Euler(euler, "XYZ").to_matrix() @ Z_UP
    return rounded([math.degrees(a) for a in turn.to_euler("ZXY")], ANGLE_DIGITS)


def to_blender_rotation(degrees):
    turn = Euler([math.radians(a) for a in degrees], "ZXY").to_matrix()
    return (Z_UP @ turn @ Y_UP).to_euler("XYZ")


def to_engine_scale(scale):
    return [scale[0], scale[2], scale[1]]


def to_blender_scale(scale):
    return [scale[0], scale[2], scale[1]]


def aim(direction):
    return Vector(direction).to_track_quat("-Z", "Y").to_euler()


def facing(rotation):
    turn = rotation if isinstance(rotation, Quaternion) else rotation.to_quaternion()
    return turn @ FORWARD


def light_scale(kind):
    return SUN_SCALE if kind == "sun" else LAMP_SCALE


def to_engine_spot(size, blend):
    half = size / 2
    inner = math.acos(math.cos(half) + (1 - math.cos(half)) * blend)
    return round(math.degrees(half), ANGLE_DIGITS), round(math.degrees(half - inner), ANGLE_DIGITS)


def to_blender_spot(angle, penumbra):
    half = math.radians(angle)
    inner = half - math.radians(penumbra)
    blend = (math.cos(inner) - math.cos(half)) / (1 - math.cos(half)) if half > 0 else 0.0
    return 2 * half, max(0.0, min(1.0, blend))


def to_engine_camera(location, rotation, fov):
    turn = Euler(rotation, "XYZ").to_matrix()
    height = 2 * math.atan(math.tan(fov / 2) / CAMERA_ASPECT)
    return {"position": to_engine_point(location),
            "look": to_engine_direction(turn @ FORWARD),
            "up": to_engine_direction(turn @ Vector((0, 1, 0))),
            "heightAngle": round(math.degrees(height), ANGLE_DIGITS)}


def to_blender_camera(view):
    look, up = to_blender_direction(view["look"]), to_blender_direction(view["up"])
    right = look.cross(up).normalized()
    up = right.cross(look).normalized()
    turn = Matrix((right, up, -look)).transposed()
    fov = 2 * math.atan(math.tan(math.radians(view["heightAngle"]) / 2) * CAMERA_ASPECT)
    return to_blender_point(view["position"]), turn.to_euler("XYZ"), fov
