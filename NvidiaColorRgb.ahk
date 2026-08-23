#NoEnv
#SingleInstance, Force
#Persistent
#InstallKeybdHook
#UseHook
#KeyHistory, 0
#HotKeyInterval 1
#MaxHotkeysPerInterval 127
#NoTrayIcon

init:
version = 1.0
traytip, Supply %version%, Running in background!, 5, 1
Menu, tray, NoStandard
Menu, tray, Tip, Supply %version%
Menu, tray, Add, Supply %version%, return
Menu, tray, Add
Menu, tray, Add, Help, info
Menu, tray, Add, Exit, exit
SetKeyDelay,-1, 1
SetControlDelay, -1
SetMouseDelay, -1
SetWinDelay,-1
SendMode, InputThenPlay
SetBatchLines,-1
ListLines, Off
CoordMode, Pixel, Screen, RGB
CoordMode, Mouse, Screen
PID := DllCall("GetCurrentProcessId")
Process, Priority, %PID%, High
 
EMCol := 0xff0000
ColVn := 64
AntiShakeX := (A_ScreenHeight // 160)
AntiShakeY := (A_ScreenHeight // 128)
ZeroX := (A_ScreenWidth // 2)
ZeroY := (A_ScreenHeight // 2)
CFovX := (A_ScreenWidth // 20)
CFovY := (A_ScreenHeight // 20)
ScanL := ZeroX - CFovX
ScanT := ZeroY
ScanR := ZeroX + CFovX
ScanB := ZeroY + CFovY
NearAimScanL := ZeroX - AntiShakeX
NearAimScanT := ZeroY - AntiShakeY
NearAimScanR := ZeroX + AntiShakeX
NearAimScanB := ZeroY + AntiShakeY

Loop, {
    KeyWait, LButton, D
    PixelSearch, AimPixelX, AimPixelY, NearAimScanL, NearAimScanT, NearAimScanR, NearAimScanB, EMCol, ColVn, Fast RGB
    if (!ErrorLevel=0) {
        loop, 10 {
            PixelSearch, AimPixelX, AimPixelY, ScanL, ScanT, ScanR, ScanB, EMCol, ColVn, Fast RGB
            AimX := AimPixelX - ZeroX
            AimY := AimPixelY - ZeroY
            DirX := -1
            DirY := -1
            If ( AimX > 0 ) {
                DirX := 1
            }
            If ( AimY > 0 ) {
                DirY := 1
            }
            AimOffsetX := AimX * DirX
            AimOffsetY := AimY * DirY
            MoveX := Floor(( AimOffsetX ** ( 1 / 2 ))) * DirX
            MoveY := Floor(( AimOffsetY ** ( 1 / 2 ))) * DirY
            DllCall("mouse_event", uint, 1.0, int, MoveX * 2.0, int, MoveY, uint, 0.7, int, 0.7)
        }
    }
}

Pause:: pause
return:
goto, init

info:
msgbox, 0, Supply %version%, Made by Supply`nGame must be running in borderless windowed mode.`nPress pause key to pause this program.`nLeft click automatically aims down target near the center of the screen.`nRecommended for near distance(~15m) and full-auto weapons.
return

exit:
exitapp