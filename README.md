<img src="assets/logo.png" alt="fN0B" width="160">

# fN0B

**fN0B** is a one-knob effect. Pick a preset, turn the knob, and hear the whole chain move. Each preset is one effect or a short chain, and the knob sweeps its key parameters together so every position sounds intentional. Three presets ship: Warm Drive, Filter Funk, Bitcrush Destruct.

VST3. Stupid Audio. [MIT](LICENSE). Site: [docs/](docs/index.html). Manual: [docs/docs.html](docs/docs.html).

The site backdrop is `docs/fx.js`, a PixiJS scene kept in sync with the other Stupid Audio sites. Add `?fx` to any page to preview it full screen with the page chrome hidden; `?fx=false` keeps the normal page.

## Release

Windows, Linux, and macOS files come from GitHub Actions (`.github/workflows/dist.yml`). Run it from the Actions tab for artifacts only, or push a numeric tag such as `0.0.1` (no `v` prefix) to build and publish a GitHub Release with the files attached. `<version>` is the `version` in `fN0B.jucer`.

The `dist` artifact holds:

- `fN0B-<version>-win-x64-vst3.zip`
- `fN0B-<version>-win-x64-installer.msi`
- `fN0B-<version>-linux-x64-vst3.zip`
- `fN0B-<version>-mac-universal-vst3.zip`

The MSI installs the VST3 into `Common Files\VST3` by default; the folder can be changed.

The Mac zip is ad-hoc signed, not notarized. Gatekeeper blocks it until you right-click Open, or run `xattr -dr com.apple.quarantine` on the `.vst3`. The Linux build uses the system ALSA, FreeType, fontconfig, and X11 libraries.

`just build-dist` on Windows writes the VST3 zip and the MSI. Building the MSI needs the .NET 8 SDK.
