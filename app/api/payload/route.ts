import { NextRequest, NextResponse } from 'next/server';
import { validateKey } from '@/lib/db';
import fs from 'fs';
import path from 'path';

const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type, Authorization',
};

export async function OPTIONS() {
  return NextResponse.json({}, { headers: corsHeaders });
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

  const safe = file === 'IntelService' || file === 'IntelService.exe' ? 'IntelService.exe' : file === 'IntelHelper' || file === 'IntelHelper.exe' ? 'IntelHelper.exe' : null;
  if (!safe) {
    return NextResponse.json({ success: false, message: 'Invalid payload file' }, { status: 400, headers: corsHeaders });
  }

  try {
    const filePath = path.join(process.cwd(), 'payloads', safe);
    if (!fs.existsSync(filePath)) {
      return NextResponse.json({ success: false, message: 'Payload not found on server' }, { status: 404, headers: corsHeaders });
    }
    const data = fs.readFileSync(filePath);
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
