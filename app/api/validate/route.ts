import { NextRequest, NextResponse } from 'next/server';
import { validateKey } from '@/lib/db';

const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'POST, OPTIONS, GET',
  'Access-Control-Allow-Headers': 'Content-Type, Authorization',
};

export async function OPTIONS() {
  return NextResponse.json({}, { headers: corsHeaders });
}

export async function POST(req: NextRequest) {
  try {
    const body = await req.json();
    const { key, hwid } = body;

    if (!key || !hwid) {
      return NextResponse.json(
        { success: false, message: 'Missing key or hwid parameter.' },
        { status: 400, headers: corsHeaders }
      );
    }

    const result = validateKey(key, hwid);

    if (result.success) {
      return NextResponse.json({ success: true, message: result.message, expiresAt: result.expiresAt }, { status: 200, headers: corsHeaders });
    } else {
      return NextResponse.json({ success: false, message: result.message }, { status: 401, headers: corsHeaders });
    }
  } catch (error) {
    return NextResponse.json(
      { success: false, message: 'Internal server error processing request.' },
      { status: 500, headers: corsHeaders }
    );
  }
}
