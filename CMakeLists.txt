set(LLVM_TARGET_DEFINITIONS Epiphany.td)

#tablegen(LLVM EpiphanyGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM EpiphanyGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM EpiphanyGenCallingConv.inc -gen-callingconv)
#tablegen(LLVM EpiphanyGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM EpiphanyGenInstrInfo.inc -gen-instr-info)
#tablegen(LLVM EpiphanyGenMCCodeEmitter.inc -gen-emitter -mc-emitter)
tablegen(LLVM EpiphanyGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM EpiphanyGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM EpiphanyGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM EpiphanyGenSubtargetInfo.inc -gen-subtarget)
add_public_tablegen_target(EpiphanyCommonTableGen)

add_llvm_target(EpiphanyCodeGen
  EpiphanyAsmPrinter.cpp
  EpiphanyFrameLowering.cpp
  EpiphanyISelDAGToDAG.cpp
  EpiphanyISelLowering.cpp
  EpiphanyInstrInfo.cpp
  EpiphanyMachineFunctionInfo.cpp
  EpiphanyMCInstLower.cpp
  EpiphanyRegisterInfo.cpp
  EpiphanySelectionDAGInfo.cpp
  EpiphanySubtarget.cpp
  EpiphanyTargetMachine.cpp
  EpiphanyTargetObjectFile.cpp
  EpiphanyLSOptPass.cpp
  CondMovPass.cpp
  )

#add_subdirectory(AsmParser)
#add_subdirectory(Disassembler)
add_subdirectory(InstPrinter)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
add_subdirectory(Utils)