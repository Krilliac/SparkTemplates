# 16 — Camera Toolkit  (Intermediate)

An interactive demonstration of every camera mode and effect available in
SparkEngine, from first-person and third-person setups to cinematic paths,
depth of field, camera shake, split-screen, and FOV animation.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| First-person camera | Mouse look + WASD translation |
| Third-person chase | Spring-arm follow with damping |
| Orbit camera | Rotate around a pivot, zoom in/out |
| Free-fly / spectator | 6DOF movement (no gravity) |
| Camera shake | Parametric trauma, frequency, decay |
| Depth of Field | Focus distance, aperture, bokeh |
| Path interpolation | CatmullRom, Linear, CubicBezier keyframes |
| Split-screen | Two cameras rendering to separate viewports |
| Cinematic letterbox | Aspect-ratio bars for widescreen feel |
| FOV animation | Smooth zoom for sniper / sprint effects |

## Controls

| Key | Action |
|-----|--------|
| Tab | Cycle camera modes (FPS / TPS / Orbit / FreeFly) |
| W / A / S / D | Movement (mode-dependent) |
| Mouse | Look / orbit (mode-dependent) |
| F | Toggle Depth of Field |
| K | Trigger camera shake |
| L | Toggle cinematic letterbox |
| V | Toggle split-screen |
| + / - | Adjust FOV |

## Building

Requires an **installed** SparkEngine SDK (see the repository root
[README](../../README.md#prerequisites) for install instructions).

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<path-to-SparkEngine-install-prefix>
cmake --build build --config Release
```

> `CMAKE_PREFIX_PATH` must point at the SparkEngine **install prefix** (the
> directory you passed to `cmake --install --prefix`), **not** the build tree or
> its `bin/` subdirectory.

This produces `CameraToolkit.dll` (Windows) or `libCameraToolkit.so` (Linux).
Run it with the engine:

```bash
# Windows
SparkEngine.exe -game CameraToolkit.dll

# Linux
./SparkEngine -game libCameraToolkit.so
```
