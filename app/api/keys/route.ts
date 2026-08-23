import { NextRequest, NextResponse } from 'next/server';
import { getAllKeys, createKey, resetHWID, toggleKeyStatus, deleteKey } from '@/lib/db';

export async function GET() {
  const keys = getAllKeys();
  return NextResponse.json({ success: true, keys });
}

export async function POST(req: NextRequest) {
  try {
    const body = await req.json();
    const { action, durationDays, key } = body;

    if (action === 'create') {
      const newKey = createKey(Number(durationDays) || 30);
      return NextResponse.json({ success: true, key: newKey });
    }

    if (action === 'reset_hwid') {
      const ok = resetHWID(key);
      return NextResponse.json({ success: ok, message: ok ? 'HWID reset successfully' : 'Key not found' });
    }

    if (action === 'toggle_status') {
      const ok = toggleKeyStatus(key);
      return NextResponse.json({ success: ok, message: ok ? 'Status updated' : 'Key not found' });
    }

    if (action === 'delete') {
      const ok = deleteKey(key);
      return NextResponse.json({ success: ok, message: ok ? 'Key deleted' : 'Key not found' });
    }

    return NextResponse.json({ success: false, message: 'Invalid action' }, { status: 400 });
  } catch (error) {
    return NextResponse.json({ success: false, message: 'Server error' }, { status: 500 });
  }
}
