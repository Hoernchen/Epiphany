//===-- EpiphanyMachineFuctionInfo.cpp - Epiphany machine function info -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file just contains the anchor for the EpiphanyMachineFunctionInfo to
// force vtable emission.
//
//===----------------------------------------------------------------------===//
#include "EpiphanyMachineFunctionInfo.h"

using namespace llvm;

void EpiphanyMachineFunctionInfo::anchor() { }
