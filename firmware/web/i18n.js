// Interface text lives in lang/<code>.json. To add a language, add its file
// (tools/check_lang.mjs checks it against en.json) and list it in LANGUAGES.
const I18n = (() => {
  const LANGUAGES = { en: 'English', fr: 'Français' };
  const FALLBACK = 'en';
  const STORAGE_KEY = 'cwc.lang';
  let strings = {};

  // 'auto' or a language code, remembered per browser.
  function preference() {
    try {
      return localStorage.getItem(STORAGE_KEY) || 'auto';
    } catch {
      return 'auto';
    }
  }

  function setPreference(code) {
    try {
      if (code === 'auto') localStorage.removeItem(STORAGE_KEY);
      else localStorage.setItem(STORAGE_KEY, code);
    } catch {
      // Private browsing: the choice lasts until the page is reloaded.
    }
  }

  function pick() {
    const chosen = preference();
    if (chosen in LANGUAGES) return chosen;
    for (const tag of navigator.languages || [navigator.language || FALLBACK]) {
      const code = tag.slice(0, 2).toLowerCase();
      if (code in LANGUAGES) return code;
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

  return { LANGUAGES, load, t, apply, preference, setPreference };
})();
