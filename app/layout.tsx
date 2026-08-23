import type { Metadata } from 'next';
import './globals.css';

export const metadata: Metadata = {
  title: 'Hollow Launcher - License Key Dashboard',
  description: 'Pro Web-Hosted License Key Manager & API for Launcher.exe',
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className="dark">
      <body className="bg-[#0F111A] text-slate-100 antialiased min-h-screen">
        {children}
      </body>
    </html>
  );
}
