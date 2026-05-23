add_llvm_component_group(Cobalt)

set(LLVM_TARGET_DEFINITIONS Cobalt.td)

tablegen(LLVM CobaltGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM CobaltGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM CobaltGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(CobaltCommonTableGen)

add_llvm_target(CobaltCodeGen
  CobaltFrameLowering.cpp
  CobaltInstrInfo.cpp
  CobaltRegisterInfo.cpp
  CobaltSubtarget.cpp
  CobaltTargetMachine.cpp

  DEPENDS
  CobaltCommonTableGen

  LINK_COMPONENTS
  CobaltDesc
  CobaltInfo
  CodeGen
  Core
  Support
  Target
  TargetParser

  ADD_TO_COMPONENT
  Cobalt
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
