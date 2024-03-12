# Changing toolchain path of SplitMap and copying it 
# into ${install_path}/bin


ccf_root=$(pwd)
#Change this to the path you want and make sure it is a true path!
install_path="/home/caohhh/ccf/install" 

cd scripts/CGRALib
chmod +x SplitMap
sed -i "/ccf_root\=/c\ccf_root=\"${ccf_root}\"" SplitMap 
sed -i "/install_path\=/c\install_path=\"${install_path}\"" SplitMap 
cp -p SplitMap  ${install_path}/bin
echo "SplitMap manipulation and copy complete"
cd ../..