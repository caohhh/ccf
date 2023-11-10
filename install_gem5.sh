cd gem5
scons build/ARM/gem5.opt -j8
sed -i "/AtomicCGRA/! s/AtomicSimpleCPU/AtomicCGRA,AtomicSimpleCPU/g" ./build/variables/ARM
scons build/ARM/gem5.opt -j8
scons build/ARM/gem5.debug -j8
cd ../