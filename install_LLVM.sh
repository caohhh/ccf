echo Installing Clang/LLVM + LLVM-Gold

cd llvm-project
llvm_dir=$(pwd)
install_path="/home/caohhh/ccf/install" 

mkdir build
cd build
cmake -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_BINUTILS_INCDIR="${llvm_dir}/binutils/include" --enable-pic -DCMAKE_BUILD_TYPE=Release ../llvm -DCMAKE_INSTALL_PREFIX=${install_path}
make -j 8 && make -j 8 install

cd ../..