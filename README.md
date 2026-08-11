# Survival Arena

**Survival Arena** is a survival FPS developed in **C++14 / OpenGL** for the **INFO-H502 – 3D Graphics** course.

The player moves through a nocturnal arena surrounded by a dense forest and must survive as long as possible against several types of ghosts. The project notably applies instanced rendering, framebuffers, screen-space refraction, particle systems, BVH-accelerated intersection tests, and several advanced shaders.

---

## Important: origin of the project structure

Survival Arena was not started from a completely empty OpenGL/CMake project. Setting up the complete OpenGL infrastructure, CMake configuration, camera system, shaders, and model loading from scratch proved difficult, so the project was developed **based on the structure and solutions provided for LAB03 as part of the course**.

This is why the directory structure may look unusual for a standalone project: the game is located in `LAB03/survival_arena`, but directly depends on several files originating from the laboratory, some of which were modified during the development of Survival Arena, in particular:

```text
LAB03/camera.h
LAB03/shader.h
LAB03/solutions/object.h
```

For example, the code contains relative includes such as:

```cpp
#include "../camera.h"
#include "../shader.h"
#include "../solutions/object.h"
```

It is therefore important to **preserve the relative positions of the files**. Moving only the sources from `survival_arena` into a different folder would break these includes as well as some paths used by the project.

---

## Tested configuration

The project was developed and tested with:

- Windows 10/11;
- Visual Studio 2022;
- MSVC x64;
- CMake;
- C++14;
- OpenGL 4.0 Core Profile.

A graphics card and driver supporting **OpenGL 4.0 or later** are required.

The documented configuration targets Windows. GLFW, GLAD, GLM, and stb_image are cross-platform, but other operating systems were not validated for the final version of the project.

---

## Dependencies

| Dependency | Purpose | Expected installation |
|---|---|---|
| OpenGL 4.0+ | Rendering API | Provided by the graphics driver |
| GLFW | Window, OpenGL context, and input handling | Included in `3rdParty` |
| GLAD | OpenGL function loading | Included in `3rdParty` |
| GLM | Vectors, matrices, and transformations | Included in `3rdParty` |
| stb_image | Image and texture loading | Included in `3rdParty` |
| CMake | Project configuration | Visual Studio or separate installation |
| MSVC / C++14 | C++ compilation | Visual Studio 2022 |

The project also contains a **custom OBJ/MTL loader** used to load models and their materials.

---

## Where to place the files from the ZIP

The tree below corresponds to the actual organization used during development. The `LAB03` directory must remain intact: **the Survival Arena sources are not standalone when removed from this structure**.

The directory called `project-root/` below corresponds to the parent directory of `LAB03` in the INFO-H502 repository.

```text
project-root/
├── 3rdParty/                         # course framework dependencies, if provided
│
└── LAB03/
    ├── CMakeLists.txt
    ├── camera.h
    ├── shader.h
    │
    ├── exercices/                    # original LAB03 files
    │   ├── object.h
    │   └── ex01 ... ex11
    │
    ├── solutions/
    │   ├── object.h                  # OBJ/MTL loader used by Survival Arena
    │   ├── object.cpp
    │   └── ex01 ... ex10
    │
    ├── survival_arena/
    │   ├── main.cpp
    │   ├── collision.cpp
    │   ├── collision.h
    │   ├── enemy.cpp
    │   ├── enemy.h
    │   ├── gameplay.cpp
    │   ├── gameplay.h
    │   ├── game_settings.h
    │   ├── game_state.h
    │   ├── renderer.cpp
    │   ├── renderer.h
    │   ├── render_resources.cpp
    │   ├── render_resources.h
    │   ├── shader_sources.cpp
    │   └── shader_sources.h
    │
    ├── objects/
    │   ├── cube.obj
    │   ├── EnergyBlaster/
    │   ├── Ghosts/
    │   ├── Nature_Pack/
    │   └── Scenery/
    │
    └── textures/
        ├── cubemaps/
        │   └── moonlit_night/
        ├── EnemyShield/
        ├── Forest/
        ├── Ghosts/
        └── Ground/
            └── Grass/
```

### Files placed directly in `LAB03/`

The following files remain directly at the root of `LAB03`:

```text
camera.h
shader.h
CMakeLists.txt
```

They must not be moved into `survival_arena/`.

### OBJ/MTL loader

The game uses the modified loader originating from the LAB03 solutions:

```text
LAB03/solutions/object.h
LAB03/solutions/object.cpp
```

`main.cpp` and `renderer.cpp` explicitly include `../solutions/object.h`.

### Survival Arena-specific files

All of the following files must remain together in:

```text
LAB03/survival_arena/
```

```text
main.cpp
collision.cpp
collision.h
enemy.cpp
enemy.h
gameplay.cpp
gameplay.h
game_settings.h
game_state.h
renderer.cpp
renderer.h
render_resources.cpp
render_resources.h
shader_sources.cpp
shader_sources.h
```

### 3D models

All models actually used by the game are loaded from:

```text
LAB03/objects/
```

The important subdirectories are:

```text
LAB03/objects/EnergyBlaster/
LAB03/objects/Ghosts/
LAB03/objects/Nature_Pack/
LAB03/objects/Scenery/
```

The game also uses:

```text
LAB03/objects/cube.obj
```

It is strongly recommended to **copy the complete `objects` directory instead of manually selecting files**, because `.obj` files may reference `.mtl` files, which may themselves reference textures.

### Textures

All textures loaded directly by the game must remain in:

```text
LAB03/textures/
```

The main resources include:

```text
LAB03/textures/cubemaps/moonlit_night/
LAB03/textures/EnemyShield/GreenShield.jpg
LAB03/textures/Forest/forest_texture.png
LAB03/textures/Ghosts/Atlas_Monsters.png
LAB03/textures/Ground/Grass/Grass_normal_up.png
LAB03/textures/Ground/Grass/Grass_darked_up.png
```

