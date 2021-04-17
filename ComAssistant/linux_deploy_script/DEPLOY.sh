# Reference(How to deploy qt program):https://www.cnblogs.com/linuxAndMcu/p/11016322.html

echo "copy new ComAssistant"
cp -f ../ComAssistant ./

echo "collect dependence files"
./linuxdeployqt-7-x86_64.AppImage ComAssistant -appimage

echo "prepare file to folder"
releaseFolder="ComAssistantLinux"
rm -rf $releaseFolder
mkdir $releaseFolder
mv doc/ $releaseFolder
mv lib/ $releaseFolder
# mv translations/ $releaseFolder
mv plugins/ $releaseFolder
mv ComAssistant $releaseFolder
cp install.sh $releaseFolder
cp en_US.qm $releaseFolder

echo "compress file"
rm -rf $releaseFolder.zip
zip -r $releaseFolder.zip $releaseFolder

echo "clean file"
rm -rf $releaseFolder
rm -rf AppRun
rm -rf default.desktop
rm -rf default.png
rm -rf qt.conf

echo "success!"


