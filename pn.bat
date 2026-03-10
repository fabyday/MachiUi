@echo off
setlocal
cd /d "%~dp0Source\JavaScript"
echo Target Directory: %CD%
call pnpm %*
endlocal