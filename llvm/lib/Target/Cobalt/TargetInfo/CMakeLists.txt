add_llvm_component_library(LLVMCobaltInfo
  CobaltTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  Cobalt
  )
