// Calls the Device API (docs/api.md). Adds the settings-PIN session token and,
// when the robot answers "pin_required", asks for the PIN and retries.
const Api = (() => {
  const TOKEN_KEY = 'cwc.token';

  function token() {
    try {
      return sessionStorage.getItem(TOKEN_KEY) || '';
    } catch {
      return '';
    }
  }

  function setToken(value) {
    try {
      if (value) sessionStorage.setItem(TOKEN_KEY, value);
      else sessionStorage.removeItem(TOKEN_KEY);
    } catch {
      // Private browsing: the PIN will be asked again after a reload.
    }
  }

  // Sends a JSON body with `method`; returns { ok, status, data }.
  async function call(method, path, body) {
    const headers = { 'Content-Type': 'application/json' };
    if (token()) headers.Authorization = `Bearer ${token()}`;
    const response = await fetch(path, { method, headers, body: body === undefined ? undefined : JSON.stringify(body) });
    let data = {};
    try { data = await response.json(); } catch { /* empty body */ }

    if (response.status === 401 && data.error === 'pin_required') {
      setToken('');
      if (await askPin()) return call(method, path, body);
    }
    return { ok: response.ok, status: response.status, data };
  }

  // Shows the PIN dialog until the PIN is right or the person cancels.
  function askPin() {
    return new Promise(resolve => {
      const dialog = document.createElement('dialog');
      dialog.className = 'pin-dialog';
      dialog.innerHTML = `
        <form method="dialog">
          <h2>${I18n.t('pin.title')}</h2>
          <p class="hint">${I18n.t('pin.text')}</p>
          <input type="password" inputmode="numeric" autocomplete="off" pattern="[0-9]*" maxlength="8" required>
          <p class="message" data-kind="error"></p>
          <div class="actions">
            <button class="primary" value="ok">${I18n.t('pin.unlock')}</button>
            <button class="secondary" value="cancel" formnovalidate>${I18n.t('calibration.cancel')}</button>
          </div>
        </form>`;
      document.body.append(dialog);
      const input = dialog.querySelector('input');
      const message = dialog.querySelector('.message');

      dialog.querySelector('form').addEventListener('submit', async event => {
        if (event.submitter?.value === 'cancel') return;
        event.preventDefault();
        const response = await fetch('/api/auth/unlock', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ pin: input.value }),
        });
        const result = await response.json();
        if (response.ok) {
          setToken(result.token);
          dialog.close('ok');
          return;
        }
        message.textContent = result.retryAfter
          ? I18n.t('pin.locked', { seconds: result.retryAfter })
          : I18n.t('pin.wrong');
        input.value = '';
        input.focus();
      });
      dialog.addEventListener('close', () => {
        dialog.remove();
        resolve(dialog.returnValue === 'ok');
      });
      dialog.showModal();
      input.focus();
    });
  }

  return { call, askPin, setToken, token };
})();
