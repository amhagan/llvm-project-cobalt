add_llvm_component_group(Cobalt)

add_llvm_target(CobaltCodeGen
  CobaltTargetMachine.cpp

  LINK_COMPONENTS
  CobaltInfo
  Core
  Support
  Target
  TargetParser

  ADD_TO_COMPONENT
  Cobalt
  )

add_subdirectory(TargetInfo)
