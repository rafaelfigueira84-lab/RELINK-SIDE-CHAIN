# RELINK Smart Sidechain v0.1 — Experimental

VST3 stereo for Cubase 10.5 Pro / Windows 10. Built using JUCE 8.0.10 (downloaded during CMake configure).

## What it does

- External audio sidechain input receives kick audio (not MIDI).
- Detects kick transients using a smoothed amplitude envelope and adjustable detection threshold.
- Learns a rough duration from the kick envelope, gradually updating its estimate in **AUTO TIMING** mode.
- Applies a smooth gain ducking envelope to the stereo instrument track.
- **AMOUNT** (0–100%) scales depth; **MAX REDUCTION** sets the upper dB limit.
- Manual duration, timing offset, state persistence and live numerical metering.

**Important limitations:** this is an experimental first-pass detector, not an AI model and not a spectrum-aware or multi-track automatic mixer. It does not currently analyze spectral masking between kick and bass. Its estimated duration depends on threshold and source level. There is no lookahead: offset can delay ducking, but cannot begin attenuation before an incoming kick transient. No guarantee of optimal settings or artifact-free processing on every source. A later version can add spectral analysis, calibration and lookahead latency handling.

## Windows build without local development tools

1. Create a GitHub repository and upload all contents of this directory, preserving `.github/workflows/windows-vst3.yml`.
2. Open **Actions > Build Windows VST3 > Run workflow**.
3. Download the `RELINK-Smart-Sidechain-Windows-VST3` build artifact, unzip it.
4. Copy the entire `RELINK Smart Sidechain.vst3` bundle into `C:\Program Files\Common Files\VST3\`.
5. Restart Cubase, rescan plugins if necessary.

Alternatively, with Visual Studio 2022 and CMake 3.22+, run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 4
```

## Cubase routing

1. Place this plugin as an insert **on the bass track**, not on the kick or master.
2. Activate the plugin's sidechain input in Cubase.
3. On the kick track, add a send targeting **RELINK Smart Sidechain – Sidechain**, and enable that send. Start with the send at 0 dB.
4. Play a section with kick and bass. Check that the plugin reads `KICK DETECTED`.
5. Enable AUTO TIMING, set AMOUNT around 50–65%, MAX REDUCTION around 8–12 dB and adjust by ear.
6. If the plugin triggers too often or misses kicks, adjust KICK DETECTION threshold. If needed, disable AUTO and set a fixed duration.

**Note:** external sidechain availability and channel activation are determined by the host and the plugin bus layout. Always test with audio first and verify the sidechain signal is reaching the plugin.
