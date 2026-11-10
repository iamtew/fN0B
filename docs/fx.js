// PixiJS backdrop: a slow knob turn, full-viewport, no CDN.
const C = { mint: 0x35e0a6, amber: 0xf2c14e, grey: 0x7c918b };
const reduceMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;

const pointer = { x: 0.5, y: 0.5 };
addEventListener('pointermove', (e) => {
  pointer.x = e.clientX / innerWidth;
  pointer.y = e.clientY / innerHeight;
}, { passive: true });

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
  const scene = dialScene(PIXI, glow);
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
    const ease = Math.min(1, dt * 2.5);
    scene.root.x += ((0.5 - pointer.x) * 40 - scene.root.x) * ease;
    scene.root.y += ((0.5 - pointer.y) * 24 - scene.root.y) * ease;
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

// A knob that sweeps its arc once every ~14 s, with a glow trail and a wave below.
function dialScene(PIXI, glow) {
  const root = new PIXI.Container();
  const arc = new PIXI.Graphics();
  const wave = new PIXI.Graphics();
  arc.blendMode = 'add';
  wave.blendMode = 'add';
  const core = glowSprite(PIXI, glow, C.mint);
  root.addChild(arc, wave, core);

  const START = -Math.PI * 0.75;
  const SWEEP = Math.PI * 1.5;

  return {
    root,
    update(t, w, h) {
      const cx = w * 0.5, cy = h * 0.42;
      const r = Math.min(w, h) * 0.16;

      // One full turn of the knob per cycle, with a soft ease at the ends.
      const turn = (t / 14) % 1;
      const eased = turn * turn * (3 - 2 * turn);
      const angle = START + eased * SWEEP;

      arc.clear();
      // Track.
      arc.arc(cx, cy, r, START, START + SWEEP);
      arc.stroke({ width: 1, color: C.grey, alpha: 0.35 });
      // Filled portion, glowing.
      for (const [width, alpha] of GLOW_PASSES) {
        arc.arc(cx, cy, r, START, angle);
        arc.stroke({ width, color: C.mint, alpha: alpha * 0.9, cap: 'round' });
      }

      // The pointer.
      const px = cx + Math.cos(angle) * r, py = cy + Math.sin(angle) * r;
      glowLine(arc, [cx + Math.cos(angle) * r * 0.45, cy + Math.sin(angle) * r * 0.45, px, py], C.amber, 0.6);

      const pulse = 0.5 + 0.5 * Math.sin(t * 1.1);
      core.x = cx;
      core.y = cy;
      core.scale.set(3 + 2.5 * pulse);
      core.alpha = 0.25 + 0.3 * pulse;

      // Wave below, breathing with the pulse.
      wave.clear();
      const pts = [], wy = h * 0.86, amp = h * 0.05 * (0.6 + 0.4 * pulse);
      for (let x = 0; x <= w; x += 3) {
        const u = x / w;
        pts.push(x, wy + amp * Math.sin(u * 8 + t * 1.6) * Math.exp(-u * 0.6));
      }
      glowLine(wave, pts, C.mint, 0.7);
    },
  };
}
