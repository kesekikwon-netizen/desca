@echo off
rem Kerf factory launcher. Works from any folder: factory status | new ... | verify <ID> | ci <sha> | help
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0factory\factory.ps1" %*
exit /b %ERRORLEVEL%
