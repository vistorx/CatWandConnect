// Home-screen tip (docs/adr/0007): on a phone's first visit, explains how to
// add the robot's page to the home screen so it opens like an app.
const HomeScreen = (() => {
  const STORAGE_KEY = 'cwc.homescreen';  // 'done' after Got it, else when last shown (ms)
  const AGAIN_AFTER_MS = 24 * 60 * 60 * 1000;  // Later: at most once a day

  // Drawn for the project: no Apple or Google artwork.
  const svg = body => `<svg class="hs-icon" viewBox="0 0 24 24" aria-hidden="true" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">${body}</svg>`;
  const ICONS = {
    share: svg('<path d="M8.5 9H6v12h12V9h-2.5M12 3v12M8 7l4-4 4 4"/>'),
    add: svg('<rect x="4" y="4" width="16" height="16" rx="3.5"/><path d="M12 8.5v7M8.5 12h7"/>'),
    menu: svg('<circle cx="12" cy="5" r="1.3" fill="currentColor"/><circle cx="12" cy="12" r="1.3" fill="currentColor"/><circle cx="12" cy="19" r="1.3" fill="currentColor"/>'),
  };
  // A home screen with the robot's icon among the others.
  const tiles = [8, 23, 38, 53].flatMap(x => [12, 31].map(y => `<rect x="${x}" y="${y}" width="11" height="11" rx="3"/>`)).join('');
  const SCREEN = `<svg class="hs-screen" viewBox="0 0 72 84" aria-hidden="true">
    <rect x="1" y="1" width="70" height="82" rx="10" fill="none" stroke="currentColor" stroke-width="2"/>
    <g fill="currentColor" opacity=".25">${tiles}</g>
    <image href="icon.svg" x="23" y="50" width="26" height="26"/></svg>`;

  const STEPS = {
    ios: [['home.ios.1', ICONS.share], ['home.ios.2', ICONS.add], ['home.ios.3', '']],
    android: [['home.android.1', ICONS.menu], ['home.android.2', ICONS.add], ['home.android.3', '']],
  };

  function platform() {
    const ua = navigator.userAgent;
    // iPadOS reports a Mac; only the touch screen gives it away.
    if (/iPhone|iPad|iPod/.test(ua) || (/Macintosh/.test(ua) && navigator.maxTouchPoints > 1)) return 'ios';
    if (/Android/.test(ua)) return 'android';
    return '';
  }

  const standalone = () => navigator.standalone === true
    || matchMedia('(display-mode: standalone), (display-mode: fullscreen)').matches;

  function stored() {
    try { return localStorage.getItem(STORAGE_KEY); } catch { return null; }
  }

  function store(value) {
    try { localStorage.setItem(STORAGE_KEY, String(value)); } catch { /* private browsing */ }
  }

  const due = () => {
    const value = stored();
    return value !== 'done' && !(Date.now() - Number(value) < AGAIN_AFTER_MS);
  };

  const escape = text => String(text).replace(/[&<>"]/g, c => `&#${c.charCodeAt(0)};`);

  // `host`: the robot's .local name, offered when the page was opened by IP.
  function open(host = '') {
    if (document.querySelector('.hs-dialog')) return;
    let shown = platform() || 'ios';
    const dialog = document.createElement('dialog');
    dialog.className = 'hs-dialog';
    dialog.setAttribute('aria-modal', 'true');
    dialog.setAttribute('aria-labelledby', 'hs-title');
    const byIp = /^\d{1,3}(\.\d{1,3}){3}$/.test(location.hostname) && host;
    const address = byIp ? `http://${host}/` : '';
    dialog.innerHTML = `
      <div class="hs-head">${SCREEN}<h2 id="hs-title" tabindex="-1">${I18n.t('home.title')}</h2></div>
      <p class="hint">${I18n.t('home.intro')}</p>
      <ol class="hs-steps"></ol>
      <p class="hint hs-note"></p>
      ${byIp ? `<p class="note">${I18n.t('home.ip', { address: `<a href="${escape(address)}">${escape(address)}</a>` })}</p>` : ''}
      <button type="button" class="text-button hs-switch"></button>
      <div class="actions">
        <button type="button" class="primary" value="done">${I18n.t('home.done')}</button>
        <button type="button" class="secondary" value="later">${I18n.t('home.later')}</button>
      </div>`;

    const render = () => {
      dialog.querySelector('.hs-steps').innerHTML = STEPS[shown]
        .map(([key, icon]) => `<li><span>${I18n.t(key, { icon })}</span></li>`).join('');
      dialog.querySelector('.hs-note').textContent = shown === 'ios' ? I18n.t('home.ios.other') : '';
      dialog.querySelector('.hs-note').hidden = shown !== 'ios';
      dialog.querySelector('.hs-switch').textContent = I18n.t(shown === 'ios' ? 'home.show.android' : 'home.show.ios');
    };
    render();
    dialog.querySelector('.hs-switch').addEventListener('click', () => {
      shown = shown === 'ios' ? 'android' : 'ios';
      render();
    });

    // Got it: never again. Later, Escape or a tap outside: maybe tomorrow
    // (reopened from Help after Got it, it stays never again).
    const close = value => {
      if (value === 'done' || stored() === 'done') store('done');
      else store(Date.now());
      dialog.close();
    };
    dialog.querySelectorAll('.actions button').forEach(b => b.addEventListener('click', () => close(b.value)));
    dialog.addEventListener('cancel', event => { event.preventDefault(); close('later'); });
    dialog.addEventListener('click', event => { if (event.target === dialog) close('later'); });
    dialog.addEventListener('close', () => dialog.remove());

    document.body.append(dialog);
    dialog.showModal();
    dialog.querySelector('#hs-title').focus();
  }

  // On the main screen: a phone, in the browser, on the home network, not
  // already told. Records the showing, so Later waits a day even if the page
  // is just closed.
  async function maybeShow() {
    if (!platform() || standalone() || !due()) return;
    let wifi;
    try {
      wifi = await (await fetch('/api/wifi/status')).json();
    } catch {
      return;
    }
    if (wifi.hotspot || wifi.mode === 'hotspot') return;
    store(Date.now());
    open(wifi.host || '');
  }

  return { open, maybeShow };
})();
