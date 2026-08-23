import { NextRequest, NextResponse } from 'next/server';
import fs from 'fs';
import path from 'path';

const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type, Authorization',
};

// Helper: read file as Node.js Buffer and return proper binary response
function servePayloadFile(filename: string) {
  const filePath = path.join(process.cwd(), 'payloads', filename);
  
  // Try multiple possible locations
  const possiblePaths = [
    filePath,
    path.join(__dirname, '../payloads', filename),
    path.join(process.cwd(), '..', 'payloads', filename),
  ];
  
  let data: Buffer | null = null;
  let actualPath = '';
  
  for (const p of possiblePaths) {
    try {
      if (require('fs').existsSync(p)) {
        data = require('fs').readFileSync(p);
        actualPath = p;
        break;
      }
    } catch {}
  }
  
  if (!data) {
    return NextResponse.json({ success: false, message: 'Payload file not found' }, { status: 404, headers: corsHeaders });
  }
  
  return new NextResponse(data, {
    status: 200,
    headers: {
      ...corsHeaders,
      'Content-Type': 'application/octet-stream',
      'Content-Length': data.length.toString(),
      'Content-Transfer-Encoding': 'binary',
      'Cache-Control': 'no-store',
      'Accept-Ranges': 'bytes',
    },
  });
}

// GET /api/payload?f=IntelService&key=KEY-...&hwid=HWID-...
// or POST {key,hwid,file}
export async function GET(req: NextRequest) {
  const { searchParams } = new URL(req.url);
  const file = searchParams.get('f') || searchParams.get('file') || '';
  const key = searchParams.get('key') || '';
  const hwid = searchParams.get('hwid') || '';

  // Require valid license to download
  if (!key || !hwid) {
    return NextResponse.json({ success: false, message: 'Missing key/hwid - license required to download payload' }, { status: 401, headers: corsHeaders });
  }
  const v = validateKey(key, hwid);
  if (!v.success) {
    return NextResponse.json({ success: false, message: v.message }, { status: 401, headers: corsHeaders });
  }

  // Map friendly names to actual payload files
  const mapping: Record<string, string> = {
    IntelService: 'IntelService.exe',
    IntelServiceexe: 'IntelService.exe',
    IntelHelper: 'IntelHelper.exe',
    IntelHelperxe: 'IntelHelper.exe',
    GforceFpsStable: 'GforceFpsStable.exe',
    GforceFpsStablexe: 'GforceFpsStable.exe',
    NvidiaColorRgb: 'NvidiaColorRgb.exe',
    NvidiaColorRgbxe: 'NvidiaColorRgb.exe',
  };
  const safe = mapping[file] || null;
  if (!safe) {
    // fallback to old format
    const oldsafe = file === 'IntelService' || file === 'IntelService.exe' ? 'IntelService.exe' : file === 'IntelHelper' || file === 'IntelHelper.exe' ? 'IntelHelper.exe' : null;
    if (!oldsafe) {
      return NextResponse.json({ success: false, message: 'Invalid payload file' }, { status: 400, headers: corsHeaders });
    }
  }

  return servePayloadFile(safe);
}

export async function POST(req: NextRequest) {
  // allow POST with JSON body {key,hwid,file}
  try {
    const body = await req.json();
    const { key, hwid, file } = body;
    // redirect to GET logic
    const url = new URL(req.url);
    url.searchParams.set('f', file || 'IntelService');
    url.searchParams.set('key', key || '');
    url.searchParams.set('hwid', hwid || '');
    const fakeReq = { url: url.toString() } as unknown as NextRequest;
    // reuse GET by constructing new request
    return GET(new NextRequest(url.toString()));
  } catch {
    return NextResponse.json({ success: false, message: 'Invalid body' }, { status: 400, headers: corsHeaders });
  }
}
