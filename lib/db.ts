import fs from 'fs';
import path from 'path';

export interface LicenseKey {
  key: string;
  createdAt: string;
  durationDays: number; // 0 = Lifetime, 1, 7, 30, 365
  hwid: string | null;
  activatedAt: string | null;
  expiresAt: string | null;
  isActive: boolean;
}

// In Vercel serverless environment, write to /tmp or fallback to memory
const getFilePath = () => {
  if (process.env.VERCEL) {
    return path.join('/tmp', 'licenses.json');
  }
  return path.join(process.cwd(), 'licenses.json');
};

// Default seed keys if DB is empty - ADD ALL PERSISTENT KEYS HERE (Vercel /tmp is ephemeral!)
const defaultKeys: LicenseKey[] = [
  {
    key: "KEY-PRO-OP04-8888-9999",
    createdAt: new Date().toISOString(),
    durationDays: 30,
    hwid: null,
    activatedAt: null,
    expiresAt: null,
    isActive: true
  },
  {
    key: "KEY-8391-8JMH-3NSL-0MNG",
    createdAt: new Date().toISOString(),
    durationDays: 30,
    hwid: null,
    activatedAt: null,
    expiresAt: null,
    isActive: true
  }
];

function readDB(): LicenseKey[] {
  try {
    const filePath = getFilePath();
    if (!fs.existsSync(filePath)) {
      fs.writeFileSync(filePath, JSON.stringify(defaultKeys, null, 2));
      return [...defaultKeys];
    }
    const data = fs.readFileSync(filePath, 'utf-8');
    const parsed: LicenseKey[] = JSON.parse(data);
    // Merge: ensure all defaultKeys exist (fixes Vercel ephemeral /tmp desync)
    let merged = false;
    for (const dk of defaultKeys) {
      if (!parsed.find(k => k.key.toUpperCase() === dk.key.toUpperCase())) {
        parsed.unshift(dk);
        merged = true;
      }
    }
    if (merged) {
      try { fs.writeFileSync(filePath, JSON.stringify(parsed, null, 2)); } catch {}
    }
    return parsed;
  } catch (error) {
    return [...defaultKeys];
  }
}

function writeDB(keys: LicenseKey[]) {
  try {
    const filePath = getFilePath();
    fs.writeFileSync(filePath, JSON.stringify(keys, null, 2));
  } catch (error) {
    console.error('Failed to write DB:', error);
  }
}

function generateRandomKeyStr(): string {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
  const part = (len: number) => Array.from({ length: len }, () => chars[Math.floor(Math.random() * chars.length)]).join('');
  return `KEY-${part(4)}-${part(4)}-${part(4)}-${part(4)}`;
}

export function getAllKeys(): LicenseKey[] {
  return readDB();
}

export function createKey(durationDays: number = 30): LicenseKey {
  const keys = readDB();
  const newKey: LicenseKey = {
    key: generateRandomKeyStr(),
    createdAt: new Date().toISOString(),
    durationDays,
    hwid: null,
    activatedAt: null,
    expiresAt: null,
    isActive: true
  };
  keys.unshift(newKey);
  writeDB(keys);
  return newKey;
}

export function validateKey(keyInput: string, hwidInput: string): { success: boolean; message: string; expiresAt?: string } {
  const keys = readDB();
  const index = keys.findIndex(k => k.key.trim().toUpperCase() === keyInput.trim().toUpperCase());

  if (index === -1) {
    return { success: false, message: "License key does not exist." };
  }

  const lic = keys[index];

  if (!lic.isActive) {
    return { success: false, message: "License key is disabled by admin." };
  }

  const now = new Date();

  // Bind HWID on first activation
  if (!lic.hwid) {
    lic.hwid = hwidInput;
  }

  // Check HWID Match
  if (lic.hwid !== hwidInput) {
    return { success: false, message: "HWID mismatch! Key is bound to another PC." };
  }

  // First time use activation timestamp & expiration calculation
  if (!lic.activatedAt) {
    lic.activatedAt = now.toISOString();
    if (lic.durationDays > 0) {
      const expDate = new Date(now.getTime() + lic.durationDays * 24 * 60 * 60 * 1000);
      lic.expiresAt = expDate.toISOString();
    } else {
      lic.expiresAt = "LIFETIME";
    }
  }

  // Check Expiration
  if (lic.expiresAt && lic.expiresAt !== "LIFETIME") {
    const expTime = new Date(lic.expiresAt).getTime();
    if (now.getTime() > expTime) {
      return { success: false, message: "License key has expired." };
    }
  }

  // Save updated activation/HWID
  keys[index] = lic;
  writeDB(keys);

  return {
    success: true,
    message: `License valid! Expires: ${lic.expiresAt || 'Lifetime'}`,
    expiresAt: lic.expiresAt || 'Lifetime'
  };
}

export function resetHWID(keyInput: string): boolean {
  const keys = readDB();
  const index = keys.findIndex(k => k.key === keyInput);
  if (index !== -1) {
    keys[index].hwid = null;
    writeDB(keys);
    return true;
  }
  return false;
}

export function toggleKeyStatus(keyInput: string): boolean {
  const keys = readDB();
  const index = keys.findIndex(k => k.key === keyInput);
  if (index !== -1) {
    keys[index].isActive = !keys[index].isActive;
    writeDB(keys);
    return true;
  }
  return false;
}

export function deleteKey(keyInput: string): boolean {
  let keys = readDB();
  const initialLen = keys.length;
  keys = keys.filter(k => k.key !== keyInput);
  if (keys.length !== initialLen) {
    writeDB(keys);
    return true;
  }
  return false;
}
