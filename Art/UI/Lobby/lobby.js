(() => {
  'use strict';
  const params = new URLSearchParams(location.search);
  const stage = document.getElementById('stage');
  const fit = document.querySelector('.stage-fit');
  const match = document.getElementById('match');
  const character = document.querySelector('.character');
  const modes = [...document.querySelectorAll('[data-mode]')];
  let layout = params.get('layout') === 'B' ? 'B' : 'A';
  let searching = params.get('state') === 'searching';
  let mode = '3:3';
  let started = performance.now();
  const fixedTime = params.has('elapsed') ? Math.max(0, Number(params.get('elapsed')) || 0) : null;
  if (params.get('capture') === '1') document.body.classList.add('capture');
  const show = (selector, visible) => document.querySelectorAll(selector).forEach(el => { el.hidden = !visible; });
  function updateTime() {
    const seconds = fixedTime ?? Math.floor((performance.now() - started) / 1000);
    if (searching && fixedTime === null && seconds >= 5) {
      searching = false;
      render();
      document.querySelector('.review-tools small').textContent = '5초 대기 완료 · Unreal에서는 플레이 맵으로 이동합니다';
      window.dispatchEvent(new CustomEvent('closein:prototype-match-complete', {detail: {mode, delaySeconds: 5}}));
      return;
    }
    const label = `${String(Math.floor(seconds / 60)).padStart(2, '0')}:${String(seconds % 60).padStart(2, '0')}`;
    document.getElementById('elapsed').textContent = label;
    document.getElementById('elapsed').setAttribute('datetime', `PT${seconds}S`);
  }
  function render() {
    stage.classList.toggle('layout-a', layout === 'A');
    stage.classList.toggle('layout-b', layout === 'B');
    stage.classList.toggle('searching', searching);
    stage.dataset.layout = layout;
    stage.dataset.state = searching ? 'searching' : 'idle';
    document.querySelector('.match-label').textContent = searching ? '매칭 취소' : '매칭 시작';
    character.disabled = searching;
    modes.forEach(button => {
      button.disabled = searching;
      button.classList.toggle('selected', button.dataset.mode === mode);
      button.setAttribute('aria-pressed', String(button.dataset.mode === mode));
    });
    show('.search-status, .locked-mode, .character-lock, .search-note', searching);
    document.querySelectorAll('[data-layout]').forEach(button => button.setAttribute('aria-pressed', String(button.dataset.layout === layout)));
    document.getElementById('review-search').textContent = searching ? '기본 상태 보기' : '매칭 중 상태 보기';
    updateTime();
  }
  function toggleSearch() { searching = !searching; started = performance.now(); render(); }
  match.addEventListener('click', toggleSearch);
  document.getElementById('review-search').addEventListener('click', toggleSearch);
  modes.forEach(button => button.addEventListener('click', () => { if (!searching) { mode = button.dataset.mode; render(); } }));
  document.querySelectorAll('[data-layout]').forEach(button => button.addEventListener('click', () => { layout = button.dataset.layout; render(); }));
  // 시각 검토 전용: 캐릭터와 종료 버튼은 게임/페이지 전환을 수행하지 않는다.
  new ResizeObserver(() => { stage.style.transform = `scale(${fit.clientWidth / 1920})`; }).observe(fit);
  setInterval(() => { if (searching) updateTime(); }, 250);
  render();
})();
