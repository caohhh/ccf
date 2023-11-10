ccf_root=$(pwd)
#Change this to the path you want and make sure it is a true path!
install_path="/home/caohhh/ccf/install" 

# Installing CMake'
cmake_version="$(cmake -version | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' | head -1)"
cmake_exist=$(which cmake)
if [ $(echo "$cmake_version > 3.19" | bc) -eq 0 -o -z "$cmake_exist" ]; then
    echo Installing CMake 3.19.5
    
    wget https://github.com/Kitware/CMake/releases/download/v3.19.5/cmake-3.19.5.tar.gz --no-check-certificate
    tar zxvf cmake-3.19.5.tar.gz    
    cd cmake-3.19.5
    
    ./configure --prefix=${install_path}
    make -j 8 && make -j 8 install
    
    cd ..
    
    rm -rf cmake-3.19.5*
fi