::首先要确保路径存在才运行

echo "Enter Release_64 to build"
mkdir ../build-ComAssistant-Desktop_Qt_5_14_1_MinGW_64_bit-Release/
cd ../build-ComAssistant-Desktop_Qt_5_14_1_MinGW_64_bit-Release/
qmake.exe ../ComAssistant/ComAssistant.pro -spec win32-g++ "CONFIG+=qtquickcompiler"
mingw32-make.exe -j8 2>&1

echo "Return back"
cd ../ComAssistant/

echo "Enter Release_32 to build"
mkdir ../build-ComAssistant-Desktop_Qt_5_14_1_MinGW_32_bit-Release
cd ../build-ComAssistant-Desktop_Qt_5_14_1_MinGW_32_bit-Release/
qmake.exe ../ComAssistant/ComAssistant.pro -spec win32-g++ "CONFIG+=qtquickcompiler"
mingw32-make.exe -j8 2>&1

echo "Start pack software"
cd "../build-ComAssistant-Desktop_Qt_5_14_1_MinGW_64_bit-Release/release - min/"
z_auto_pack.bat

echo "finished!!"
pause