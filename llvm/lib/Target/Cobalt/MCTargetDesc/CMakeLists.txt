add_llvm_component_library(LLVMCobaltDesc
  CobaltMCAsmInfo.cpp
  CobaltMCTargetDesc.cpp

  LINK_COMPONENTS
  CobaltInfo
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  Cobalt
  )

add_dependencies(LLVMCobaltDesc CobaltCommonTableGen)
