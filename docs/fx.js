// PixiJS backdrop: a churn maw eating toward the viewer, full-viewport, no CDN.
const C = { mint: 0x35e0a6, amber: 0xf2c14e, light: 0xe8f5f0, grey: 0x7c918b, ink: 0x08100f,
  silver: 0x9fb5ae };  // sage lifted toward white, for the rim and the deep strands
const SQUASH = 0.38;  // lens squash: the maw is wider than it is tall
const SPIN = 0.75;    // rigid turn, rad/s
const SPIRAL = 2.4;   // radians each arm wraps from rim to throat
const SHEAR = 0.9;    // differential knead, radians; bounded so the arms stay readable
const GULP = 5.5;     // seconds per swallow
const ARMS = 9;
const SAMPLES = 30;
const TAU = Math.PI * 2;
const reduceMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
const rand = (lo, hi) => lo + Math.random() * (hi - lo);
const clamp = (v, lo, hi) => Math.min(hi, Math.max(lo, v));

// ?fx / ?fx=true keeps only the canvas. ?fx=false / 0 / off stays a normal page.
const fxRaw = new URLSearchParams(location.search).get('fx');
const fxOnly = fxRaw !== null && !/^(0|false|off)$/i.test(fxRaw);
if (fxOnly) document.body.classList.add('fx-only');

const pointer = { x: 0.5, y: 0.5 };
if (!fxOnly) {
  addEventListener('pointermove', (e) => {
    pointer.x = e.clientX / innerWidth;
    pointer.y = e.clientY / innerHeight;
  }, { passive: true });
}

start().catch((err) => {
  console.info('fx: animated backdrop disabled:', err?.message || err);
  document.getElementById('fx')?.remove();
});

async function start() {
  const PIXI = await import('./vendor/pixi.min.mjs');
  const app = new PIXI.Application();
  await app.init({
    canvas: document.getElementById('fx'),
    resizeTo: window,
    backgroundAlpha: 0,
    antialias: true,
    autoDensity: true,
    resolution: Math.min(devicePixelRatio || 1, 2),
    preference: 'webgl',
  });

  const glow = glowTexture(PIXI);
  const scene = mawScene(PIXI, glow);
  app.stage.addChild(scene.root);
  bootTicker(app, scene);
}

function bootTicker(app, scene) {
  if (reduceMotion) {
    app.ticker.stop();
    const still = () => {
      const { width: w, height: h } = app.screen;
      scene.update(8, w, h, 0);
      app.render();
    };
    addEventListener('resize', () => requestAnimationFrame(still));
    still();
    return;
  }

  let t = 0;
  app.ticker.add((ticker) => {
    const dt = Math.min(ticker.deltaMS / 1000, 0.05);
    t += dt;
    const { width: w, height: h } = app.screen;
    // Preview mode is full-bleed, so there is no page to parallax against.
    if (!fxOnly) {
      const ease = Math.min(1, dt * 2.5);
      scene.root.x += ((0.5 - pointer.x) * 40 - scene.root.x) * ease;
      scene.root.y += ((0.5 - pointer.y) * 24 - scene.root.y) * ease;
    }
    scene.update(t, w, h, dt);
  });

  document.addEventListener('visibilitychange', () => {
    if (document.hidden) app.ticker.stop(); else app.ticker.start();
  });
}

function glowTexture(PIXI, size = 64) {
  const c = document.createElement('canvas');
  c.width = c.height = size;
  const g = c.getContext('2d');
  const r = size / 2;
  const grd = g.createRadialGradient(r, r, 0, r, r, r);
  grd.addColorStop(0, 'rgba(255,255,255,1)');
  grd.addColorStop(0.25, 'rgba(255,255,255,0.5)');
  grd.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = grd;
  g.fillRect(0, 0, size, size);
  return PIXI.Texture.from(c);
}

function glowSprite(PIXI, tex, tint) {
  const s = new PIXI.Sprite(tex);
  s.anchor.set(0.5);
  s.tint = tint;
  s.blendMode = 'add';
  return s;
}

