add_llvm_library( LLVMMultiDDGGen MODULE
  MultiDDGGen.cpp
  DFG.cpp
  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
