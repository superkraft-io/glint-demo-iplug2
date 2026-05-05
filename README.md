# glint-iplug2-demo


A demo audio plugin built on top of [glint](https://github.com/superkraft-io/glint) and [iPlug2](https://github.com/iPlug2/iPlug2OOS).

---

## Requirements

- **Node.js** (v18+)
- **CMake** (3.25+)
- **Windows**: Visual Studio 2022 with the "Desktop development with C++" workload
- **macOS**: Xcode command line tools
- **Python 3** (only required when building Skia from source — not needed for prebuilt)
- **Git**

---

## Getting Started

### 1. Clone the repo

```sh
git clone https://github.com/superkraft-io/glint-iplug2-demo
cd glint-iplug2-demo
```

### 2. Init submodules

```sh
git submodule update --init --recursive
```

### 3. Init Skia

#### Option A — Prebuilt libraries (recommended, fast)

Download prebuilt Skia libraries — no Python, no lengthy compile. This only needs to be run once (or again if you switch backends).

```sh
# macOS — Metal (recommended)
node third_party/glint/scripts/init_skia.mjs --prebuilt --backend metal

# Windows — Direct3D 12 (recommended)
node third_party/glint/scripts/init_skia.mjs --prebuilt --backend d3d12
```

#### Option B — Build from source

If you need a custom configuration, you can build Skia from source instead. This takes significantly longer.

```sh
# macOS — Metal (recommended)
node third_party/glint/scripts/init_skia.mjs --source --backend metal --config Both

# Windows — Direct3D 12 (recommended)
node third_party/glint/scripts/init_skia.mjs --source --backend d3d12 --config Both
```

**Available backends by platform:**

| Backend | Platform | Description |
|---------|----------|-------------|
| `metal` | macOS | Metal (GPU) — recommended for macOS |
| `d3d12` | Windows | Direct3D 12 (GPU) — recommended for Windows |
| `opengl` | Windows | OpenGL (GPU) |
| `dawn` | macOS, Windows | Dawn / WebGPU (GPU) |
| `cpu` | macOS, Windows | Software CPU renderer |

> `--config Both` builds both Debug and Release Skia libraries, which are required for the respective CMake build configurations. Only applicable for `--source`.

---

### 4. Build & Run

Open the `glint-iplug2-demo` folder in VS Code. Build tasks and launch configurations are already set up in `.vscode/`.

Use the **Run and Debug** panel (`Ctrl+Shift+D` / `⇧⌘D`) and pick a configuration:

| Icon | Bundle mode | Description |
|------|-------------|-------------|
| 💽 | No bundle | C++ uses source files directly from `glint_user_code/web/` |
| 📦 | Shallow bundle | Glint web assets are bundled (filenames only) |
| 🧱 | Deep bundle | Glint web assets are fully inlined into the binary |

**macOS** — select any configuration under `MacOS › Standalone`, e.g. `💽 Debug`, then press **F5**. The CMake build runs automatically before launch.

**Windows** — select any configuration under `Windows › Standalone` or `Windows › VST3 (Reaper)`, e.g. `💽 Debug +v`, then press **F5**. MSBuild runs automatically before launch.

> For a first build, `💽 Debug` (no bundle) is the fastest option — no bundling step required.

---

## VS Code

Open the `glint-iplug2-demo` folder in VS Code. Launch configurations and build tasks are already set up in `.vscode/`. Use the **Run and Debug** panel to build and launch the demo.

---

## License

The **Glint engine** (`third_party/glint`) is a commercial product.

- Free for non-commercial use: https://superkraft.io/license-free
- Commercial use & terms: https://superkraft.io/terms
- Licensing inquiries: **hello@superkraft.io**

The demo code in this repository is provided solely for evaluation purposes.

