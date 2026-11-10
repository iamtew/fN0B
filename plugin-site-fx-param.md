# `?fx` preview flag for the other plugin sites

Copy the block below into a Clanker session opened on the target repo.

The flag shipped here first (fN0B) and is identical everywhere. Only the scene
constructor differs, so do not copy this repo's churn maw into another site.

| Repo | Scene call | `#fx` canvas |
| --- | --- | --- |
| K1X | `kickScene(PIXI, glow)` | index.html only |
| SNR3 | `snareScene(PIXI, glow)` | index.html only |
| 5HKR | `grooveScene(PIXI)` | index.html only |
| F4RT | `fartScene(PIXI, glow, w0, h0)` | index.html only |
| V4JB | `fogScene(PIXI, glow, w0, h0)` | index.html only |
| TEW03 | `tewScene(PIXI)` | index.html only |

No repo includes `fx.js` on `docs/docs.html`, so the flag would be a no-op there.
---

Add the `?fx` full-screen preview flag to this plugin site's backdrop.

## Context

`docs/fx.js` is a PixiJS backdrop. `docs/index.html` holds one
`<canvas id="fx" aria-hidden="true">` at the top of `<body>` (line ~22) and loads
`fx.js` as a module. `docs/style.css` positions `#fx` fixed and full-bleed with
`z-index: 0`. The animation is decorative, sitting behind the page.

The sibling repo fN0B already ships this flag. Port it here, adapted to this
repo's scene. Do not copy fN0B's scene — only the flag plumbing.

## What `?fx` means

`?fx`, `?fx=true`, `?fx=1`  -> only the animated canvas, page chrome hidden.
`?fx=false`, `?fx=0`, `?fx=off` -> normal page.
No param at all -> normal page.

Never make the flag a trap: an explicit off must always win.

## Changes

### 1. `docs/fx.js` — parse the flag, once, at module top level

Put this above the `pointer` block, near the other module-level setup:

```js
// ?fx / ?fx=true keeps only the canvas. ?fx=false / 0 / off stays a normal page.
const fxRaw = new URLSearchParams(location.search).get('fx');
const fxOnly = fxRaw !== null && !/^(0|false|off)$/i.test(fxRaw);
if (fxOnly) document.body.classList.add('fx-only');
```

Note this deliberately treats the bare `?fx` form (which parses to `''`) as on,
because the check is "present AND not explicitly off".

### 2. `docs/fx.js` — drop pointer parallax in preview mode

Wrap the existing listener:

```js
const pointer = { x: 0.5, y: 0.5 };
if (!fxOnly) {
  addEventListener('pointermove', (e) => {
    pointer.x = e.clientX / innerWidth;
    pointer.y = e.clientY / innerHeight;
  }, { passive: true });
}
```

And inside `bootTicker`'s ticker callback, wrap the two parallax lerps:

```js
    // Preview mode is full-bleed, so there is no page to parallax against.
    if (!fxOnly) {
      const ease = Math.min(1, dt * 2.5);
      scene.root.x += ((0.5 - pointer.x) * 40 - scene.root.x) * ease;
      scene.root.y += ((0.5 - pointer.y) * 24 - scene.root.y) * ease;
    }
```

### 3. `docs/style.css` — hide everything but the canvas

Add directly after the existing `#fx` rule:

```css
/* ?fx preview: the canvas is the whole page. */
body.fx-only { overflow: hidden; cursor: none; }
body.fx-only > :not(#fx) { display: none; }
```

Keep the chassis background on `body` — the scene is additive and expects that
backdrop. Do not set a new background here.

### 4. `README.md` — one line

Under the existing site/docs sentence, add:

```
Add `?fx` to any page to preview the backdrop full screen with the page chrome
hidden; `?fx=false` keeps the normal page.
```

## Constraints

- Do NOT change the scene: no geometry, alpha, timing, palette, or scale edits.
  This is flag plumbing only. `scene.root.alpha` stays as it is unless the scene
  already dims itself for the page (this repo does not).
- Do NOT touch the manual page: `docs/docs.html` has no `#fx` canvas and no
  `fx.js` include, so there is nothing to preview there. Leave it alone.
- No new files, no new dependencies, no build step.
- `docs/fx.js` must stay in the house style shared by the family: same
  `glowLine` / `GLOW_PASSES` helpers, same `bootTicker` / `reduceMotion` branch.
- Keep `prefers-reduced-motion` working: a still frame at `t = 8`, re-rendered on
  resize, in both normal and preview mode.
- Do not add a variant switcher, HUD, keyboard shortcuts, or tuning controls.
  The preview is the animation and nothing else.

## Verify

`docs/` is already served at http://localhost:8000 — never start a server.

1. `/index.html` — page renders normally, backdrop still visible behind panels.
2. `/index.html?fx` — confirm `document.body.className` contains `fx-only` and
   `getComputedStyle(document.querySelector('main')).display === 'none'`.
3. Two screenshots ~2s apart in `?fx` — the frames must differ (scene is live).
4. Confirm canvas size attributes match `innerWidth`/`innerHeight`.
5. `/index.html?fx=false` — normal page, flag ignored.
6. A narrow window (~420px wide) in `?fx` — still fills, nothing clipped.
7. `node --check docs/fx.js` parses clean.

## Commit

Two commits, in the house style (imperative, lower-case subject, <=50 chars,
bullets in the body):

- `add fx-only preview via url param` — `docs/fx.js`, `docs/style.css`, `README.md`

Do not push. Do not force anything.

## Known wart

`fx.js` is a module, so `fx-only` lands after first paint and the page content
can blink before hiding. fN0B ships it this way. If the flash is unacceptable,
the fix is a small inline `<script>` in `<head>` that reads the param and sets
the class before paint — but that duplicates the parse, so it is left out above
unless asked for.
