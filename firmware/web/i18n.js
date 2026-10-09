// Interface text lives in lang/<code>.json. To add a language, add its file
// and its code to SUPPORTED.
const I18n = (() => {
  const SUPPORTED = ['en'];
  const FALLBACK = 'en';
  let strings = {};

  function pick() {
    const wanted = navigator.languages || [navigator.language || FALLBACK];
    for (const tag of wanted) {
      const code = tag.slice(0, 2).toLowerCase();
      if (SUPPORTED.includes(code)) return code;
    }
    return FALLBACK;
  }

  async function fetchLang(code) {
    const response = await fetch(`lang/${code}.json`);
    return response.ok ? response.json() : {};
  }

  function t(key, vars = {}) {
    let text = strings[key] ?? key;
    for (const [name, value] of Object.entries(vars)) text = text.replaceAll(`{${name}}`, value);
    return text;
  }

  function apply(root = document) {
    root.querySelectorAll('[data-i18n]').forEach(el => { el.textContent = t(el.dataset.i18n); });
    root.querySelectorAll('[data-i18n-aria]').forEach(el => el.setAttribute('aria-label', t(el.dataset.i18nAria)));
  }

  async function load() {
    const code = pick();
    strings = await fetchLang(FALLBACK);
    if (code !== FALLBACK) strings = { ...strings, ...(await fetchLang(code)) };
    document.documentElement.lang = code;
    apply();
  }

  return { load, t, apply };
})();
