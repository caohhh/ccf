# Changing toolchain path of cgracc and cgra++ and copying them 
# into ${install_path}/bin


ccf_root=$(pwd)
#Change this to the path you want and make sure it is a true path!
install_path="/home/caohhh/ccf/install" 

cd scripts/CGRALib
sed -i "/ccf_root\=/c\ccf_root=\"${ccf_root}\"" cgracc 
sed -i "/ccf_root\=/c\ccf_root=\"${ccf_root}\"" cgra++ 
sed -i "/ccf_root\=/c\ccf_root=\"${ccf_root}\"" cgraexe
sed -i "/ccf_root\=/c\ccf_root=\"${ccf_root}\"" cgradb
sed -i "/install_path\=/c\install_path=\"${install_path}\"" cgracc 
sed -i "/install_path\=/c\install_path=\"${install_path}\"" cgra++ 
sed -i "/install_path\=/c\install_path=\"${install_path}\"" cgraexe
sed -i "/install_path\=/c\install_path=\"${install_path}\"" cgradb
cp cgracc cgra++ cgraexe cgradb ${install_path}/bin
echo "cgracc, cgra++, and cgraexe manipulation and copy complete"
cd ../..