#NoEnv
#SingleInstance Force
#Persistent
#InstallKeybdHook
#UseHook
#KeyHistory 0
#HotkeyInterval 1
#MaxHotkeysPerInterval 127
#NoTrayIcon

init:

version = 1.0
TrayTip, Supply Stabilizer %version%, Running in background!, 5, 1

Menu, Tray, NoStandard
Menu, Tray, Tip, Supply Stabilizer %version%
Menu, Tray, Add, Supply Stabilizer %version%, return
Menu, Tray, Add
Menu, Tray, Add, Help, info
Menu, Tray, Add, Exit, exit

SetKeyDelay -1, 1
SetControlDelay -1
SetMouseDelay -1
SetWinDelay -1
SendMode InputThenPlay
SetBatchLines -1
ListLines Off

CoordMode Pixel, Screen
CoordMode Mouse, Screen

PID := DllCall("GetCurrentProcessId")
Process Priority, %PID%, High

EMCol := 0xFF0000
ColVn := 64

AntiShakeX := A_ScreenHeight // 160
AntiShakeY := A_ScreenHeight // 128

ZeroX := A_ScreenWidth // 2
ZeroY := A_ScreenHeight // 2

CFovX := A_ScreenWidth // 20
CFovY := A_ScreenHeight // 20

ScanL := ZeroX - CFovX
ScanT := ZeroY
ScanR := ZeroX + CFovX
ScanB := ZeroY + CFovY

NearAimScanL := ZeroX - AntiShakeX
NearAimScanT := ZeroY - AntiShakeY
NearAimScanR := ZeroX + AntiShakeX
NearAimScanB := ZeroY + AntiShakeY

Loop {
    KeyWait LButton, D

    PixelSearch px, py, NearAimScanL, NearAimScanT, NearAimScanR, NearAimScanB, EMCol, ColVn, Fast RGB
    if (ErrorLevel = 0) {
        loop 20 {
            PixelSearch px, py, ScanL, ScanT, ScanR, ScanB, EMCol, ColVn, Fast RGB
            if ErrorLevel
                continue

            dx := px - ZeroX
            dy := py - ZeroY

            DirX := dx > 0 ? 1 : -1
            DirY := dy > 0 ? 1 : -1

            ox := Abs(dx)
            oy := Abs(dy)

            mx := Floor(ox * 0.48) * DirX
            my := Floor(oy * 0.48) * DirY

            DllCall("mouse_event", "UInt", 1, "Int", mx, "Int", my, "UInt", 0, "UInt", 0)
        }
    }
}

Pause::Pause

return:
goto init

info:
MsgBox 0, Supply Stabilizer %version%, Supply Stabilizer v%version%`n`n• Hold LMB to lock onto red targets`n• Press Pause key to toggle on/off`n• ESC to exit`n• Works in borderless windowed mode
return

exit:
ExitApp