install_path="/home/caohhh/ccf/install" 

cd mappings
echo Building Mapping Part
find . -maxdepth 1 -mindepth 1 -type d | while read dir; do
   
    cd $dir
    exe_name=${PWD##*/}
    LINE="/${exe_name}/Release/${exe_name}"
    FILE='../.gitignore'
    grep -qxF -- "$LINE" "$FILE" || echo "$LINE" >> "$FILE"
    LINE="/${exe_name}/Release/debugfile"
    grep -qxF -- "$LINE" "$FILE" || echo "$LINE" >> "$FILE"

    cd ./Release
    make
    cd ../DFGFiles
    g++ -O3 nodefile.cpp -o nodefile
    g++ -O3 edgefile.cpp -o edgefile
    cd ../../
done
echo "Mapping compilation complete"
cd ..