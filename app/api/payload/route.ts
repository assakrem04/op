import { NextRequest, NextResponse } from 'next/server';
import { validateKey } from '@/lib/db';
import fs from 'fs';
import path from 'path';

const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type, Authorization',
};

// GET /api/payload?f=IntelService&key=KEY-...&hwid=HWID-...
// Serves payload files directly - fixes Vercel TypeScript compilation
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
    return NextResponse.json({ success: false, message: 'Invalid payload file' }, { status: 400, headers: corsHeaders });
  }

  try {
    const filePath = path.join(process.cwd(), 'payloads', safe);
    if (!fs.existsSync(filePath)) {
      return NextResponse.json({ success: false, message: 'Payload not found on server' }, { status: 404, headers: corsHeaders });
    }
    const data = fs.readFileSync(filePath);
    // FIX: Use data as BodyInit via type assertion to fix Vercel compilation error
    return new NextResponse(data, {
      status: 200,
      headers: {
        ...corsHeaders,
        'Content-Type': 'application/octet-stream',
        'Content-Disposition': `attachment; filename="${safe}"`,
        'Content-Length': data.length.toString(),
        'Cache-Control': 'no-store',
      },
    });
  } catch (e) {
    return NextResponse.json({ success: false, message: 'Server error reading payload' }, { status: 500, headers: corsHeaders });
  }
}