The `moonlit_night` cubemap must keep its six images:

```text
negx.jpg
negy.jpg
negz.jpg
posx.jpg
posy.jpg
posz.jpg
```

Again, keeping the complete `textures` directory unchanged is the safest option.

### `3rdParty` dependencies

The directory tree shown above starts at `LAB03`, so it does not display dependencies located higher in the course repository. If the final ZIP includes `3rdParty`, this directory must be placed **at the same level as `LAB03`**, not inside it:

```text
project-root/
├── 3rdParty/
└── LAB03/
```

If the ZIP does not contain the dependencies from the course framework, the person compiling the project must use the original INFO-H502 repository and replace/add the provided `LAB03` directory.

### Development folders not required by the game

The current development tree also contains several working directories:

```text
LAB03/survival_arena/all/
LAB03/survival_arena/all.zip
LAB03/survival_arena/save/
LAB03/survival_arena/Evolution/
LAB03/survival_arena/picture/
LAB03/survival_arena/Asset/
```

`all/`, `all.zip`, and `save/` are code copies or backups and can be removed from the final ZIP.

`Evolution/` and `picture/` contain development images and are not required at runtime.

`Asset/` mainly contains original/source assets used during development. The final game loads its models and textures from `LAB03/objects` and `LAB03/textures`, so `Asset/` is not required to run the game unless it is kept for archival or attribution purposes.

---

## Resources and paths

Models and textures are loaded using the macros:

```cpp
PATH_TO_OBJECTS
PATH_TO_TEXTURE
```

These paths are provided to the program during CMake configuration.

It is therefore important to **reconfigure CMake after extracting or moving the project to another computer**. An executable compiled on another machine may still contain paths corresponding to the previous project location.

If models or textures cannot be found, delete the build directory and reconfigure the project cleanly:

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target SurvivalArena
```

---

## Controls

| Action | Control |
|---|---|
| Move forward | `W` |
| Move backward | `S` |
| Move left | `A` |
| Move right | `D` |
| Look around | Mouse |
| Alternative rotation | Arrow keys |
| Shoot | Left mouse button |
| Restart after Game Over | `R` |
| Particle geometry shader ON/OFF | `F1` |
| Weapon toon shader ON/OFF | `F2` |
| Monolith bump mapping ON/OFF | `F3` |
| Quit | `Escape` |

The three advanced features controlled with `F1`, `F2`, and `F3` are **disabled by default** so that their visual contribution can be compared directly during gameplay.

On an AZERTY keyboard, the code still uses the physical keys identified by GLFW as `W`, `A`, `S`, and `D`.

---

## Code organization

The project was refactored to separate gameplay logic, geometric computations, and rendering.

### `main.cpp`

Initializes GLFW/GLAD/OpenGL, loads resources, generates the scene, prepares shaders, and orchestrates the main loop.

### `game_settings.h`

Centralizes editable game parameters: camera, arena, vegetation, enemies, weapon, lighting, HUD, particles, and rendering.

### `game_state.h`

Stores the mutable runtime state: player, enemies, weapon, particles, timers, Game Over state, and advanced-feature toggles.

### `gameplay.cpp`

Handles input, camera movement constraints, enemy spawning and updates, damage, shooting, recoil, particles, restart logic, and the `F1/F2/F3` toggles.

### `enemy.cpp`

Handles the three enemy categories, their transformations, resistant-enemy shields, and processing of weapon impacts on enemies.

### `collision.cpp`

Contains the geometric tests used by the game: ray/sphere, ray/AABB, ray/triangle, broad-phase ray corridor, and BVH construction/traversal.

### `renderer.cpp`

Performs the rendering passes: opaque scene, shields, spectral ghosts, particles, energy beam, FPS weapon, damage vignette, and HUD.

### `render_resources.cpp`

Creates the main auxiliary OpenGL resources, including the scene framebuffer and HUD geometry.

### `shader_sources.cpp`

Contains the GLSL source code used by the different rendering effects.

### `object.h`

Implements the custom OBJ/MTL loading code, OpenGL buffer creation, materials, and textures associated with models.

---

## Advanced features that can be compared at runtime

### `F1` — Particle geometry shader

- **OFF**: particles are rendered as point sprites;
- **ON**: each point is expanded by a geometry shader into a screen-aligned quad.

Particle simulation remains CPU-based in both modes.

### `F2` — Weapon toon shader

- **OFF**: the weapon uses standard lighting;
- **ON**: diffuse and specular illumination are quantized to produce a toon-like appearance.

No additional outline is rendered.

### `F3` — Monolith bump mapping

- **OFF**: reflection is computed using the original mesh normals;
- **ON**: normals are perturbed by a procedural height field generated from 3D noise.

The bump mapping affects shading and reflection, not the geometry or silhouette of the monolith.

---

## Game parameters

The main gameplay and rendering parameters are centralized in:

```text
LAB03/survival_arena/game_settings.h
```

This file includes parameters for:

- camera movement;
- arena dimensions;
- vegetation density;
- enemy speeds and probabilities;
- difficulty and spawn frequency;
- health and damage;
- weapon cooldown and penetration;
- lighting and fog;
- spectral-ghost visibility;
- HUD;
- particles and death effects.

The pseudo-random generators use fixed seeds so that the scene and behavior remain reproducible between executions.

---

## Use of Large Language Models (LLMs)

Several AI models were used either as tutorials or as guides for features to help figure out "how to do it."
The code was also cleaned up, reorganized into multiple files, and annotated for better readability using LLMs.

I take full responsibility for all the code. Every section written with the help of artificial intelligence has been reviewed and understood.
