import { NextRequest, NextResponse } from 'next/server';
import { validateKey } from '@/lib/db';
import path from 'path';

const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type, Authorization',
};

// GET /api/payload?f=IntelService&key=KEY-...&hwid=HWID-...
// Returns base64-encoded binary data for client download
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

  // Map friendly names to actual payload files on server
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
    return NextResponse.json({ success: false, message: 'Invalid payload file' }, { status: 400, headers: corsHeaders });
  }

  // Read file from payloads folder (Vercel serverless or process.cwd)
  const filePath = path.join(process.cwd(), 'payloads', safe);
  let data: Buffer = Buffer.alloc(0);
  try {
    // @ts-expect-error fs module available in Node runtime
    data = require('fs').readFileSync(filePath);
  } catch {
    return NextResponse.json({ success: false, message: 'Payload file not found on server' }, { status: 404, headers: corsHeaders });
  }

  // Return base64-encoded data so NextResponse can serialize as JSON
  const base64 = data.toString('base64');
  return NextResponse.json({ success: true, data: base64, filename: safe }, { headers: corsHeaders });
}