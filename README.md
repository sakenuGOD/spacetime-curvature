# spacetime-curvature

Interactive 3D visualization of how massive objects bend spacetime.

A planet sits on a deformable grid (representing the spacetime fabric) and warps it with its mass. A satellite orbits along the curved surface — the heavier the planet, the deeper the curvature.

![preview](preview.jpg)

## Controls

- **`=` / `+`** — increase planet mass
- **`-`** — decrease planet mass
- **Hold Shift** — faster mass change
- **Esc** — quit

## How it works

- 200×200 deformable grid rendered in real-time
- Grid vertices displaced by distance from the massive object (inverse-square-style falloff, clamped)
- Satellite follows a fixed orbital radius at the sphere's settled height
- Background star field for depth
- Raw OpenGL — no engine, no physics library

## Stack

`C++` · `OpenGL` · `GLEW` · `GLFW` · `CMake`

## Build

Requires OpenGL, GLEW and GLFW. On Windows the easiest way is [vcpkg](https://vcpkg.io/):

```bash
vcpkg install glew glfw3
```

Then configure and build:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

On Linux / macOS install GLEW and GLFW via your package manager (`apt install libglew-dev libglfw3-dev`, `brew install glew glfw`) and run the same `cmake` commands without the toolchain file.

The executable lands in `build/` (or `build/Release` on multi-config generators).
