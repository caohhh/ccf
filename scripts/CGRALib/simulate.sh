#!/bin/bash
ccf_root="/home/caohhh/ccf/ccf"


obj=""

if [ $# -eq 0 ]; then
    ${ccf_root}/gem5/build/ARM/gem5.opt --debug-flags=CGRA,CGRA_Execute,CGRA_Detailed,Instruction_Fetch,CGRA_IFU,CGRAPred ${ccf_root}/gem5/configs/CGRA/config.py -n 1 --cpu-type CGRA4x4R4torus -c $obj &> out.log
else
    # If arguments are provided
    ${ccf_root}/gem5/build/ARM/gem5.opt --debug-flags=CGRA,CGRA_Execute,CGRA_Detailed,Instruction_Fetch,CGRA_IFU,CGRAPred ${ccf_root}/gem5/configs/CGRA/config.py -n 1 --cpu-type CGRA4x4R4torus -c $obj -o "$@" &> out.log
fi
