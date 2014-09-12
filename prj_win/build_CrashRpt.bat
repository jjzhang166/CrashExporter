@echo off
echo "CrashRpt build begin"

if defined VS100COMNTOOLS (call "%VS100COMNTOOLS%\vsvars32.bat")
 
devenv CrashRpt.vcxproj /Rebuild "Release"  

echo "CrashRpt build over"

rmdir /s /q Release





