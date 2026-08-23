import http.server
import socketserver
import json
import sqlite3
import datetime
import secrets
import string

PORT = 5000
DB_FILE = "licenses.db"
ADMIN_SECRET = "admin123"

def init_db():
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS keys (
            key TEXT PRIMARY KEY,
            created_at TEXT,
            duration_days INTEGER,
            hwid TEXT,
            activated_at TEXT,
            expires_at TEXT,
            is_active INTEGER DEFAULT 1
        )
    ''')
    conn.commit()
    conn.close()

def generate_key_str():
    chars = string.ascii_uppercase + string.digits
    parts = [''.join(secrets.choice(chars) for _ in range(4)) for _ in range(4)]
    return "KEY-" + "-".join(parts)

def create_license_key(duration_days=30):
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    key = generate_key_str()
    created_at = datetime.datetime.now(datetime.timezone.utc).isoformat()
    cursor.execute('''
        INSERT INTO keys (key, created_at, duration_days, hwid, activated_at, expires_at, is_active)
        VALUES (?, ?, ?, NULL, NULL, NULL, 1)
    ''', (key, created_at, duration_days))
    conn.commit()
    conn.close()
    return key

def validate_license_key(key, hwid):
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    cursor.execute('SELECT key, duration_days, hwid, activated_at, expires_at, is_active FROM keys WHERE key = ?', (key,))
    row = cursor.fetchone()

    if not row:
        conn.close()
        return False, "License key does not exist."

    key_str, duration_days, stored_hwid, activated_at, expires_at, is_active = row

    if is_active != 1:
        conn.close()
        return False, "License key has been disabled."

    now = datetime.datetime.now(datetime.timezone.utc)

    # BIND HWID IF FIRST TIME
    if not stored_hwid:
        stored_hwid = hwid
        cursor.execute('UPDATE keys SET hwid = ? WHERE key = ?', (hwid, key_str))

    # CHECK HWID MATCH
    if stored_hwid != hwid:
        conn.close()
        return False, "HWID mismatch. This key is bound to another PC."

    # SET ACTIVATION AND EXPIRATION ON FIRST USE
    if not activated_at:
        activated_at = now.isoformat()
        if duration_days > 0:
            exp_time = now + datetime.timedelta(days=duration_days)
            expires_at = exp_time.isoformat()
        else:
            expires_at = "LIFETIME"
        cursor.execute('UPDATE keys SET activated_at = ?, expires_at = ? WHERE key = ?', (activated_at, expires_at, key_str))
        conn.commit()

    # CHECK EXPIRATION
    if expires_at != "LIFETIME":
        exp_datetime = datetime.datetime.fromisoformat(expires_at)
        if now > exp_datetime:
            conn.close()
            return False, "License key has expired."

    conn.commit()
    conn.close()
    return True, f"Success! License valid. Expires: {expires_at}"

class LicenseAPIHandler(http.server.BaseHTTPRequestHandler):
    def _send_json(self, status, data):
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode('utf-8'))

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length)
        
        try:
            req = json.loads(body.decode('utf-8')) if body else {}
        except Exception:
            return self._send_json(400, {"success": False, "message": "Invalid JSON body"})

        if self.path == '/api/validate':
            key = req.get("key", "").strip()
            hwid = req.get("hwid", "").strip()
            if not key or not hwid:
                return self._send_json(400, {"success": False, "message": "Missing key or hwid"})

            valid, message = validate_license_key(key, hwid)
            if valid:
                return self._send_json(200, {"success": True, "message": message})
            else:
                return self._send_json(401, {"success": False, "message": message})

        elif self.path == '/api/create_key':
            secret = req.get("secret", "")
            if secret != ADMIN_SECRET:
                return self._send_json(403, {"success": False, "message": "Unauthorized admin secret"})

            duration_days = int(req.get("duration_days", 30))
            new_key = create_license_key(duration_days)
            return self._send_json(200, {"success": True, "key": new_key, "duration_days": duration_days})

        else:
            return self._send_json(404, {"success": False, "message": "Endpoint not found"})

if __name__ == "__main__":
    init_db()
    print(f"[+] License API Server listening on http://localhost:{PORT}")
    with socketserver.TCPServer(("", PORT), LicenseAPIHandler) as httpd:
        httpd.serve_forever()
