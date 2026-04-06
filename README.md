# spacetime-curvature

Interactive 3D visualization of how massive objects bend spacetime.

A planet sits on a deformable grid (representing spacetime fabric) and warps it with its mass. A satellite orbits along the curved surface — the heavier the planet, the deeper the curvature and the tighter the orbit.

## Controls

- **Mouse wheel** — change planet mass (grid deformation depth)
- **Hold Shift** — faster mass change

## How it works

- 200x200 deformable grid rendered in real-time
- Grid vertices displaced based on distance from the massive object (inverse-square falloff)
- Satellite follows the curved surface at a fixed orbital radius
- Background star field for depth
- Built with raw OpenGL — no engine, no physics library

## Stack

`C++` `OpenGL` `GLEW` `GLFW`

## Build

Open `.sln` in Visual Studio, build and run. Requires GLEW and GLFW libraries.
