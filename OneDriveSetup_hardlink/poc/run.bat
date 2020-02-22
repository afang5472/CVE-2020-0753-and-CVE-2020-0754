@echo off
mkdir C:\test
for /l %%i in (1000,1,2000) do (echo 1>C:\test\abc%%i.txt)
for /l %%i in (1000,1,2000) do (CreateHardlink.exe "C:\Windows\TEMP\aria-debug-%%i.log" "C:\test\abc%%i.txt")
pause