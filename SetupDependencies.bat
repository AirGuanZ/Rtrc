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

:: ====================================== boost multiprecision ======================================

cd ./External/
if exist "./multiprecision-Boost/" rmdir /s /q "./multiprecision-Boost/"
"C:/Program Files/7-Zip/7z.exe" x -y multiprecision-Boost.7z
cd ../

:: ====================================== d3d12 agality sdk ======================================

cd ./External/
if exist "./microsoft.direct3d.d3d12.1.715.0-preview/" rmdir /s /q "./microsoft.direct3d.d3d12.1.715.0-preview/"
"C:/Program Files/7-Zip/7z.exe" x -y microsoft.direct3d.d3d12.1.715.0-preview.7z
cd ../