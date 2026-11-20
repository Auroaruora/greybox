# Setup

## Clone

```bash
git clone --recursive <repo-url>
```

Already cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/greybox
```

Release build, without the debug tools:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
./build-release/greybox
```

## Export from Blender

Needs Blender 4.5. Writes every scene in `blender/scenes/` into `assets/`.

```bash
cd blender
/Applications/Blender.app/Contents/MacOS/Blender -b --python scripts/export_layout.py
```

## Libraries

| Library | Version | In |
|---|---|---|
| GLFW | 3.4 | `external/glfw` (submodule) |
| GLM | 1.0.1 | `external/glm` (submodule) |
| GLEW (glew-cmake) | 2.2.0 | `external/glew` (submodule) |
| Dear ImGui | 1.92.5 | `external/imgui` (submodule) |
| stb | `9600522` | `external/stb` (submodule) |
| nlohmann/json | 3.12.0 | `external/json/nlohmann/json.hpp` |