const GLOW_PASSES = [[22, 0.05], [10, 0.14], [2.5, 1]];
function glowLine(g, pts, color, scale = 1, passes = GLOW_PASSES) {
  for (const [width, alpha] of passes) {
    g.moveTo(pts[0], pts[1]);
    for (let i = 2; i < pts.length; i += 2) g.lineTo(pts[i], pts[i + 1]);
    g.stroke({ width: width * scale, color, alpha, cap: 'round', join: 'round' });
  }
}

// Dim body under a bright head that runs inward: the eating read.
const BODY_PASSES = [[18, 0.05], [8, 0.13], [2.2, 0.5]];
const HEAD_PASSES = [[18, 0.12], [7, 0.4], [2.2, 1]];
const ARM_COLORS = [C.mint, C.mint, C.amber, C.grey, C.mint, C.amber, C.mint, C.grey, C.mint];

// A wide horizontal funnel. Strands shear against each other, debris spirals in,
// and the ink slit swallows both on every gulp.
function mawScene(PIXI, glow) {
  const root = new PIXI.Container();
  const arms = new PIXI.Graphics();
  const pupil = new PIXI.Graphics();
  const core = new PIXI.Graphics();
  const shock = new PIXI.Graphics();
  arms.blendMode = 'add';
  core.blendMode = 'add';
  shock.blendMode = 'add';

  const debris = Array.from({ length: 120 }, (_, i) => {
    const s = glowSprite(PIXI, glow, Math.random() < 0.7 ? C.mint : C.amber);
    s.u = rand(0.1, 1.16);            // 0 throat .. 1 rim, so a resize cannot strand it
    s.th = rand(0, TAU);
    s.su = rand(0.04, 0.15);          // radial speed, disc widths per second
    s.size = i % 11 === 0 ? rand(0.45, 0.8) : rand(0.09, 0.26);
    s.alpha = 0;
    return s;
  });

  const bloom = glowSprite(PIXI, glow, C.mint);
  bloom.alpha = 0;

  // Order matters: the slit has to swallow the strands and the debris.
  root.addChild(arms, ...debris, pupil, bloom, core, shock);
  root.alpha = fxOnly ? 1 : 0.62;

  return {
    root,
    update(t, w, h, dt) {
      // One swallow per GULP, decaying fast so it reads as a gulp, not a beat.
      const env = Math.exp(-((t % GULP) / GULP) * 4.6);
      const spin = t * SPIN;
      // The gulp also throws the whole maw at the viewer, then pulls it back.
      const R = Math.min(w, h) * (fxOnly ? 0.66 : 0.46) * (1 + 0.09 * env);
      const Rin = R * 0.05;
      const cx = w * 0.5;
      const cy = h * (fxOnly ? 0.5 : 0.46);

      // u = 0 at the throat, 1 at the rim. The spiral plus the shearing term is
      // what makes neighbouring strands knead against each other instead of
      // turning like one rigid record. The ripple keeps them organic, not CAD.
      const at = (a, u) => {
        const wob = 1 + 0.05 * Math.sin(u * 9 - t * 1.7 + a * 1.9);
        const r = (Rin + (R - Rin) * u) * wob;
        const th = (a / ARMS) * TAU - SPIRAL * u - spin - SHEAR * Math.sin(t * 1.1 - u * 3.4);
        return [cx + Math.cos(th) * r, cy + Math.sin(th) * r * SQUASH];
      };

      arms.clear();
      for (let a = 0; a < ARMS; a++) {
        const color = ARM_COLORS[a];
        const pts = [];
        for (let i = 0; i <= SAMPLES; i++) pts.push(...at(a, i / SAMPLES));
        // Throat half is a faint thread, rim half carries the body: strands thin
        // as they are eaten. Two polylines, not one, so the taper reads.
        const mid = (SAMPLES >> 1) * 2;
        glowLine(arms, pts.slice(0, mid + 4), color, 0.45, BODY_PASSES);
        glowLine(arms, pts.slice(mid), color, 1.2, BODY_PASSES);
        // Two bright heads per strand, sliding rim -> throat. Each is its own
        // short polyline; a shared point list would chord across the gaps.
        for (let p = 0; p < 2; p++) {
          const u = 1 - ((t * 0.26 + a * 0.19 + p * 0.5) % 1);
          const seg = [
            ...at(a, Math.min(1, u + 0.045)),
            ...at(a, u),
            ...at(a, Math.max(0, u - 0.045)),
          ];
          glowLine(arms, seg, color, 1, HEAD_PASSES);
        }
      }
      // Rim ellipses: depth, and a horizon for the funnel.
      for (const [rr, alpha] of [[R, 0.16], [R * 0.62, 0.09], [R * 1.1, 0.05]]) {
        arms.ellipse(cx, cy, rr, rr * SQUASH).stroke({ width: 1, color: C.silver, alpha });
      }

      // The slit opens when it eats.
      const rx = R * 0.52 * (1 + 0.08 * env);
      const ry = R * 0.15 * (1 + 0.45 * env);
      pupil.clear();
      pupil.ellipse(cx, cy, rx * 1.12, ry * 1.25).fill({ color: C.ink, alpha: 0.55 });
      pupil.ellipse(cx, cy, rx, ry).fill({ color: C.ink, alpha: 0.95 });

      bloom.x = cx;
      bloom.y = cy;
      bloom.scale.set(R / 45, (R / 45) * 0.21);
      bloom.alpha = 0.16 + 0.22 * env;

      // Hot seam: mint halo, white core flickering, amber under-fringe.
      core.clear();
      const seam = [];
      const step = rx / 12;
      for (let x = cx - rx; x <= cx + rx; x += step) {
        seam.push(x, cy + Math.sin(x * 0.02 + t * 3) * ry * 0.12);
      }
      glowLine(core, seam, C.mint, 1.9, [[30, 0.06], [12, 0.14]]);
      glowLine(core, [cx - rx, cy + ry * 0.5, cx + rx, cy + ry * 0.5], C.amber, 0.8, [[8, 0.10], [2, 0.35]]);
      glowLine(core, seam, C.light, 1, [[9, 0.18], [3, 0.7], [1.4, 1]]);

      // One ring per gulp, expanding and dying before the next.
      const k = 1 - (t % GULP) / GULP;
      const rr = R * (0.5 + 0.62 * (1 - k));
      shock.clear();
      shock.ellipse(cx, cy, rr, rr * SQUASH * 0.55).stroke({ width: 1 + 2 * k, color: C.mint, alpha: 0.32 * k * k });
      shock.ellipse(cx, cy, rr * 0.8, rr * SQUASH * 0.44).stroke({ width: 1, color: C.amber, alpha: 0.18 * k * k });

      for (const s of debris) {
        if (dt > 0) {
          s.u -= s.su * dt;
          if (s.u < 0.08) {
            s.u = rand(1.02, 1.18);
            s.th = rand(0, TAU);
            s.su = rand(0.04, 0.15);
          }
          s.th -= (SPIN + 0.9 / (0.3 + s.u)) * dt;
        }
        const r = Rin + (R - Rin) * s.u;
        const ct = Math.cos(s.th), st = Math.sin(s.th);
        s.x = cx + ct * r;
        s.y = cy + st * r * SQUASH;
        // Streak along the actual travel: tangential plus inward.
        const om = SPIN + 0.9 / (0.3 + s.u);
        const vr = (R - Rin) * s.su;
        s.rotation = Math.atan2(-ct * r * SQUASH * om - st * SQUASH * vr, st * r * om - ct * vr);
        const wpx = s.size * R * (0.05 + 0.2 * s.u);
        s.scale.set((wpx * 4) / 64, wpx / 64);
        s.alpha = 0.55 * clamp((1.14 - s.u) / 0.14, 0, 1) * clamp((s.u - 0.06) / 0.18, 0, 1);
      }
    },
  };
}
