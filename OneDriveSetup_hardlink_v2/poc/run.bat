@echo off
mkdir C:\test
for /l %%i in (10000,1,11000) do (echo 1>C:\test\abc%%i.txt)
for /l %%i in (10000,1,11000) do (CreateHardlink.exe "C:\ProgramData\Microsoft OneDrive\setup\logs\Install-2020-02-03.1304.%%i.1.aodl" "C:\test\abc%%i.txt")
pause