@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File "%~dp0tools\launch_hook_test.ps1" %*
if errorlevel 1 (
    echo Launcher failed. See the message above and docs\FIRST_GAME_TEST.md.
    pause
    exit /b 1
)
endlocal
