add_llvm_component_library(LLVMCobaltDesc
  CobaltAsmBackend.cpp
  CobaltELFObjectWriter.cpp
  CobaltInstPrinter.cpp
  CobaltMCAsmInfo.cpp
  CobaltMCCodeEmitter.cpp
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
