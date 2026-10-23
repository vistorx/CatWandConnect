// The Robot clock (docs/adr/0013): every page load sends the browser's time,
// its UTC offset now and its time zone name. The robot has no other clock
// besides Home Assistant's Native integration, and never asks the internet.
(() => {
  const now = new Date();
  let zone = '';
  try {
    zone = Intl.DateTimeFormat().resolvedOptions().timeZone || '';
  } catch {
    // Old browser: the offset is enough for now.
  }
  fetch('/api/time', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ epochMs: now.getTime(), offset: -now.getTimezoneOffset(), zone }),
  }).catch(() => {});  // older firmware or no connection: nothing to do
})();
