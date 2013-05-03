//===-- EpiphanyCondMovPass.cpp -----------------------===//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "epiphany_condmovpass"
#include "Epiphany.h"
#include "EpiphanyMachineFunctionInfo.h"
#include "EpiphanySubtarget.h"
#include "EpiphanyTargetMachine.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"

using namespace llvm;

namespace {

class EpiphanyCondMovPass : public MachineFunctionPass {

private:
  EpiphanyTargetMachine& QTM;

 public:
  static char ID;
  EpiphanyCondMovPass(EpiphanyTargetMachine& TM) : MachineFunctionPass(ID), QTM(TM) {}

  const char *getPassName() const {
    return "Epiphany movCC cleanup";
  }
  bool runOnMachineFunction(MachineFunction &Fn);
};

char EpiphanyCondMovPass::ID = 0;

bool EpiphanyCondMovPass::runOnMachineFunction(MachineFunction &Fn) {
	bool modified = false;
	// Loop over all of the basic blocks.
	for(MachineFunction::iterator MBBb = Fn.begin(), MBBe = Fn.end(); MBBb != MBBe; ++MBBb) {
		MachineBasicBlock* MBB = MBBb;

		std::vector<MachineInstr*> killmeplease;
		// Loop over all instructions.
		for(MachineBasicBlock::iterator MII = MBB->begin(), E = MBB->end(); MII != E; ++MII){
			MachineInstr *MI = &*MII;
			if((MI->getOpcode() == Epiphany::MOVww || MI->getOpcode() == Epiphany::MOVss) && MI->getOperand(0).isReg() && MI->getOperand(1).isReg() && MI->getOperand(0).getReg() == MI->getOperand(1).getReg())
				killmeplease.push_back(MI);
		}

		if(!killmeplease.empty()){
			modified = true;
			for (std::vector<MachineInstr*>::iterator kb = killmeplease.begin(), ke = killmeplease.end(); kb != ke ; kb++)
				(*kb)->eraseFromParent(); // we can safely do this because mov Rd, Rd is a nop with no side effects.
		}

	}
	return modified;
}

}// namespace


//===----------------------------------------------------------------------===//
//                         Public Constructor Functions
//===----------------------------------------------------------------------===//

FunctionPass *llvm::createEpiphanyCondMovPass(EpiphanyTargetMachine &TM) {
  return new EpiphanyCondMovPass(TM);
}
