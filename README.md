# Dreamverb
> **Sound Capsule** · VST3 + AU · Built with Plugin Studio

---

## Build in 3 steps (Mac, one-time setup)

### Step 1 — Unzip & open Terminal
Drag the unzipped `Dreamverb` folder into your Desktop, then open Terminal.

### Step 2 — Run the build script
```bash
cd ~/Desktop/Dreamverb
chmod +x build.sh
./build.sh
```

The script will:
- Install Homebrew if missing
- Install CMake + Git if missing
- Download JUCE automatically (~200MB, first run only)
- Compile Dreamverb
- Copy `Dreamverb.vst3` → `~/Library/Audio/Plug-Ins/VST3/`

### Step 3 — Open Ableton
Preferences → Plug-Ins → Rescan  
Look for **Sound Capsule** in your plugin browser → **Dreamverb** is there.

---

## Parameters

| Label | ID | Min | Max | Default |
|-------|----|-----|-----|---------|
| MIX | `mix` | 0 | 1 | 0.5 |
| SIZE | `size` | 0 | 1 | 0.6 |
| PRE | `pre` | 0 | 200 | 20 |
| DAMP | `damp` | 0 | 1 | 0.4 |

---

## Rebuild after UI changes

Just run `./build.sh` again — it skips the JUCE download and recompiles in ~30 seconds.

---
*Plugin Studio · Sound Capsule / Plugin Corp*