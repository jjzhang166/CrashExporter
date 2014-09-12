@echo off
echo "CrashExporter build begin"

if defined VS100COMNTOOLS (call "%VS100COMNTOOLS%\vsvars32.bat")

devenv ..\thirdparty\zlib\zlib.vcxproj /Rebuild "Release" 
devenv ..\thirdparty\libpng\libpng.vcxproj /Rebuild "Release" 
devenv CrashExporter.vcxproj /Rebuild "Release" 

echo "CrashExporter build over"



rmdir /s /q Release





