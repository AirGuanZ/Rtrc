:: ====================================== tbb ======================================

cd ./External/
if exist "./oneapi-tbb-2021.11.0/" rmdir /s /q "./oneapi-tbb-2021.11.0/"
"C:/Program Files/7-Zip/7z.exe" x -y oneapi-tbb-2021.11.0.7z
cd ../

:: ====================================== eigen ======================================

cd ./External/
if exist "./eigen-3.4.0/" rmdir /s /q "./eigen-3.4.0/"
"C:/Program Files/7-Zip/7z.exe" x -y eigen-3.4.0.7z
cd ../
