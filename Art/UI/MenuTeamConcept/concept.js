(() => {
  'use strict';
  const params = new URLSearchParams(location.search);
  const scene = document.getElementById('scene');
  const roster = document.getElementById('roster');
  const queue = document.getElementById('queue');
  const back = document.getElementById('back');
  const dialogLayer = document.getElementById('dialog-layer');
  const input = document.getElementById('invite-nickname');
  const inviteSubmit = document.getElementById('invite-submit');
  const reviewNote = document.getElementById('review-note');
  const completeLayer = document.getElementById('match-complete-layer');
  const freezeCompletion = params.get('view') === 'complete' && params.get('live') !== '1';
  const isLeader = params.get('role') !== 'member';
  const frozenElapsed = params.has('elapsed') ? Math.max(0, Number(params.get('elapsed')) || 0) : null;
  let screen = 'main';
  let partial = false;
  let searching = false;
  let inviteOpen = false;
  let completed = false;
  let nextReady = false;
  let completedAt = 0;
  let startedAt = performance.now();
  if (params.get('capture') === '1') document.body.classList.add('capture');

  function createImage(name, className = '') {
    const img = document.createElement('img');
    img.src = `Components/${name}.svg`;
    img.alt = '';
    img.className = className;
    return img;
  }
  function renderRoster() {
    roster.replaceChildren();
    for (let index = 0; index < 3; index++) {
      const occupied = !partial || index === 0;
      const card = document.createElement('article');
      card.className = `player-card${index === 0 ? ' leader' : ''}${occupied ? '' : ' empty'}`;
      card.dataset.slot = String(index + 1);
      card.dataset.occupied = String(occupied);
      card.setAttribute('aria-label', `${index + 1}번 ${occupied ? '참가자' : '빈'} 슬롯`);
      card.setAttribute('aria-readonly', String(searching || completed));
      if (occupied) {
        if (index === 0) {
          const badge = document.createElement('span');
          badge.className = 'leader-badge';
          badge.append(createImage('leader-mark'), document.createTextNode('팀장'));
          card.append(badge);
        }
        const info = document.createElement('div');
        info.className = 'player-info';
        const name = document.createElement('span');
        name.className = 'player-name';
        name.textContent = index === (isLeader ? 0 : 1) ? '닉네임' : `닉네임 ${String(index + 1).padStart(2, '0')}`;
        info.append(createImage('profile-neutral', 'profile-symbol'), name);
        card.append(info);
      } else {
        const button = document.createElement('button');
        button.className = 'invite-slot';
        button.type = 'button';
        button.disabled = searching || completed;
        button.setAttribute('aria-label', `${index + 1}번 슬롯에 초대`);
        const label = document.createElement('span');
        label.textContent = '초대';
        button.append(createImage('invite-plus'), label);
        button.addEventListener('click', openInvite);
        card.append(button);
      }
      roster.append(card);
    }
  }
  function updateElapsed() {
    const seconds = frozenElapsed ?? Math.floor((performance.now() - startedAt) / 1000);
    document.getElementById('elapsed').textContent = `${String(Math.floor(seconds / 60)).padStart(2, '0')}:${String(seconds % 60).padStart(2, '0')}`;
    document.getElementById('elapsed').setAttribute('datetime', `PT${seconds}S`);
  }
  function render() {
    scene.classList.toggle('team-view', screen === 'team');
    scene.classList.toggle('searching', searching);
    scene.setAttribute('aria-label', screen === 'main' ? 'Close IN 메인 메뉴' : 'Close IN 팀 로비');
    scene.dataset.screen = screen;
    scene.dataset.searching = String(searching);
    scene.dataset.rosterLocked = String(searching || completed);
    scene.dataset.completed = String(completed);
    scene.dataset.nextReady = String(nextReady);
    document.getElementById('main-screen').hidden = screen !== 'main';
    document.getElementById('team-screen').hidden = screen !== 'team';
    // 1인 팀도 매칭을 시작할 수 있다. 빈 슬롯의 자동 충원 규칙은 별개이다.
    document.getElementById('queue-controls').hidden = completed;
    document.getElementById('search-status').hidden = !searching;
    document.getElementById('search-lock-note').hidden = !searching;
    document.getElementById('member-note').hidden = isLeader || searching;
    queue.disabled = !isLeader || completed;
    queue.querySelector('.queue-label').textContent = searching ? '매칭 취소' : '매칭 시작';
    back.disabled = searching || completed;
    dialogLayer.hidden = !inviteOpen;
    completeLayer.hidden = !completed;
    document.getElementById('main-screen').inert = inviteOpen || completed;
    document.getElementById('team-screen').inert = inviteOpen || completed;
    document.querySelector('.account').inert = inviteOpen || completed;
    renderRoster();
    updateElapsed();
    const state = screen === 'main' ? 'main' : completed ? 'complete' : inviteOpen ? 'invite' : searching ? 'searching' : 'team';
    document.querySelectorAll('[data-preview]').forEach(button => button.setAttribute('aria-pressed', String(button.dataset.preview === state)));
    reviewNote.textContent = completed ? '매칭 완료 표시 3초 후 다음 화면 연결 지점 · 실제 게임 이동은 Unreal 클라이언트에서 수행' : '시각 제안 · 1인 팀도 매칭 시작 가능 · 빈 슬롯 자동 충원은 미정';
  }
  function showPreview(state) {
    screen = state === 'main' ? 'main' : 'team';
    partial = state !== 'full';
    searching = state === 'searching';
    inviteOpen = state === 'invite';
    completed = state === 'complete';
    nextReady = false;
    completedAt = performance.now();
    startedAt = performance.now();
    render();
    if (inviteOpen) input.focus();
  }
  function openInvite() {
    if (searching || completed) return;
    inviteOpen = true;
    input.value = '';
    inviteSubmit.disabled = true;
    render();
    input.focus();
  }
  function closeInvite() {
    inviteOpen = false;
    render();
    roster.querySelector('button')?.focus();
  }
  document.getElementById('game-start').addEventListener('click', () => showPreview('team'));
  back.addEventListener('click', () => { if (!searching) showPreview('main'); });
  queue.addEventListener('click', () => {
    if (!isLeader || completed) return;
    searching = !searching;
    startedAt = performance.now();
    inviteOpen = false;
    render();
  });
  document.getElementById('invite-cancel').addEventListener('click', closeInvite);
  input.addEventListener('input', () => { inviteSubmit.disabled = !input.value.trim(); });
  document.getElementById('invite-form').addEventListener('submit', event => {
    event.preventDefault();
    if (!input.value.trim() || searching) return;
    closeInvite();
    reviewNote.textContent = '초대 요청 화면 확인용 · 요청을 전송하거나 팀원을 추가하지 않았습니다';
  });
  dialogLayer.addEventListener('keydown', event => {
    if (event.key === 'Escape') { event.preventDefault(); closeInvite(); }
    if (event.key !== 'Tab') return;
    const focusable = [...dialogLayer.querySelectorAll('input,button')].filter(el => !el.disabled);
    const first = focusable[0], last = focusable[focusable.length - 1];
    if (event.shiftKey && document.activeElement === first) { event.preventDefault(); last.focus(); }
    else if (!event.shiftKey && document.activeElement === last) { event.preventDefault(); first.focus(); }
  });
  document.querySelectorAll('[data-preview]').forEach(button => button.addEventListener('click', () => showPreview(button.dataset.preview)));
  const fit = document.querySelector('.stage-fit');
  new ResizeObserver(() => { scene.style.transform = `scale(${fit.clientWidth / 1920})`; }).observe(fit);
  setInterval(() => {
    const now = performance.now();
    if (searching) updateElapsed();
    if (searching && frozenElapsed === null && now - startedAt >= 5000) {
      searching = false;
      completed = true;
      completedAt = now;
      render();
    }
    if (completed && !freezeCompletion && !nextReady && now - completedAt >= 3000) {
      nextReady = true;
      scene.dataset.nextReady = 'true';
      window.dispatchEvent(new CustomEvent('closein:match-complete-next', {detail:{holdSeconds:3}}));
      reviewNote.textContent = '3초 표시 완료 · 다음 화면 연결 지점 (게임 이동은 Unreal에서 처리)';
    }
  }, 100);
  showPreview(params.get('view') || 'main');
})();
