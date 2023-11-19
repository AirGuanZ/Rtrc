cd ./External/llvm16.0.6

if exist "./llvm-project-16.0.6.src/" rmdir /s /q "./llvm-project-16.0.6.src"
"C:/Program Files/7-Zip/7z.exe" x -y llvm-project-16.0.6.src.7z

if exist "./build_debug/" rmdir /s /q "./build_debug"
if exist "./install_debug/" rmdir /s /q "./install_debug"
mkdir build_debug
mkdir install_debug
cd build_debug
cmake ..\llvm-project-16.0.6.src\llvm\ -DLLVM_ENABLE_PROJECTS='clang' -DCMAKE_INSTALL_PREFIX='%CD%/install_debug' -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config=Debug --parallel 16
cmake --install . --config=Debug
cd ..
if exist "./build_debug/" rmdir /s /q "./build_debug"

if exist "./build_release/" rmdir /s /q "./build_release"
if exist "./install_release/" rmdir /s /q "./install_release"
mkdir build_release
mkdir install_release
cd build_release
cmake ..\llvm-project-16.0.6.src\llvm\ -DLLVM_ENABLE_PROJECTS='clang' -DCMAKE_INSTALL_PREFIX='%CD%/install_release' -DCMAKE_BUILD_TYPE=Release
cmake --build . --config=Release --parallel 16
cmake --install . --config=Release
cd ..
if exist "./build_release/" rmdir /s /q "./build_release"

if exist "./llvm-project-16.0.6.src/" rmdir /s /q "./llvm-project-16.0.6.src"

cd ../..
