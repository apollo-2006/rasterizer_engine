// Page controls for the SDL2 build. The engine owns its loop; these call the
// switches it exports and read back its per-frame timing.
(function () {
  const $ = (id) => document.getElementById(id);
  let samples = [];

  // SDL_CreateWindow sets document.title to the window title; keep the page's own.
  const title = document.title;
  new MutationObserver(() => { if (document.title !== title) document.title = title; })
    .observe(document.querySelector('title'), { childList: true });

  Module.onRuntimeInitialized = function () {
    $('cull').onchange = (e) => Module._set_culling(e.target.checked ? 1 : 0);
    $('depth').onchange = (e) => Module._set_depth_view(e.target.checked ? 1 : 0);
    $('pause').onchange = (e) => Module._set_paused(e.target.checked ? 1 : 0);

    // The keyboard shortcuts flip the engine's state directly; keep the boxes in step.
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT') return;
      const box = { KeyC: 'cull', KeyZ: 'depth', Space: 'pause' }[e.code];
      if (box) { $(box).checked = !$(box).checked; if (e.code === 'Space') e.preventDefault(); }
    }, true);

    setInterval(() => {
      samples.push(Module._get_frame_ms());
      if (samples.length > 20) samples.shift();
      const mean = samples.reduce((a, b) => a + b, 0) / samples.length;
      $('sMs').textContent = mean.toFixed(2) + ' ms';
      $('sTris').textContent = Module._get_triangles_drawn();
    }, 100);
  };
})();
