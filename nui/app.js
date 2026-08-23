// Next.js API Validation URL
const API_URL = '/api/validate';

// DOM Elements
const form = document.getElementById('licenseForm');
const keyInput = document.getElementById('keyInput');
const btnSubmit = document.getElementById('btnSubmit');
const btnText = document.getElementById('btnText');
const spinner = document.getElementById('spinner');
const statusBox = document.getElementById('statusBox');
const statusDot = document.getElementById('statusDot');
const statusMsg = document.getElementById('statusMsg');
const btnClose = document.getElementById('btnClose');
const btnMin = document.getElementById('btnMin');

// Helper: HWID Generation in JS
function getBrowserHWID() {
  const userAgent = navigator.userAgent;
  const screenRes = `${window.screen.width}x${window.screen.height}`;
  const lang = navigator.language;
  let hash = 0;
  const str = `${userAgent}-${screenRes}-${lang}`;
  for (let i = 0; i < str.length; i++) {
    hash = ((hash << 5) - hash) + str.charCodeAt(i);
    hash |= 0;
  }
  return `HWID-WEB-${Math.abs(hash).toString(16).toUpperCase()}`;
}

// Load saved key on load
document.addEventListener('DOMContentLoaded', () => {
  const savedKey = localStorage.getItem('hollow_license_key');
  if (savedKey) {
    keyInput.value = savedKey;
  }
});

// Update Status Message
function setStatus(msg, type = 'normal') {
  statusMsg.textContent = msg;
  statusBox.className = `status-box ${type}`;
}

// Window Controls
btnClose.addEventListener('click', () => {
  if (window.chrome && window.chrome.webview) {
    window.chrome.webview.postMessage('close');
  } else if (window.cefQuery) {
    window.cefQuery({ request: 'close' });
  } else {
    window.close();
  }
});

btnMin.addEventListener('click', () => {
  if (window.chrome && window.chrome.webview) {
    window.chrome.webview.postMessage('minimize');
  }
});

// Submit Form Handler
form.addEventListener('submit', async (e) => {
  e.preventDefault();
  const key = keyInput.value.trim();

  if (!key) {
    setStatus('Please enter a valid license key.', 'error');
    return;
  }

  // Show Loading Spinner
  btnText.textContent = 'VALIDATING...';
  spinner.classList.remove('hidden');
  btnSubmit.disabled = true;
  setStatus('Connecting to Next.js License API...', 'normal');

  const hwid = getBrowserHWID();

  try {
    const response = await fetch(API_URL, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ key, hwid })
    });

    const data = await response.json();

    if (response.ok && data.success) {
      localStorage.setItem('hollow_license_key', key);
      setStatus(`Success! ${data.message}`, 'success');
      btnText.textContent = 'LAUNCHING...';

      // Send auth signal to C++ host container
      setTimeout(() => {
        if (window.chrome && window.chrome.webview) {
          window.chrome.webview.postMessage('auth_success');
        } else {
          alert('Authenticated successfully! Launching payloads...');
        }
      }, 800);
    } else {
      setStatus(`Error: ${data.message || 'Invalid license key.'}`, 'error');
      btnText.textContent = 'AUTHENTICATE & LAUNCH';
      spinner.classList.add('hidden');
      btnSubmit.disabled = false;
    }
  } catch (err) {
    setStatus('Error: Network error reaching API endpoint.', 'error');
    btnText.textContent = 'AUTHENTICATE & LAUNCH';
    spinner.classList.add('hidden');
    btnSubmit.disabled = false;
  }
});
