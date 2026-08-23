'use client';

import React, { useState, useEffect } from 'react';
import { Key, Shield, RefreshCw, Trash2, Plus, Copy, Check, Clock, Laptop, Activity } from 'lucide-react';

interface LicenseKey {
  key: string;
  createdAt: string;
  durationDays: number;
  hwid: string | null;
  activatedAt: string | null;
  expiresAt: string | null;
  isActive: boolean;
}

export default function Dashboard() {
  const [keys, setKeys] = useState<LicenseKey[]>([]);
  const [loading, setLoading] = useState(true);
  const [duration, setDuration] = useState(30);
  const [copiedKey, setCopiedKey] = useState<string | null>(null);
  const [isGenerating, setIsGenerating] = useState(false);

  const fetchKeys = async () => {
    try {
      const res = await fetch('/api/keys');
      const data = await res.json();
      if (data.success) {
        setKeys(data.keys);
      }
    } catch (err) {
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchKeys();
  }, []);

  const handleCreateKey = async () => {
    setIsGenerating(true);
    try {
      const res = await fetch('/api/keys', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'create', durationDays: duration }),
      });
      const data = await res.json();
      if (data.success) {
        await fetchKeys();
      }
    } catch (err) {
      console.error(err);
    } finally {
      setIsGenerating(false);
    }
  };

  const handleAction = async (action: string, keyStr: string) => {
    try {
      const res = await fetch('/api/keys', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action, key: keyStr }),
      });
      const data = await res.json();
      if (data.success) {
        fetchKeys();
      }
    } catch (err) {
      console.error(err);
    }
  };

  const copyToClipboard = (text: string) => {
    navigator.clipboard.writeText(text);
    setCopiedKey(text);
    setTimeout(() => setCopiedKey(null), 2000);
  };

  const totalKeys = keys.length;
  const activeKeys = keys.filter(k => k.isActive).length;
  const boundKeys = keys.filter(k => k.hwid !== null).length;

  return (
    <div className="min-h-screen bg-[#0F111A] text-slate-100 p-6 md:p-10 font-sans">
      <div className="max-w-7xl mx-auto space-y-8">
        
        {/* Header */}
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4 border-b border-[#23283B] pb-6">
          <div>
            <h1 className="text-3xl font-extrabold flex items-center gap-3 text-white tracking-wide">
              <Shield className="w-8 h-8 text-[#5865F2]" />
              HOLLOW LAUNCHER <span className="text-xs bg-[#5865F2]/20 text-[#5865F2] px-2.5 py-1 rounded-full font-bold border border-[#5865F2]/40">SERVERLESS ADMIN</span>
            </h1>
            <p className="text-slate-400 text-sm mt-1">Manage licenses, HWID bindings, and API configurations</p>
          </div>

          <div className="flex items-center gap-3">
            <span className="flex items-center gap-2 bg-emerald-500/10 text-emerald-400 text-xs px-3 py-1.5 rounded-full border border-emerald-500/30">
              <span className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse"></span>
              Live API Online
            </span>
          </div>
        </div>

        {/* Stats Grid */}
        <div className="grid grid-cols-1 sm:grid-cols-3 gap-5">
          <div className="bg-[#161926] p-5 rounded-2xl border border-[#23283B] flex items-center justify-between">
            <div>
              <p className="text-xs text-slate-400 font-semibold uppercase tracking-wider">Total Licenses</p>
              <h3 className="text-3xl font-extrabold text-white mt-1">{totalKeys}</h3>
            </div>
            <div className="p-3 bg-[#5865F2]/10 text-[#5865F2] rounded-xl border border-[#5865F2]/20">
              <Key className="w-6 h-6" />
            </div>
          </div>

          <div className="bg-[#161926] p-5 rounded-2xl border border-[#23283B] flex items-center justify-between">
            <div>
              <p className="text-xs text-slate-400 font-semibold uppercase tracking-wider">Active Keys</p>
              <h3 className="text-3xl font-extrabold text-emerald-400 mt-1">{activeKeys}</h3>
            </div>
            <div className="p-3 bg-emerald-500/10 text-emerald-400 rounded-xl border border-emerald-500/20">
              <Activity className="w-6 h-6" />
            </div>
          </div>

          <div className="bg-[#161926] p-5 rounded-2xl border border-[#23283B] flex items-center justify-between">
            <div>
              <p className="text-xs text-slate-400 font-semibold uppercase tracking-wider">HWID Bound</p>
              <h3 className="text-3xl font-extrabold text-purple-400 mt-1">{boundKeys}</h3>
            </div>
            <div className="p-3 bg-purple-500/10 text-purple-400 rounded-xl border border-purple-500/20">
              <Laptop className="w-6 h-6" />
            </div>
          </div>
        </div>

        {/* Generate Key Control Card */}
        <div className="bg-[#161926] p-6 rounded-2xl border border-[#23283B] space-y-4">
          <h2 className="text-lg font-bold text-white flex items-center gap-2">
            <Plus className="w-5 h-5 text-[#5865F2]" /> Generate New License Key
          </h2>

          <div className="flex flex-wrap items-center gap-4">
            <select
              value={duration}
              onChange={(e) => setDuration(Number(e.target.value))}
              className="bg-[#202436] text-white px-4 py-2.5 rounded-xl border border-[#3C4464] focus:outline-none focus:border-[#5865F2] text-sm"
            >
              <option value={1}>1 Day (Trial)</option>
              <option value={7}>7 Days (Weekly)</option>
              <option value={30}>30 Days (Monthly)</option>
              <option value={365}>365 Days (Yearly)</option>
              <option value={0}>Lifetime (0 Days)</option>
            </select>

            <button
              onClick={handleCreateKey}
              disabled={isGenerating}
              className="bg-[#5865F2] hover:bg-[#4752C4] text-white font-bold px-6 py-2.5 rounded-xl transition duration-150 flex items-center gap-2 text-sm shadow-lg shadow-[#5865F2]/20 disabled:opacity-50"
            >
              <Plus className="w-4 h-4" />
              {isGenerating ? 'Generating...' : 'Create License Key'}
            </button>
          </div>
        </div>

        {/* License Keys Table */}
        <div className="bg-[#161926] rounded-2xl border border-[#23283B] overflow-hidden">
          <div className="p-5 border-b border-[#23283B] flex items-center justify-between">
            <h2 className="text-lg font-bold text-white flex items-center gap-2">
              <Key className="w-5 h-5 text-slate-400" /> Active License Keys ({keys.length})
            </h2>
            <button
              onClick={fetchKeys}
              className="text-xs text-slate-400 hover:text-white flex items-center gap-1 bg-[#202436] px-3 py-1.5 rounded-lg border border-[#3C4464]"
            >
              <RefreshCw className="w-3.5 h-3.5" /> Refresh
            </button>
          </div>

          <div className="overflow-x-auto">
            <table className="w-full text-left text-sm text-slate-300">
              <thead className="bg-[#1D2132] text-slate-400 uppercase text-[11px] font-bold tracking-wider">
                <tr>
                  <th className="px-6 py-4">License Key</th>
                  <th className="px-6 py-4">Duration</th>
                  <th className="px-6 py-4">HWID Binding</th>
                  <th className="px-6 py-4">Expiration</th>
                  <th className="px-6 py-4">Status</th>
                  <th className="px-6 py-4 text-right">Actions</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-[#23283B]">
                {loading ? (
                  <tr>
                    <td colSpan={6} className="text-center py-10 text-slate-400">Loading licenses...</td>
                  </tr>
                ) : keys.length === 0 ? (
                  <tr>
                    <td colSpan={6} className="text-center py-10 text-slate-400">No license keys created yet.</td>
                  </tr>
                ) : (
                  keys.map((k) => (
                    <tr key={k.key} className="hover:bg-[#1C2030] transition duration-150">
                      <td className="px-6 py-4 font-mono font-bold text-white flex items-center gap-2">
                        {k.key}
                        <button
                          onClick={() => copyToClipboard(k.key)}
                          className="text-slate-400 hover:text-[#5865F2] p-1 rounded"
                          title="Copy Key"
                        >
                          {copiedKey === k.key ? <Check className="w-4 h-4 text-emerald-400" /> : <Copy className="w-4 h-4" />}
                        </button>
                      </td>
                      <td className="px-6 py-4 text-xs font-semibold">
                        {k.durationDays === 0 ? (
                          <span className="bg-purple-500/10 text-purple-400 px-2.5 py-1 rounded-md border border-purple-500/30">Lifetime</span>
                        ) : (
                          <span>{k.durationDays} Days</span>
                        )}
                      </td>
                      <td className="px-6 py-4 font-mono text-xs text-slate-400">
                        {k.hwid ? (
                          <span className="text-slate-200 truncate max-w-[140px] inline-block" title={k.hwid}>
                            {k.hwid}
                          </span>
                        ) : (
                          <span className="text-slate-500 italic">Not Bound</span>
                        )}
                      </td>
                      <td className="px-6 py-4 text-xs">
                        {k.expiresAt ? (
                          <span className="flex items-center gap-1 text-slate-300">
                            <Clock className="w-3.5 h-3.5 text-slate-400" />
                            {k.expiresAt === 'LIFETIME' ? 'Never' : new Date(k.expiresAt).toLocaleDateString()}
                          </span>
                        ) : (
                          <span className="text-slate-500 italic">Unused</span>
                        )}
                      </td>
                      <td className="px-6 py-4">
                        {k.isActive ? (
                          <span className="bg-emerald-500/10 text-emerald-400 text-xs px-2.5 py-1 rounded-md border border-emerald-500/30 font-bold">Active</span>
                        ) : (
                          <span className="bg-rose-500/10 text-rose-400 text-xs px-2.5 py-1 rounded-md border border-rose-500/30 font-bold">Disabled</span>
                        )}
                      </td>
                      <td className="px-6 py-4 text-right space-x-2">
                        <button
                          onClick={() => handleAction('reset_hwid', k.key)}
                          className="bg-amber-500/10 text-amber-400 hover:bg-amber-500/20 px-2.5 py-1.5 rounded-lg text-xs font-medium transition border border-amber-500/30"
                          title="Reset Hardware ID"
                        >
                          <RefreshCw className="w-3.5 h-3.5 inline mr-1" /> Reset HWID
                        </button>

                        <button
                          onClick={() => handleAction('toggle_status', k.key)}
                          className="bg-slate-700/40 text-slate-300 hover:bg-slate-700/70 px-2.5 py-1.5 rounded-lg text-xs font-medium transition"
                        >
                          {k.isActive ? 'Disable' : 'Enable'}
                        </button>

                        <button
                          onClick={() => handleAction('delete', k.key)}
                          className="bg-rose-500/10 text-rose-400 hover:bg-rose-500/20 p-1.5 rounded-lg transition border border-rose-500/30"
                          title="Delete Key"
                        >
                          <Trash2 className="w-3.5 h-3.5" />
                        </button>
                      </td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        </div>

      </div>
    </div>
  );
}
