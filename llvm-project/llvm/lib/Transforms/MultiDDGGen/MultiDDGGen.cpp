//===-- MultiDGGGen.cpp - Generate Multi Data Dependency Graph ------------===//
//
// Author: Cao Heng
// Date: 2023/11/30
//
//===----------------------------------------------------------------------===//
///
/// This file is modified on top of CondDDGGen, transforms a annoted loop into
/// a Multi-DDG where with conditional branches, only one will be executed on 
/// the CGRA.
///
//===----------------------------------------------------------------------===//

#include <tuple>
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Support/CommandLine.h"


#include "DFG.h"

using namespace llvm;

#define DEBUG_TYPE "multiDDG"



namespace {

static cl::opt<int> splitBr("split", cl::desc("the branch id for the DFG to be split at"), cl::init(-1));

/**
 * Get the loop hint metadata node with a given name
 * @param L the loop
 * @param Name the CGRA metadata name to be checked (e.g. "llvm.loop.CGRA.enable")
 * @return the metadata node, or nullptr if no such node exists
*/
static const MDNode* 
GetCGRAMetadata(const Loop *L, StringRef Name) 
{
  MDNode *LoopID = L->getLoopID();
  if (!LoopID)
    return nullptr;

  // First operand should refer to the loop id itself.
  assert(LoopID->getNumOperands() > 0 && "requires at least one operand");
  assert(LoopID->getOperand(0) == LoopID && "invalid loop id");

  for (unsigned i = 1, e = LoopID->getNumOperands(); i < e; ++i) {
    const MDNode *MD = dyn_cast<MDNode>(LoopID->getOperand(i));
    if (!MD)
      continue;

    const MDString *S = dyn_cast<MDString>(MD->getOperand(0));
    if (!S)
      continue;

    if (Name.equals(S->getString()))
      return MD;
  }
  return nullptr;
}


// Returns true if the loop has an CGRA(disable) pragma.
static bool 
HasCGRADisablePragma(const Loop *L) 
{
  return GetCGRAMetadata(L, "llvm.loop.CGRA.disable");
}


// Returns true if the loop has an CGRA(enable) pragma.
static bool 
HasCGRAEnablePragma(const Loop *L) 
{
  return GetCGRAMetadata(L, "llvm.loop.CGRA.enable");
}

/**
 * check if the loop can be executed on CGRA
*/
static bool 
canExecuteOnCGRA(Loop *L, std::vector<BasicBlock *> bbs, BasicBlock * Preheader, BasicBlock *ExitBlk)
{
  bool retVal = true;
  unsigned depth = L->getLoopDepth();

  std::vector<Loop *> subLoops = L->getSubLoops();
  unsigned subLoopsize = subLoops.size();

  if (subLoopsize > 0) {
    DEBUG("1) Nested Loop with Depth " << depth << ". ");
    DEBUG("The loop contains " << subLoopsize << " SubLoops.\n");
    retVal = false;
  }

  SmallVector<BasicBlock*, 8> ExitingBlocks;
  L->getExitingBlocks(ExitingBlocks);
  if (ExitingBlocks.size() != 1) {
    DEBUG("2) The loop has " << ExitingBlocks.size() << " exits.\n");
    retVal = false;
  }

  unsigned totalInstructions=0;
  unsigned brInstructions = 0;
  unsigned functionCalls = 0;
  unsigned floatignPtInstructions = 0;
  unsigned vectorInstructions = 0;
  for (unsigned i = 0; i < (unsigned) bbs.size(); i++) {
    totalInstructions += bbs[i]->size();
    for (BasicBlock::iterator BI = bbs[i]->begin(); BI != bbs[i]->end(); ++BI) {
      if (BI->getOpcode() == Instruction::Br) {
        BranchInst* tempBrInst = dyn_cast<llvm::BranchInst>(BI);
        if (tempBrInst->isConditional())
          brInstructions++;
      } else if(BI->getOpcode() == Instruction::Call) {
        //TODO: Check function call can be inlined
        //Check if call to a library function can be separated from user defined function
        functionCalls++;
      } else if ((BI->getOpcode() == Instruction::FAdd) ||
                (BI->getOpcode() == Instruction::FSub) ||
                (BI->getOpcode() == Instruction::FMul) ||
                (BI->getOpcode() == Instruction::FDiv) ||
                (BI->getOpcode() == Instruction::FRem) ||
                (BI->getOpcode() == Instruction::FPTrunc)) {
        floatignPtInstructions++;
      } else if ((BI->getOpcode() == Instruction::ExtractElement) ||
                (BI->getOpcode() == Instruction::InsertElement) ||
                (BI->getOpcode() == Instruction::ShuffleVector)) {
        vectorInstructions++;
      }
    }
  }

  // vector instructions in preheader and exit block are not checked


  if (functionCalls > 0) {
    DEBUG("5) Loop with total " << functionCalls << " Function Calls.\n");
    retVal = false;
  }


  if (vectorInstructions > 0) {
    DEBUG("7) Loop with " << vectorInstructions << " vector instructions.\n");
    retVal = false;
  }

  if (retVal == false) {
    DEBUG("Cannot execute this loop on CGRA.\n");
    DEBUG("Compiling for execution on CPU.\n");
  }

  return retVal;
}


//return true if set contains current basicblock
static bool 
contains(BasicBlock* current, std::vector<BasicBlock*> set)
{
  for (std::vector<BasicBlock*>::const_iterator it = set.begin() ; it != set.end(); ++it) {
    if (current == (*it))
      return true;
  }
  return false;
}


/**
 * get the distance between 2 instructions, either 0 or 1 at this level
 * @param insFrom Instruction providing variable
 * @param insTo Instruction the variable is heading to
 * @param bbs Basic blocks of the loop
 * @param loopLatch Loop latch basic block
 * @return 0 for intra iteration dependency 
 *         1 for inter iteration dependency 
 *         -1 for error
*/
static int 
getDistance(Instruction *insFrom, Instruction *insTo, std::vector<BasicBlock *> bbs, BasicBlock* loopLatch)
{
  BasicBlock* bbFrom = insFrom->getParent();
  BasicBlock* bbTo = insTo->getParent();
  //both within the same basic blocks, find the order in BB
  if (bbFrom == bbTo) {
    for (BasicBlock::iterator BI = bbFrom->begin(); BI != bbFrom->end(); ++BI) {
      //insFrom is visited before insTo-intra iteration dependency
      if (BI->isIdenticalTo(insFrom)) {
        return 0;
      }
      //insTo is visited before insFrom-inter-iteration dependency
      if (BI->isIdenticalTo(insTo)) {
        return 1;
      }
    }
    //should never reach here
    return -1;
  } else {
    //2 different blocks
    //if bbFrom is looplatch, because looplatch is the last basic block, dependency is loopcarried dependency and distance = 1
    if (bbFrom == loopLatch) {
      return 1;
    }
    //set of basicblocks to visit
    std::vector<BasicBlock*> to_visit;
    //set of already visited basicblocks
    std::vector<BasicBlock*> visited;
    //we start from bbFrom
    to_visit.push_back(bbFrom);
    //traverse loop CFG
    while (to_visit.size() > 0) {
      BasicBlock* it = to_visit[0];
      to_visit.erase(to_visit.begin());
      visited.push_back(it);
      //what are the set of next basicblocks
      for (succ_iterator pblocks = llvm::succ_begin(it); pblocks != llvm::succ_end(it); pblocks++) {
        //if BB is not within loop BB, ignore it
        if (!contains(*pblocks, bbs)) {
          continue;
        }
        //if we have reached bbTo, it is dependency inside 1 iteration
        if (bbTo == (*pblocks)) {
          return 0;
        }
        //we do not traverse beyond loop latch, if we dont reach the second block, it is inter iteration
        //we should not stop here though because there might be more blocks in to_visit set that has not traversed yet
        if ((*pblocks) == loopLatch) {
          continue;
        }
        //is this bb already visited? dont add it to traverse list
        if (contains(*pblocks, visited)) {
          continue;
        }
        //is it already added to traverse list?
        if (contains(*pblocks, to_visit)) {
          continue;
        }
        //add it to traverse list
        to_visit.push_back(*pblocks);
      }
    }
    //if we are here, it is inter-iteration dependency
    return 1;
  }
}


/**
 * Transform LLVM type to datatype in DFG 
 * also for structs, add their sizes and member sizes to respective maps
 * @param T LLVM type
 * @param bitWidth primitive size bit width
 * @return tuple of the datatype in DFG and the size this type occupies
*/
static std::tuple<Datatype, unsigned> 
getDatatype(Type* T, int bitWidth)
{
  Datatype dt;
  unsigned size = 0;
  bool isPrimitive = false;

  if (T->isIntegerTy()) {
    // integer
    if (bitWidth == 1)
      dt = character;
    else if(bitWidth == 2)
      dt = int16;
    else
      dt = int32;
    isPrimitive = true;

  } else if (T->isFloatingPointTy()) {
    // floating point
    if (T->isDoubleTy())
      dt = float64;
    else if (bitWidth == 2)
      dt = float16;
    else
      dt = float32;
    isPrimitive = true;

  } else if (T->isVectorTy()) {
    DEBUG("ERROR: Vectorization detected! Exiting.\n");
    exit(1);

  } else if (T->isArrayTy()) {
    Type* element_type = T->getArrayElementType();
    // recurssively get the size of the element
    auto [elementDT, elementSize] = getDatatype(element_type, element_type->getPrimitiveSizeInBits()/8);
    size = elementSize * T->getArrayNumElements();
    DEBUG("\t    Current node is of type array - numElements: " << T->getArrayNumElements());
    DEBUG(" - element size: " << elementSize << "\n");
    dt = elementDT;

  } else if (T->isPointerTy()) {
    DEBUG("\t    Current node is of type pointer\n");
    Type* element_type = T->getPointerElementType();
    dt = int32;
    isPrimitive = true;

  } else if(T->isStructTy()) {
    StructType* temp = dyn_cast<StructType>(T);
    if (!temp->isPacked()) {
      DEBUG("\nERROR: " << temp->getName().str());
      DEBUG(" is not packed, make sure padding is disabled by adding #pragma pack(push, 1) before declaration\n");
      exit(1);
    }

    std::string structName = T->getStructName().str();
    unsigned structElements = T->getStructNumElements();
	  DEBUG("\tStruct type - name: " << structName << " - number of elements: " << structElements << "\n");

    for (int i = 0; i < structElements; i++) {
      Type* structMember = T->getStructElementType(i);
      // recurssively update each member size
      auto [memberDT, memberSize] = getDatatype(structMember, structMember->getPrimitiveSizeInBits()/8);
      DEBUG("\t  Member " << i << " - dt: " << memberDT << " - size: " << memberSize << "\n");
      size += memberSize;
    }
    dt = int32;

  } else {
    DEBUG("\t  Cannot find data type! Defaulting to int32!\n");
    dt = int32;
    isPrimitive = true;
  }

  if (isPrimitive) {
    size = DT_Size[dt];
  }

  return std::make_tuple(dt, size); 
}

class MultiDDGGen : public LoopPass {
  public:
    static char ID;
    // default constructor
    MultiDDGGen();
    
    bool runOnLoop(Loop *L, LPPassManager &LPM) override;
    void getAnalysisUsage(AnalysisUsage &AU) const override;

  private:
    // number of loops to be executed on CGRA
    unsigned totalLoops = 0;

    unsigned nodeID = 0;
    unsigned edgeID = 0;

    unsigned gVarNo = 0;
    unsigned gPtrNo = 0;

    std::ofstream liveInNodefile;
    std::ofstream liveInEdgefile;
    std::ofstream liveoutNodefile;
    std::ofstream liveoutEdgefile;


    // map of livein value to their name
    std::map<llvm::Value*, std::string> mapLiveinValueName;

    // map of livein global value to their alignment
    std::map<std::string, unsigned> mapLiveinNameAlignment;

    // map of livein global value to their datatype
    std::map<std::string, Datatype> mapLiveinNameDatatype; 

    // map of liveout instruction to their liveout variable name
    std::map<Instruction*, std::string> mapLiveoutInstName;

    // a basic block created between loop latch and exit block for
    // the loading of liveout variables
    BasicBlock* loadBlock;

    // struct to store all the path info of a branch
    struct branchPaths
    {
      std::vector<unsigned> truePath;
      std::vector<unsigned> falsePath;
    };
    
    // map of conditional branch index (the basic block it belongs to) 
    // to all the dominated blocks of either true or false path
    std::map<int, branchPaths> mapBrIdPaths;

    /**
     * updates the branching information of each basic block of the loop
     * populate the map for branch and their paths
    */
    void updateBranchInfo(Loop* L, DominatorTree* DT);

    /**
     * attempt to add a new node to the DFG from the instruction
     * @param BI instruction to form a node
     * @param myDFG the DFG to add to
     * @param bbIdx index of the basic block this instruction belongs to
     * @return if the node is added
    */
    bool addNode(Instruction* BI, DFG* myDFG, unsigned bbIdx);

    // resets variables not passed between loop runs
    void resetVariables();

    /**
     * update the livein variable maps of the loop, for later load in insturction generation
     * @param BI instruction that may contain livein variable
     * @param bbs basic blocks of the loop
     * @param Preheader preheader block of the loop
     * @return if the IR has been modified
    */
    bool updateLiveInVariables(Instruction* BI, std::vector<BasicBlock*> bbs, BasicBlock* Preheader);

    /**
     * update the edges of the DFG
     * @param BI instruction of the node
     * @param loopDFG the DFG of the loop
     * @param L the loop
     * @param DT dominator tree of the function
     * @param bbIdx the index of the basic block BI belongs to
    */
    void updateDataDependencies(Instruction* BI, DFG* loopDFG, Loop* L, DominatorTree* DT, unsigned bbIdx);

    /**
     * go through each instruction in the loop with a node 
     * and find if there is a live out variable
     * @param BI instruction to check later use
     * @param loopDFG DFG of the loop
     * @param L the loop
     * @param bbIDx index of the basic block BI belongs to
     * @return if the IR has been modified
    */
    bool updateLiveOutVariables(Instruction* BI, DFG* loopDFG, Loop* L, unsigned bbIdx);

    /**
     * This function makes edges from a unique loop control node to
     * liveOut nodes to be later used by mapping algos to enforce
     * mapping constraints
     * @param L the loop
     * @param loopDFG DFG of the loop
     * @return tuple of the loop exit control node and exit condition
    */
    std::tuple<NODE*, bool> updateLoopControl(Loop *L, DFG* loopDFG);

    /**
     * Split the DFG of the loop into true path and false path 
     * given the branch id to split. If the branch id is out of 
     * bounds (-1 for instance), the DFG won't be split.
     * @param loopDFG DFG of the loop
     * @param splitBrId id of the branch to split at
    */
    void splitDFG(DFG* loopDFG, int splitBrId);

    /**
     * Given a split DFG, as in there are nodes assigned to true or
     * false path, fuse the nodes of the true and false path 
     * together, leaving only one path.
    */
    void fuseNodes(DFG* loopDFG);
}; // end of class MultiDDGGen

} // end of anonymous namespace


MultiDDGGen::MultiDDGGen()
  : LoopPass(ID)
{
}


void 
MultiDDGGen::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.setPreservesCFG();

  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<SCEVAAWrapperPass>();
  AU.addPreservedID(LoopSimplifyID);
  AU.addPreserved<AAResultsWrapperPass>();
  AU.addPreserved<BasicAAWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();

  // This is needed to perform LCSSA verification inside LPPassManager
  AU.addRequired<LCSSAVerificationPass>();
  AU.addPreserved<LCSSAVerificationPass>();
}


void 
MultiDDGGen::updateBranchInfo(Loop* L, DominatorTree* DT)
{
  DEBUG("updating branch info\n");
  // basic blocks of the loop
  std::vector<BasicBlock *> bbs = L->getBlocks();
  for (int i = 0; i < (int) bbs.size(); i++) {
    // skip latch
    if (bbs[i] == L->getLoopLatch())
      continue;
    // skip uncond branch
    if (bbs[i]->getSingleSuccessor() != nullptr)
      continue;
    // now find the br instruction
    BranchInst* brInst;
    DEBUG("multiple successors with basic block " << i << "\n");
    for (BasicBlock::iterator BBI = bbs[i]->begin(); BBI != bbs[i]->end(); ++BBI) {
      if (BBI->getOpcode() != Instruction::Br)
        continue;
      DEBUG("the branch instruction: " << (*BBI) << "\n");
      brInst = dyn_cast<llvm::BranchInst>(BBI);
      if (brInst->isUnconditional()) {
        DEBUG("Weird, unconditional branch at the block\n");
        continue;
      }
    }
    // now find the true block and false block
    // for br inst, successor 0 is true path, successor 1 is false path
    branchPaths paths;
    BasicBlockEdge* trueEdge = new BasicBlockEdge(bbs[i], brInst->getSuccessor(0));
    BasicBlockEdge* falseEdge = new BasicBlockEdge(bbs[i], brInst->getSuccessor(1));

    // here we get the blocks that blong to true or false path
    // mabe ii should start at i for efficiency, but just in case
    // here it starts at 0
    for (int ii = 0; ii < (int) bbs.size(); ii++) {
      if (DT->dominates(*trueEdge, bbs[ii])) {
        paths.truePath.push_back(ii);
        DEBUG("block " << ii << " in true path\n");
      } else if (DT->dominates(*falseEdge, bbs[ii])) {
        paths.falsePath.push_back(ii);
        DEBUG("block " << ii << " in false path\n");
      }
    }
    mapBrIdPaths[i] = paths;
  }
  DEBUG("Done updating branch info\n");
}


bool
MultiDDGGen::addNode(Instruction *BI, DFG* myDFG, unsigned bbIdx)
{
  Value *v = dyn_cast<Value>(BI);
  Type* T = v->getType();
  int bitWidth = T->getPrimitiveSizeInBits()/8; 
  auto [dt, size] = getDatatype(T, bitWidth);
  
  NODE *node;
  switch (BI->getOpcode()) {
    // Terminator Instructions
    case Instruction::Ret:
      DEBUG("\n\nReturn Detected!\n\n");
      return false;

    case Instruction::Br:
      return true;

    // switch not implemented yet, should add later if have time
    case Instruction::Switch:
      DEBUG("\n\nSwitch Detected!\n\n");
      DEBUG("num_cases: " << cast<SwitchInst>(BI)->getNumCases() << "\n");
      return false;

    case Instruction::IndirectBr:
      errs() << "\n\nIndirectBr Detected!\n\n";
      return false;
    case Instruction::Invoke:
      errs() << "\n\nInvoke Detected!\n\n";
      return false;
    case Instruction::Resume:
      errs() << "\n\nResume Detected!\n\n";
      return false;
    case Instruction::CatchSwitch:
      errs() << "\n\ncatchswitch Detected!\n\n";
      return false;
    case Instruction::CatchRet:
      errs() << "\n\ncatchret Detected!\n\n";
      return false;
    case Instruction::CleanupRet:
      errs() << "\n\ncleanupret Detected!\n\n";
      return false;
    case Instruction::Unreachable:
      errs() << "\n\nUnreachable Detected!\n\n";
      return false;

    // Standard binary operators...
    case Instruction::Add:
    case Instruction::FAdd:  
      node = new NODE(add, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt); 
      myDFG->insert_Node(node);
      return true;

    case Instruction::Sub:
    case Instruction::FSub:
      node = new NODE(sub, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    case Instruction::Mul:
    case Instruction::FMul:
      node = new NODE(mult, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::FDiv:
      node = new NODE(division, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    case Instruction::URem:
    case Instruction::SRem:
    case Instruction::FRem:
      node = new NODE(rem, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;
    
    // Bitwise Binary Operations
    case Instruction::Shl:
      node = new NODE(shiftl, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;
    case Instruction::LShr:
      node = new NODE(shiftr_logical, 1, nodeID++, BI->getName().str(), BI,bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;
    case Instruction::AShr:
      node = new NODE(shiftr, 1, nodeID++, BI->getName().str(), BI,bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;
    case Instruction::And:
      node = new NODE(andop, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;
    case Instruction::Or:
      node = new NODE(orop, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;
    case Instruction::Xor:
      node = new NODE(xorop, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    // Memory instructions...
    case Instruction::Alloca:
      return true;

    case Instruction::Load: {
      /* Check Type */
      node = new NODE(ld_add, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      NODE* nodeData = new NODE(ld_data, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->set_Load_Data_Bus_Read(nodeData);
      nodeData->set_Load_Address_Generator(node);
      node->setDatatype(dt);
      nodeData->setDatatype(dt);

      LoadInst *tempLoadInst;
      tempLoadInst = dyn_cast<LoadInst>(BI);
      unsigned alignment = tempLoadInst->getAlignment();

      // Check if loading node targets a struct then adjust alignment to struct size
      if (dyn_cast<Value>(BI)->getType()->isPointerTy()) {
        Type* BI_type = dyn_cast<Value>(BI)->getType();
        Type* element_type = BI_type->getPointerElementType();
        if (element_type->isStructTy()) {
          BI_type = element_type;
          Datatype dummyDT;
          std::tie(dummyDT, alignment) = getDatatype(BI_type, BI_type->getPrimitiveSizeInBits()/8);
          DEBUG("\t  Loading struct- alignment: " << alignment << "\n");
        }
      } else if (dyn_cast<Value>(BI)->getType()->isStructTy()) {
        Type* BI_type = dyn_cast<Value>(BI)->getType();
        Datatype dummyDT;
        std::tie(dummyDT, alignment) = getDatatype(BI_type, BI_type->getPrimitiveSizeInBits()/8);
        DEBUG("\t  Loading struct - alignment: " << alignment << "\n");
      }
	  
      node->setAlignment(alignment);
      myDFG->insert_Node(node);
      myDFG->insert_Node(nodeData);
      return true;
    }
    
    case Instruction::Store: {
      node = new NODE(st_add, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      NODE* nodeData = new NODE(st_data, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->set_Store_Data_Bus_Write(nodeData);
      nodeData->set_Store_Address_Generator(node);
      StoreInst *tempStoreInst;
      tempStoreInst = dyn_cast<StoreInst>(BI);
      unsigned alignment = tempStoreInst->getAlignment();
      DEBUG("alignment for store: " << alignment << "\n");
      DEBUG("T: " << T << "\n");
      StoreInst* si = dyn_cast<StoreInst>(BI); 
      Value *vi = si->getPointerOperand(); 
      Type* Ti = vi->getType()->getPointerElementType();
      DEBUG("store name: " << node->get_ID() << "\n"); 
      std::tie(dt, size) = getDatatype(Ti, alignment); 
      DEBUG("Datatype: " << dt << "\n");
      node->setAlignment(alignment);
      node->setDatatype(dt);
      nodeData->setDatatype(dt);
      myDFG->insert_Node(node);
      myDFG->insert_Node(nodeData);
      return true;
    }

    case Instruction::AtomicCmpXchg:
      DEBUG("\n\nAtomicCmpXchg Detected!\n\n");
      return false;
    case Instruction::AtomicRMW:
      DEBUG("\n\nAtomicRMW Detected!\n\n");
      return false;
    case Instruction::Fence:
      DEBUG("\n\nFence Detected!\n\n");
      return false;

    case Instruction::GetElementPtr:
      node = new NODE(add, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      dt = int32; 
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    // Convert instructions...
    case Instruction::Trunc:
    case Instruction::ZExt:
      node = new NODE(andop, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    case Instruction::SExt:
      node = new NODE(sext, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    case Instruction::FPTrunc:
    case Instruction::FPExt:
      node = new NODE(andop, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    case Instruction::FPToUI:
    case Instruction::FPToSI:
      node = new NODE(add, 1, nodeID++, BI->getName().str(), BI, bbIdx); 
      node->setDatatype((Datatype) int32);
      myDFG->insert_Node(node);
      return true; 

    case Instruction::UIToFP:
    case Instruction::SIToFP:
      node = new NODE(add, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype((Datatype) float32);
      myDFG->insert_Node(node);
      return true;

    case Instruction::IntToPtr:
    case Instruction::PtrToInt:
    case Instruction::BitCast:
      node = new NODE(add, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      // llvm document: It is always a no-op cast becasue
      // no bits change.
      return true;
    
    // Other instructions...
    case Instruction::ICmp:
    case Instruction::FCmp:
      if (dt == character) {
        Value *v = dyn_cast<Value>(BI->getOperand(0));
        Type* T = v->getType();
        int width = T->getPrimitiveSizeInBits()/8; 
        if (T->isIntegerTy()) {
          if (width == 1)
            dt = character;
          else if (width == 2)
            dt = int16;
          else
            dt = int32;
        } else if (T->isFloatingPointTy()) {
          if (T->isDoubleTy())
            dt = float64;
          else if (width == 2)
            dt = float32;
          else
            dt = float16;
        }
      }

      llvm::Instruction_Operation op;
      switch (cast<CmpInst>(BI)->getPredicate()) {
        case llvm::CmpInst::FCMP_OEQ:
        case llvm::CmpInst::FCMP_UEQ:
        case llvm::CmpInst::ICMP_EQ:
          op = cmpEQ;
          break;

        case llvm::CmpInst::FCMP_ONE:
        case llvm::CmpInst::FCMP_UNE:
        case llvm::CmpInst::ICMP_NE:
          op = cmpNEQ;
          break;

        case llvm::CmpInst::FCMP_OGT:
        case llvm::CmpInst::FCMP_UGT:
        case llvm::CmpInst::ICMP_SGT:
          op = cmpSGT;
          break;

        case llvm::CmpInst::ICMP_UGT:
          op = cmpUGT;
          break;

        case llvm::CmpInst::FCMP_OGE:
        case llvm::CmpInst::FCMP_UGE:
        case llvm::CmpInst::ICMP_SGE:
          op = cmpSGEQ;
          break;

        case llvm::CmpInst::ICMP_UGE:
          op = cmpUGEQ;
          break;

        case llvm::CmpInst::FCMP_OLT:
        case llvm::CmpInst::FCMP_ULT:
        case llvm::CmpInst::ICMP_SLT:
          op = cmpSLT;
          break;

        case llvm::CmpInst::ICMP_ULT:
          op = cmpULT;
          break;

        case llvm::CmpInst::FCMP_OLE:
        case llvm::CmpInst::FCMP_ULE:
        case llvm::CmpInst::ICMP_SLE:
          op = cmpSLEQ;
          break;

        case llvm::CmpInst::ICMP_ULE:
          op = cmpULEQ;
          break;

        default:
          DEBUG("not recognized CMP\n");
          return false;
      }

      node = new NODE(op, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    case Instruction::PHI: {
      //TODO Optimize Phi's here
      /* Phi's are generated to select inputs from multiple blocks.
          We can treat a Phi node (in loopHeader block) as a special case;
          select live-in value for the first time, and recurrent value otherwise
          If Phi is not in loopHeader block, treat it as a select instruction
      */
      DEBUG("We are here for PHI\n");
      node = new NODE(cgra_select, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;
    }
    case Instruction::Select:
      node = new NODE(cond_select, 1, nodeID++, BI->getName().str(), BI, bbIdx);
      node->setDatatype(dt);
      myDFG->insert_Node(node);
      return true;

    // Vector Operations
    case Instruction::ExtractElement:
    case Instruction::InsertElement:
    case Instruction::ShuffleVector:
      DEBUG("\n\nVector Instruction Detected!\n\n");
      return false;

    // Aggregate Operations
    case Instruction::ExtractValue:
    case Instruction::InsertValue:
      DEBUG("\n\nAggregate Instruction Detected!\n\n");
      return false;

    // Other Operations
    case Instruction::Call:
      DEBUG("\n\nCall Detected!\n\n");
      return false;
    case Instruction::VAArg:
      errs() << "\n\nva_arg Detected!\n\n";
      return false;
    case Instruction::LandingPad:
      errs() << "\n\nLandingPad Detected!\n\n";
      return false;
    case Instruction::CatchPad:
      errs() << "\n\nCatchPad Detected!\n\n";
      return false;
    case Instruction::CleanupPad:
      errs() << "\n\nCleanupPad Detected!\n\n";
      return false;

    default:
      DEBUG("\n\nUndefined Instruction Detected\n\n");
      return false;
  }
}


void
MultiDDGGen::resetVariables()
{
  mapLiveinValueName.clear();
  mapLiveinNameAlignment.clear();
  mapLiveinNameDatatype.clear();
  mapLiveoutInstName.clear();
  mapBrIdPaths.clear();
  
  nodeID = 0;
  edgeID = 0;

  loadBlock = nullptr;
}


bool
MultiDDGGen::updateLiveInVariables(Instruction *BI, std::vector<BasicBlock *> bbs, BasicBlock *Preheader)
{
  Module* M = Preheader->getParent()->getParent();
  bool irChanged = false;
  for (Use &U : BI->operands()) {  // U is of type Use &
    Value *v = U.get();
    Argument *arg = dyn_cast<Argument>(v);
    auto Inst = dyn_cast<Instruction>(v);
    if (Inst) {
      // an instruction defines V
      // Ensure that the definition is outside of the BBs of loop
      if (std::find(bbs.begin(), bbs.end(), Inst->getParent()) != bbs.end())
        continue;
    } else if (arg) {
      // an argument defines v
      Function *parentFunc = arg->getParent();
      if (parentFunc) {
        Inst = Preheader->getTerminator();
      } else
        continue;
    } else
      continue;

    // make sure the global value is not already present
    GlobalValue* G = dyn_cast<llvm::GlobalValue>(v);
    if (G != NULL) {
      continue;
    }

    std::string gPtrName;

    // make sure the livein not already created
    std::map<Value *, std::string>::iterator it;
    it = mapLiveinValueName.find(v);
    if (it != mapLiveinValueName.end())
      continue;

    llvm::Type* T = v->getType();
    GlobalVariable *gPtr;
    unsigned alignment = 4;
    Datatype dt;
    unsigned size;

    if (T->isIntegerTy() || T->isFloatingPointTy()) {
      gPtrName = "gVar" + std::to_string(++gVarNo);
      DEBUG("Load In is a variable created as " << gPtrName << "\n");


      // Create new global pointer
      gPtr = new GlobalVariable(*M, T, false, GlobalValue::CommonLinkage, 0, gPtrName);
      gPtr->setInitializer(Constant::getNullValue(T));
      irChanged = true;

      if (T->getPrimitiveSizeInBits() % 8 == 0)
        alignment = (T->getPrimitiveSizeInBits()) / 8;
      else
        alignment = T->getPrimitiveSizeInBits() % 8;

      std::tie(dt, size) = getDatatype(T, alignment);
    } else if (T->isPointerTy()) {
      gPtrName = "gPtr" + std::to_string(++gPtrNo);
      PointerType* PT = dyn_cast<PointerType>(T);
      DEBUG("Load In is a pointer created as " << gPtrName << "\n");

      int bit_width;
      if (PT != nullptr) {
        if (PT->getPrimitiveSizeInBits() % 8 == 0)
          bit_width = (PT->getElementType()->getPrimitiveSizeInBits())/8;
        else
          bit_width = PT->getElementType()->getPrimitiveSizeInBits() %8;
        DEBUG("passed bit width: " << bit_width << "\n");
        if (bit_width == 4) {
          if (PT->getElementType()->isFloatingPointTy())
            dt = float32;
          else
            dt = int32;
        } else if (bit_width == 8)
          dt = float64;
        else if(bit_width < 4)
          dt = character;
        else {
          DEBUG("unknown bit width: " << bit_width << ", defaulting data type to int32\n");
          dt = int32;
        }
      } else {
        DEBUG("Casting type of live in to a pointer failed\n");
        DEBUG("T->getPrimitiveSizeInBits(): " << T->getPrimitiveSizeInBits() << "\n");

        if (T->getPrimitiveSizeInBits() % 8 == 0)
          bit_width = (T->getPrimitiveSizeInBits())/8;
        else
          bit_width = T->getPrimitiveSizeInBits() % 8;
        if (bit_width == 4) {
          if (T->isIntegerTy())
            dt = int32;
          else
            dt = float32;
        } else if (bit_width == 8)
          dt = float64;
        else if (bit_width < 4)
          dt = character;
        else {
          DEBUG("unknown bit width: " << bit_width << ", defaulting data type to int32\n");
          dt = int32;
        }
      }

      gPtr = new GlobalVariable(*M, T, false, GlobalValue::CommonLinkage, 0, gPtrName);
      gPtr->setInitializer(ConstantPointerNull::get(PT));
      irChanged = true;
      /* Assumption is that Pointers will occupy 4 bytes for 32-bit system */
      alignment = T->getPointerAddressSpace();
      if (alignment < 4) 
        alignment = 4;
    } else {
      DEBUG("Type not a variable or a pointer, most likely will cause some problem\n");
      continue;
    }

    if (alignment == 0)
      DEBUG("Cannot find size of the memory access for live-in variable\n");
    gPtr->setUnnamedAddr(GlobalValue::UnnamedAddr::Local);
    
    // get the terminator instruction 
    // for the new store of the global val to be inserted before
    auto *TI = Inst->getParent()->getTerminator();
    // add in the new store inst
    StoreInst *newStoreInst = new StoreInst(v,gPtr,TI);
    irChanged = true;

    // Load value back from Global Pointer Inside Loop Block
    mapLiveinValueName[v] = gPtrName; 
    mapLiveinNameAlignment[gPtrName] = alignment;
    mapLiveinNameDatatype[gPtrName] = dt;
  }
  return irChanged;
}


void 
MultiDDGGen::updateDataDependencies(Instruction *BI, DFG* loopDFG, Loop* L, DominatorTree *DT, unsigned bbIdx)
{
  DEBUG("DEBUG -- From Update_dependency\n");
  DEBUG("Instruction: " << *BI << "\n");
  // basic blocks of the loop
  std::vector<BasicBlock *> bbs = L->getBlocks();
  // latch of the loop
  BasicBlock* loopLatch = L->getLoopLatch();
  // header of the loop
  BasicBlock* loopHeader = L->getHeader();

  // the node arc is from
  NODE *nodeFrom;
  // the node arc is to
  NODE *nodeTo;

  DataDepType dep = TrueDep;

  // first deal with casting operations
  switch (BI->getOpcode()) {
    case Instruction::FPToUI:
    case Instruction::FPToSI: 
      nodeTo = loopDFG->get_Node(BI);
      nodeFrom = new NODE(constant, 1, nodeID++, "ConstInt0", NULL, bbIdx);
      nodeFrom->setDatatype((Datatype) int32);
      loopDFG->insert_Node(nodeFrom);
      loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, 1);
      break;
    
    case Instruction::UIToFP:
    case Instruction::SIToFP:
      nodeTo = loopDFG->get_Node(BI);
      nodeFrom = new NODE(constant, 1, nodeID++, "ConstFP0", NULL, bbIdx);
      nodeFrom->setDatatype((Datatype) float32);
      loopDFG->insert_Node(nodeFrom);
      loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, 1);
      break;

    case Instruction::Trunc: {
      Type *T = (dyn_cast<TruncInst>(BI))->getDestTy();
      unsigned mask = (1 << T->getPrimitiveSizeInBits()) - 1;
      if (mask == 0 || mask > (0xffffffffUL))
        mask = (1 << 32) - 1; // cap to 32 bits
      nodeTo = loopDFG->get_Node(BI);
      nodeFrom = new NODE(constant, 1, nodeID++, "ConstInt"+std::to_string(mask), NULL, bbIdx);
      loopDFG->insert_Node(nodeFrom);
      nodeFrom->setDatatype(nodeTo->getDatatype()); 
      loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, 1);
      break;
    }
    case Instruction::FPTrunc: {
      Type *T = (dyn_cast<FPTruncInst>(BI))->getDestTy();
      unsigned mask = (1 << T->getPrimitiveSizeInBits()) - 1;
      if (mask == 0 || mask > (0xffffffffUL))
        mask = (1 << 32) - 1; // cap to 32 bits
      nodeTo = loopDFG->get_Node(BI);
      nodeFrom = new NODE(constant, 1, nodeID++, "ConstFP"+std::to_string(mask), NULL, bbIdx);
      loopDFG->insert_Node(nodeFrom);
      nodeFrom->setDatatype(nodeTo->getDatatype()); 
      loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, 1);
      break;
    }
    case Instruction::BitCast:
      nodeTo = loopDFG->get_Node(BI);
      if (nodeTo->getDatatype() == character || nodeTo->getDatatype() == int16 || nodeTo->getDatatype() == int32) {
        nodeFrom = new NODE(constant, 1, nodeID++, "ConstInt0", NULL, bbIdx);
        loopDFG->insert_Node(nodeFrom);
        loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, 1);
      } else {
        nodeFrom = new NODE(constant, 1, nodeID++, "ConstFP0", NULL, bbIdx);
        loopDFG->insert_Node(nodeFrom);
        loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, 1);
      }
      break;
    case Instruction::SExt: {
      DEBUG("SExt\n");
      nodeTo = loopDFG->get_Node(BI);
      Value *v = dyn_cast<Value>(BI->getOperand(0));
      Type* T = v->getType();
      unsigned mask = T->getPrimitiveSizeInBits();
      DEBUG("  Mask: " << mask << "\n");
      nodeFrom = new NODE(constant, 1, nodeID++, "ConstInt"+std::to_string(mask), NULL, bbIdx);
      nodeFrom->setDatatype(nodeTo->getDatatype());
      loopDFG->insert_Node(nodeFrom);
      loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, 1);
      break;
    }
    case Instruction::ZExt: {
      Value *v = dyn_cast<Value>(BI->getOperand(0));
      Type* T = v->getType();

      unsigned mask = (1 << T->getPrimitiveSizeInBits()) - 1;
      if(mask == 0 || mask > (0xffffffffUL)) 
        mask = (1 << 32) - 1;

      nodeTo = loopDFG->get_Node(BI);
      nodeFrom = new NODE(constant, 1, nodeID++, "ConstInt"+std::to_string(mask), NULL, bbIdx);
      nodeFrom->setDatatype(nodeTo->getDatatype());
      loopDFG->insert_Node(nodeFrom);
      loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, 1);
      break;
    }
    case Instruction::FPExt: {
      Value *v = dyn_cast<Value>(BI->getOperand(0));
      Type* T = v->getType();

      unsigned mask = (1 << T->getPrimitiveSizeInBits()) - 1;
      if(mask == 0 || mask > (0xffffffffUL)) 
        mask = (1 << 32) - 1;

      nodeTo = loopDFG->get_Node(BI);
      nodeFrom = new NODE(constant, 1, nodeID++, "ConstFP"+std::to_string(mask), NULL, bbIdx);
      nodeFrom->setDatatype(nodeTo->getDatatype());
      loopDFG->insert_Node(nodeFrom);
      loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, 1);
      break;
    }
  }

  // now for other instructions, iterate through operands
  for (unsigned int j = 0; j < BI->getNumOperands(); j++) {
    Value *operandVal = BI->getOperand(j);
    DEBUG(" j: " << j << "\n");
    DEBUG(" val_id: " << operandVal->getValueID() << "\n");
    
    //constant values can be immediate
    //if it is greater than immediate field; instruction generation should treat it as nonrecurring value
    if (operandVal->getValueID() == llvm::Value::ConstantIntVal) {
      DEBUG("  is constintval\n");
      int constVal = 0;
      if (dyn_cast<llvm::ConstantInt>(operandVal)->getBitWidth() > 1)
        constVal = dyn_cast<llvm::ConstantInt>(operandVal)->getSExtValue();
      else
        constVal = dyn_cast<llvm::ConstantInt>(operandVal)->getZExtValue();
      DEBUG("  Node name: " << "ConstInt"+std::to_string(constVal) << "\n");
      Datatype dt;
      Value *v = dyn_cast<Value>(BI);
      Type* T = v->getType();
      int bit_width = T->getPrimitiveSizeInBits()/8;
      if (bit_width == 1)
        dt = character;
      else if (bit_width == 2)
        dt = int16;
      else
        dt = int32;
      nodeFrom = new NODE(constant, 1, nodeID++, "ConstInt"+std::to_string(constVal), operandVal, bbIdx);
      DEBUG("  inserted new constant int id: " << nodeFrom->get_ID() <<"\n");
      nodeFrom->setDatatype(dt);
      loopDFG->insert_Node(nodeFrom);
      nodeFrom->setDatatype(dt); 
      nodeTo = loopDFG->get_Node(BI);
      // since for store there are 2 nodes, the arc is skipped to later
      if (BI->getOpcode() != Instruction::Store)
        loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep, j);
      continue;
    } else if (operandVal->getValueID() == llvm::Value::ConstantFPVal) {
      DEBUG("  in constFPval\n");
      //We support only single precision fp. But sometimes IR converts the float to doubles.
      // SO we need to get the appropriate FP val.
      Datatype dt;
      Value *v = dyn_cast<Value>(BI);
      Type* T = v->getType();
      std::ostringstream constVal;
      int bit_width = T->getPrimitiveSizeInBits()/8;
      if(bit_width == 8)
        dt = float64;
      else if (bit_width == 2)
        dt = float16;
      else if (bit_width == 4)
        dt = float32;

      if (dt == float64)
        constVal << cast<ConstantFP>(operandVal)->getValueAPF().convertToDouble();
      else
        constVal << cast<ConstantFP>(operandVal)->getValueAPF().convertToFloat();
      nodeFrom = new NODE(constant, 1, nodeID++, "ConstFP" + constVal.str(), operandVal, bbIdx);
      nodeFrom->setDatatype(dt);
      loopDFG->insert_Node(nodeFrom);
      nodeFrom->setDatatype(dt);
      nodeTo = loopDFG->get_Node(BI);
      // since for store there are 2 nodes, the arc is skipped to later
      if (BI->getOpcode() != Instruction::Store)
        loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, 0, TrueDep,j);
      continue;
    } else if (operandVal->getValueID() == llvm::Value::BasicBlockVal) {
      DEBUG("  !!BasicBlockVal Detected!! Not considered yet\\n");
      DEBUG("  intr: " << *BI << "\n");
      DEBUG("  val: " <<  operandVal << "\n");
      //BasicBlock *bb = dyn_cast<llvm::BasicBlock>(operand); 
    } else if ((operandVal->getValueID() >= llvm::Value::InstructionVal) ||
              (operandVal->getValueID() == llvm::Value::GlobalVariableVal) ||
              (operandVal->getValueID() == llvm::Value::ArgumentVal)) {

      DEBUG("  Inside Not Constant\n");
      if (operandVal->getValueID() == llvm::Value::GlobalVariableVal) {
        if (BI->getOpcode() != Instruction::Load && BI->getOpcode() != Instruction::Store)
          continue;
      }
      std::string operandName;
      operandName = cast<Instruction>(operandVal)->getName().str();
      int distance = 0;
      Datatype dt; 
      DEBUG("   name: " << operandName << "\n"); 

      // first for the operand, if it is a livein/globalVar
      // need to populate the livein node and edge file
      // or if not, need its distance to the instruction
      if (loopDFG->get_Node(operandVal) == NULL) {
        // create the operand node if it's not present
        DEBUG("   inside getoperand null\n");
        DEBUG("    BI: " << *BI << "\n"); 
        DEBUG("    operand: " << operandVal->getValueID() << "\n");
        DEBUG("    value name: " << operandVal->getName().str() << "\n");
        std::map<Value *,std::string>::iterator it;
        it = mapLiveinValueName.find(operandVal);
        if ((it != mapLiveinValueName.end()) ||
            (operandVal->getValueID() == llvm::Value::GlobalVariableVal)) {
          DEBUG("    is a livein or global val\n");
          std::string ptrName;
          unsigned alignment = 4;
          if (it != mapLiveinValueName.end()) {
            ptrName = it->second;
            alignment = mapLiveinNameAlignment[ptrName];
            dt = mapLiveinNameDatatype[ptrName];
            DEBUG("      livein already defined with ptr name: " << ptrName << "\n");
            DEBUG("     dt: " << dt << "\n");
          } else {
            DEBUG("    global value not in livein yet: " << operandName << "\n");
            ptrName = operandName;
          }
          nodeFrom = new NODE(constant, 1, nodeID++, ptrName, operandVal, bbIdx);
          nodeFrom->setAlignment(alignment);
          nodeTo = loopDFG->get_Node(BI); 
          loopDFG->insert_Node(nodeFrom);
          unsigned int livein_datatype = (unsigned int)(nodeTo->getDatatype());
          dep = LiveInDataDep;
          unsigned int CGRA_loadAddID = ((nodeID++) + 100);
          unsigned int CGRA_loadDataID = ((nodeID++) + 100);
          // Update LiveIn information
          liveInNodefile << nodeTo->get_ID() << "\t" << nodeTo->get_Instruction() << "\t" << nodeTo->get_Name() << "\t" << livein_datatype << "\n";
          liveInNodefile << nodeFrom->get_ID() << "\t" << constant << "\t" << ptrName << "\t" << livein_datatype << "\n";
          liveInNodefile << CGRA_loadAddID << "\t" << ld_add << "\t" << "ld_add_" + ptrName << "\t" << livein_datatype << "\n";
          liveInNodefile << CGRA_loadDataID << "\t" << ld_data << "\t" << "ld_data_" + ptrName << "\t" << livein_datatype <<"\n";

          liveInEdgefile << CGRA_loadAddID << "\t" << CGRA_loadDataID << "\t0\tLRE\t0\n";
          liveInEdgefile << CGRA_loadDataID << "\t" << nodeTo->get_ID() << "\t0\tTRU\t" << j << "\n";
          liveInEdgefile << nodeFrom->get_ID() << "\t" << CGRA_loadAddID << "\t0\tTRU\t0\n";
        } else { // end of in livein map or already global variable
          // node not present, not added as a livin variable during update livein
          // and not already a global variable
          DEBUG("   !Cannot detect defining instruction for the live-in\n\n");
          exit(1);
        }
      } else { //end of loopDFG->get_Node(operandVal) == NULL
        DEBUG("   operand already exist as a node\n");
        std::map<Value *,std::string>::iterator it;
        it = mapLiveinValueName.find(operandVal);
        if ((it != mapLiveinValueName.end()) ||
            (operandVal->getValueID() == llvm::Value::GlobalVariableVal)) {
          DEBUG("    inside if for livein or globalvariable\n");
          nodeFrom = loopDFG->get_Node(operandVal);
          nodeTo = loopDFG->get_Node(BI);
          std::string ptrName = nodeFrom->get_Name();
          unsigned int livein_datatype = (unsigned int)(nodeTo->getDatatype());
          dep = LiveInDataDep;
          distance = 0;
          unsigned int CGRA_loadAddID = ((nodeID++)+100);
          unsigned int CGRA_loadDataID = ((nodeID++)+100);

          liveInNodefile << nodeTo->get_ID() << "\t" << nodeTo->get_Instruction() << "\t" << nodeTo->get_Name() << "\t" << livein_datatype << "\n";
          liveInNodefile << nodeFrom->get_ID() << "\t" << constant << "\t" << ptrName << "\t" << livein_datatype << "\n";
          liveInNodefile << CGRA_loadAddID << "\t" << ld_add << "\t" << "ld_add_" + ptrName << "\t" << livein_datatype << "\n";
          liveInNodefile << CGRA_loadDataID << "\t" << ld_data << "\t" << "ld_data_" + ptrName << "\t" << livein_datatype <<"\n";

          liveInEdgefile << CGRA_loadAddID << "\t" << CGRA_loadDataID << "\t0\tLRE\t0\n";
          liveInEdgefile << CGRA_loadDataID << "\t" << nodeTo->get_ID() << "\t0\tTRU\t" << j << "\n";
          liveInEdgefile << nodeFrom->get_ID() << "\t" << CGRA_loadAddID << "\t0\tTRU\t0\n";
        } else // end of is livein or global var
          distance = getDistance(cast<Instruction>(operandVal), BI, bbs,loopLatch);
      }

      // here for nodeFrom of the arc, generally the operand node
      if (cast<Instruction>(operandVal)->getOpcode() == Instruction::Load) {
        //if the operand instruction corresponds to a load operation
        DEBUG("  operand ins is load\n"); 
        nodeFrom = loopDFG->get_Node_Mem_Data(cast<Instruction>(operandVal));
        if (nodeFrom == NULL) {
          nodeFrom = loopDFG->get_Node(operandVal);
        }
      } else { 
        DEBUG("  operand not load\n");
        nodeFrom = loopDFG->get_Node(operandVal);
      }

      // here for nodeTo of the arc, generally the BI node
      if (BI->getOpcode() == Instruction::Store) {
        // if its a store instruction, in the DFG, a store has 2 nodes
        // one for address, one for data, with the same ins
        DEBUG("  node ins is a store\n");
        NODE* nodeStoreFrom;
        NODE* nodeStoreTo;
        if ((dyn_cast<StoreInst>(BI))->getPointerOperand() == operandVal) {
          // operand the store pointer, need to add arc for data
          DEBUG("   operand is the pointer operand\n");
          nodeTo = loopDFG->get_Node_Mem_Add(BI);
          DEBUG("   passed nodeTo: " << nodeTo->get_Name() << "\n");
          DEBUG("   value id for store:" << (dyn_cast<StoreInst>(BI))->getValueOperand()->getValueID() << "\n"); 
          // if the other operand of the store is a constant, the arc is skipped earlier to here
          if (((dyn_cast<StoreInst>(BI))->getValueOperand()->getValueID() == llvm::Value::ConstantIntVal) || 
              ((dyn_cast<StoreInst>(BI))->getValueOperand()->getValueID() == llvm::Value::ConstantFPVal)) {
            DEBUG("    store value is constant\n");
            nodeStoreFrom = loopDFG->get_Node((dyn_cast<StoreInst>(BI))->getValueOperand());
            nodeStoreTo = loopDFG->get_Node_Mem_Data(BI);
            DEBUG("    data nodeStoreFrom: " << nodeStoreFrom->get_Name());
            DEBUG(" -> data nodeStoreTo: " << nodeStoreTo->get_Name() << "\n");
            // arc for store data
            loopDFG->make_Arc(nodeStoreFrom, nodeStoreTo, edgeID++, 0, TrueDep,0);
          }
        } else {
          // operand the store value, need to add arc for address
          DEBUG("   operand is the value operand\n");
          nodeTo = loopDFG->get_Node_Mem_Data(BI);
          DEBUG("    passed nodeTo: " << nodeTo->get_Name() << "\n");
          DEBUG("    pointer id for store:" << (dyn_cast<StoreInst>(BI))->getPointerOperand()->getValueID() << "\n");
          // if the other operand of the store is a constant, the arc is skipped earlier to here
          if (((dyn_cast<StoreInst>(BI))->getPointerOperand()->getValueID() == llvm::Value::ConstantIntVal) || 
              ((dyn_cast<StoreInst>(BI))->getPointerOperand()->getValueID() == llvm::Value::ConstantFPVal)) {
            // pointer value is constant    
            DEBUG("    pointer value is constant\n");
            nodeStoreFrom = loopDFG->get_Node((dyn_cast<StoreInst>(BI))->getPointerOperand());
            nodeStoreTo = loopDFG->get_Node_Mem_Add(BI);
            DEBUG("    address nodeStoreFrom: " << nodeStoreFrom->get_Name());
            DEBUG(" -> address nodeStoreTo: " << nodeStoreTo->get_Name() << "\n");
            // arc for store address
            loopDFG->make_Arc(nodeStoreFrom, nodeStoreTo, edgeID++, 0, TrueDep,0);
          }
        }
      } else if (BI->getOpcode() == Instruction::Load) {
        DEBUG("  instruction is a load\n"); 
        nodeTo = loopDFG->get_Node_Mem_Add(BI);
      } else {
        DEBUG("  ins not load or store\n");
        nodeTo = loopDFG->get_Node(BI);
      }

      DEBUG("  nodeFrom: " << nodeFrom->get_Name() << " - nodeTo: " << nodeTo->get_Name() << "\n");
      loopDFG->make_Arc(nodeFrom, nodeTo, edgeID++, distance,dep,j);
    } // end of value not a constant
  } // end of iterating through operands

  // fix op orders of sel
  if (BI->getOpcode() == Instruction::Select) {
    DEBUG(" Fixing op order of select\n"); 
    DEBUG("  Select node: " << loopDFG->get_Node(BI)->get_ID() << "\n");
    NODE* nodeSel = loopDFG->get_Node(BI);
    for (NODE* nodeOp : nodeSel->Get_Prev_Nodes()) {
      if (nodeOp->get_LLVM_Instruction() == (dyn_cast<SelectInst>(BI))->getCondition()) {
        // cond op
        DEBUG("  cond op is " << nodeOp->get_ID() << "\n");
        ARC* arcOp = loopDFG->get_Arc(nodeOp, nodeSel);
        arcOp->SetOperandOrder(2);
        arcOp->Set_Dependency_Type(PredDep);
      } else if (nodeOp->get_LLVM_Instruction() == (dyn_cast<SelectInst>(BI))->getTrueValue()) {
        // true op
        DEBUG("  true op is " << nodeOp->get_ID() << "\n");
        if (nodeOp->is_Load_Address_Generator())
          nodeOp = nodeOp->get_Related_Node();  
        ARC* arcOp = loopDFG->get_Arc(nodeOp, nodeSel);
        arcOp->SetOperandOrder(0);
      } else if (nodeOp->get_LLVM_Instruction() == (dyn_cast<SelectInst>(BI))->getFalseValue()) {
        // false op
        DEBUG("  false op is " << nodeOp->get_ID() << "\n");
        if (nodeOp->is_Load_Address_Generator())
          nodeOp = nodeOp->get_Related_Node();  
        ARC* arcOp = loopDFG->get_Arc(nodeOp, nodeSel);
        arcOp->SetOperandOrder(1);
      }
    }
  }

  // for get element pointer instructions
  // in the DFG, GEP is currently represented as an add
  // this whole process is kind of like calculating the offset
  if (BI->getOpcode() == Instruction::GetElementPtr) {
    DEBUG(" instruction is getelementptr\n");
    // the gep inst node
    NODE* nodeGEP = loopDFG->get_Node(BI);
    // node for operand 0, the base pointer
    NODE* nodeBase;
    // type of the current indexed element
    Type* elementType;
    // the index if it is a constant
    unsigned constIndex;

    int offsetConst = 0;
    std::vector <NODE*> offsetNodes;

    // big overhaul from the original to not use a map and store the struct
    // sizes, maybe not as optimized, but easier to implement
    for (unsigned int i = 0; i < BI->getNumOperands(); ++i) {
      NODE* nodeOp;
      Value *operandVal = BI->getOperand(i);
      std::string operandName = operandVal->getName().str();
      Datatype dt = nodeGEP->getDatatype();
      DEBUG("  Operand " << i << " - " << operandVal << " - name: " << operandName << "\n");
      //Add Node If Unnamed Private
      if (!operandName.empty()) {
        nodeOp = loopDFG->get_Node(operandName);
        if (nodeOp == NULL) {
          nodeOp = new NODE(constant, 1, nodeID++, operandName, NULL, bbIdx);
          nodeOp->setDatatype(dt);
          loopDFG->insert_Node(nodeOp);
          loopDFG->make_Arc(nodeOp, nodeGEP, edgeID++, 0, TrueDep, i);
          DEBUG("  Uncertain behavior: adding nodeOp: " << operandName << " - nodeGEP: " << nodeTo->get_Name() << "\n");
        }
      } else {
        nodeOp = loopDFG->get_Node(operandVal);
      }
      if (nodeOp->is_Load_Address_Generator())
        nodeOp = nodeOp->get_Related_Node();
      DEBUG("  nodeOp: " << nodeOp->get_Name() << ", id: " << nodeOp->get_ID() <<"\n");

      // if this operand of the GEP is multiply
      // set the order of constant and variable multipliers
      // not sure why this is done, but there must be reasons I suppose
      if (nodeOp->get_Instruction() == mult) {
        std::vector<NODE*> prevNodes = nodeOp->Get_Prev_Nodes();
        for (unsigned int n = 0; n < prevNodes.size(); n++) {
          if (prevNodes[n]->get_Name().find("ConstInt") != std::string::npos) {
            // this previous node is const int
            ARC *arcMult = loopDFG->get_Arc(prevNodes[n], nodeOp);
            if (arcMult != NULL)
              arcMult->SetOperandOrder(1);
          } else {
            // this previous node not const int
            ARC* arcMult = loopDFG->get_Arc(prevNodes[n], nodeOp);
            if (arcMult != NULL)
              arcMult->SetOperandOrder(0);
          }
        }
      }

      // const or mul node, for the calculation of the current index offset
      NODE* nodeOffset;
      if (i == 0) {
        // operand 0 of GEP ins is the base pointer
        DEBUG("For the base pointer\n");
        elementType = operandVal->getType();
      } else { 
        // other operands are the indices
        DEBUG("For index " << i - 1 << "\n");

        // first get the type of the current indexed element should be on
        // using the previous type
        Type* nextType;
        if (elementType->isPointerTy()) {
          DEBUG("previous type pointer\n");
          nextType = dyn_cast<PointerType>(elementType)->getPointerElementType();
          // pointers can only be the first, per refrence
          if (i != 1) {
            DEBUG("ERROR! pointer in the middle\n");
            exit(1);
          }
        } else if (elementType->isArrayTy()) {
          DEBUG("previous type array\n");
          nextType = dyn_cast<ArrayType>(elementType)->getArrayElementType();
        } else if (elementType->isStructTy()) {
          DEBUG("previous type struct\n");
          // for accessing a struct, the index should be a constant
          nextType = dyn_cast<StructType>(elementType)->getStructElementType(constIndex);
        } else {
          DEBUG("ERROR! most likely vector\n");
          exit(1);
        }

        // now for the actual index itself, first prepare the offsets
        if (nodeOp->get_Instruction() == constant) {
          // first for constants
          // a fix for multiple constants with the same value
          for (auto nodeOpNew : nodeGEP->Get_Prev_Nodes()) {
            if (loopDFG->get_Arc(nodeOpNew, nodeGEP)->GetOperandOrder() == i) {
              if (nodeOpNew->get_LLVM_Instruction() == nodeOp->get_LLVM_Instruction()) {
                DEBUG("replacing nodeOp:" << nodeOp->get_ID() << " with " << nodeOpNew->get_ID() << "\n");
                nodeOp = nodeOpNew;
              }
              else {
                DEBUG("ERROR!! replacing constants in GEP, values of the constants does not match\n");
                exit(1);
              }
            }
          }
          if (nodeOp->get_Name() == "ConstInt0") {
            // for const 0, the node can be deleted
            DEBUG("this index is constant 0, nodeid:"<< nodeOp->get_ID() << "\n");
            constIndex = 0;
            // remove this arc
            loopDFG->Remove_Arc(nodeOp, nodeGEP);
            // delete the constant node 
            loopDFG->delete_Node(nodeOp);
          } else {
            // other constants
            // first get the constant
            DEBUG("constant index\n");
            std::string constName = nodeOp->get_Name();
            char* tempStr = new char[constName.length()+1];
            strcpy(tempStr,constName.c_str());
            sscanf(tempStr, "%*[^-0123456789]%d",&constIndex);

            // now calculate the offset
            // can be pointer, array or struct
            unsigned offset = 0;
            Type* nextType;
            if (elementType->isPointerTy()) {
              auto [dummyType, size] = getDatatype(nextType, nextType->getPrimitiveSizeInBits()/8);             
              offset = constIndex * size;
            } else if (elementType->isArrayTy()) {
              auto [dummyType, size] = getDatatype(nextType, nextType->getPrimitiveSizeInBits()/8);             
              offset = constIndex * size;
            } else if (elementType->isStructTy()) {
              for (unsigned jj = 0; jj < constIndex; jj++) {
                Type* typeIt = dyn_cast<StructType>(elementType)->getStructElementType(jj);
                auto [dummyType, size] = getDatatype(typeIt, typeIt->getPrimitiveSizeInBits()/8);
                offset += size;
              }
            } else{
              DEBUG("Error, vector most likely\n");
              exit(1);
            }
            DEBUG("current index: " << constIndex << "offset is " << offset << "\n");
            offsetConst += offset;
            // remove op to gep arc
            loopDFG->Remove_Arc(nodeOp, nodeGEP);
            // delete the constant node if no other refreces
            if (nodeOp->Get_Next_Nodes().size() == 0)
              loopDFG->delete_Node(nodeOp);   
          }         
        } else {  // end of if index constant
          // not a constant, this should only happen with array, or maybe pointer
          // calculate the offset
          DEBUG("dynamic index\n");
          // dynamic index shoud only be for array or pointer
          if (!(elementType->isPointerTy() || elementType->isArrayTy())) {
            DEBUG("ERROR, dynamic index should only be array or pointer\n");
            exit(1);
          }

          auto [dummyType, size] = getDatatype(nextType, nextType->getPrimitiveSizeInBits()/8); 
          DEBUG("dynamic index node : " << nodeOp->get_Name() << " with element size of " << size << "\n");
          // now the offset should be a mult node
          NODE* nodeSize = new NODE(constant, 1, nodeID++, "ConstInt"+std::to_string(size), NULL, bbIdx);
          DEBUG("inserted offset node id:" << nodeSize->get_ID() << ", size: " << size <<"\n");
          nodeSize->setDatatype(int32);
          loopDFG->insert_Node(nodeSize);
          nodeOffset = new NODE(mult, 1, nodeID++, std::to_string(nodeID-1), NULL, bbIdx);
          nodeOffset->setDatatype(int32);
          loopDFG->insert_Node(nodeOffset);
          loopDFG->make_Arc(nodeOp, nodeOffset, edgeID++, 0, TrueDep, 0);
          loopDFG->make_Arc(nodeSize, nodeOffset, edgeID++, 0, TrueDep, 1);
          // remove op to gep arc
          loopDFG->Remove_Arc(nodeOp, nodeGEP);
          offsetNodes.push_back(nodeOffset);
        }
        elementType = nextType;
      } // end of if this operand is index
    } // end of iterating through operands
    // now add up all the offset nodes
    // first for the constants
    if (offsetConst != 0) {
      NODE* nodeOffsetConst = new NODE(constant, 1, nodeID++, "ConstInt"+std::to_string(offsetConst), NULL, bbIdx);
      nodeOffsetConst->setDatatype(int32);
      loopDFG->insert_Node(nodeOffsetConst);
      offsetNodes.push_back(nodeOffsetConst);
    }
    #ifndef NDEBUG_M
      DEBUG("offsetNodes size: " << offsetNodes.size() << "\n");
      for (auto node : offsetNodes) {
        DEBUG(node->get_Name() << "\n");
      }
    #endif
    // just in case
    if (offsetNodes.size() == 0) {
      NODE* nodeOffsetConst = new NODE(constant, 1, nodeID++, "ConstInt"+std::to_string(0), NULL, bbIdx);
      nodeOffsetConst->setDatatype(int32);
      loopDFG->insert_Node(nodeOffsetConst);
      offsetNodes.push_back(nodeOffsetConst);
    }
    // add the nodes until only one left
    while (offsetNodes.size() > 1) {
      // get the first 2 nodes
      NODE* node0 = offsetNodes[0];
      NODE* node1 = offsetNodes[1];
      // for the new add node
      NODE* nodeAdd = new NODE(add, 1, nodeID++, std::to_string(nodeID-1), NULL, bbIdx);
      nodeAdd->setDatatype(int32);
      loopDFG->insert_Node(nodeAdd);
      loopDFG->make_Arc(node0, nodeAdd, edgeID++, 0, TrueDep, 0);
      loopDFG->make_Arc(node1, nodeAdd, edgeID++, 0, TrueDep, 1);
      offsetNodes.push_back(nodeAdd);
      // remove the added two nodes
      offsetNodes.erase(offsetNodes.begin(), offsetNodes.begin() + 2);
    }
    // add arc to gep
    loopDFG->make_Arc(offsetNodes[0], nodeGEP, edgeID++, 0, TrueDep, 1);
  } // end of if GEP

  //Fix Successor Operand Order
  if ((BI->getOpcode() == Instruction::Load) || (BI->getOpcode() == Instruction::Store)) {
    DEBUG(" Inside Load or Store\n");
    nodeTo = loopDFG->get_Node(BI);
    for (unsigned int i=0; i< BI->getNumOperands(); ++i) {
      Instruction *priorInst = (dyn_cast<Instruction>(BI->getOperand(i)));
      if (priorInst == NULL) 
        continue;
      if (priorInst->getOpcode() == Instruction::GetElementPtr) {
        nodeFrom = loopDFG->get_Node(BI->getOperand(i));
        ARC* arc = loopDFG->get_Arc(nodeFrom, nodeTo);
        if(arc != NULL)
          arc->SetOperandOrder(0);
      }
    }
  }

  // now for phi nodes
  if (BI->getOpcode() == Instruction::PHI) {
    DEBUG(" Instruction is PHI\n");
    NODE* nodePhi = loopDFG->get_Node(BI);

    // if the phi node is at loop header, since we are dealing with
    // a simplified loop with a preheader and a single backedge, no
    // need to act on the phi node
    if (BI->getParent() != loopHeader) {
      // not in loopheader
      DEBUG("phi inst not in loopheader\n");
      DEBUG("  node: " << nodePhi->get_ID() << "\n");
      // list of the basic blocks of the phi node
      std::vector<BasicBlock*> bbList;
      // list of the operands of the phi node
      std::vector<Instruction*> operandList;

      //Populate lists with the information from phi node
      for (unsigned int ii=0; ii < dyn_cast<PHINode>(BI)->getNumIncomingValues(); ii++) {
        bbList.push_back(dyn_cast<PHINode>(BI)->getIncomingBlock(ii));
        operandList.push_back(dyn_cast<Instruction>(BI->getOperand(ii)));
      } 
      #ifndef NDEBUG_M
        for (unsigned int ii=0; ii < (int) operandList.size(); ii++)
          DEBUG("  op nodes: " << loopDFG->get_Node(operandList[ii])->get_ID() << "\n");
        DEBUG("  BBLIST:\n\n"); 
        for (unsigned int ii=0; ii < (int) bbList.size(); ii++) {
          DEBUG("   ii: " << ii << "\n"); 
          for (BasicBlock::iterator BBBI = bbList[ii]->begin(); BBBI!= bbList[ii]->end(); ++BBBI)
            DEBUG("    " << *BBBI << "\n");
          DEBUG("\n"); 
        }
      #endif

      // now first we need to find the br instruction
      // only consider the phi node to have 2 operands for now
      if (bbList.size() != 2) {
        DEBUG("ERROR, the phi node have a incoming size of " << bbList.size());
        DEBUG("currently only considering size of 2\n");
        exit(1);
      }

      // find the common dominator of the 2 basic blocks
      BasicBlock* commonDom =  DT->findNearestCommonDominator(bbList[0], bbList[1]);
      if (!L->contains(commonDom)) {
        // this should not happen with a simplified loop
        DEBUG("ERROR! PHI node common domninator not in loop\n");
        exit(1);
      }

      // get the index for the common dominator
      int domIndex = 0;
      for (std::vector<BasicBlock*>::const_iterator it = bbs.begin() ; it != bbs.end(); ++it) {
        if (commonDom == (*it))
          break;
        else 
          domIndex++;
      }

      DEBUG("The common dominator is: " << commonDom->getName());
      DEBUG(" with id of " << domIndex << "\n");
      // the branching instruction causing this phi
      BranchInst* brInst;
      // cond instruction of this branching
      Instruction* condInst;
      // next, find the condition instruction of this branching
      for(BasicBlock::iterator BBI = commonDom->begin(); BBI !=commonDom->end(); ++BBI) {
        if (BBI->getOpcode() != Instruction::Br)
          continue;
        DEBUG("inside common Dom: " << (*BBI) << "\n");
        brInst = dyn_cast<llvm::BranchInst>(BBI);
        if (brInst->isUnconditional()) {
          DEBUG("ERROR! Unconditional branch at common dominator\n");
          exit(1);
        }
        condInst = dyn_cast<llvm::Instruction>(brInst->getCondition());
      }
      // now find the true cond node and false cond node
      NODE* trueNode;
      NODE* falseNode;
      // use edge here
      BasicBlockEdge* trueEdge = new BasicBlockEdge(commonDom, brInst->getSuccessor(0));
      BasicBlockEdge* falseEdge = new BasicBlockEdge(commonDom, brInst->getSuccessor(1));
      BasicBlockEdge* phiEdge0 = new BasicBlockEdge(bbList[0], BI->getParent());
      BasicBlockEdge* phiEdge1 = new BasicBlockEdge(bbList[1], BI->getParent());

      // for br inst, successor 0 is true path, successor 1 is false path
      if (DT->dominates(*trueEdge, *phiEdge0)) {
        if (DT->dominates(*falseEdge, *phiEdge1)) {
          trueNode = loopDFG->get_Node(operandList[0]);
          falseNode = loopDFG->get_Node(operandList[1]);
        } else {
          DEBUG("ERROR, block 0 is true, block 1 is not false\n");
          exit(1);
        }
      } else if (DT->dominates(*falseEdge, *phiEdge0)) {
        if (DT->dominates(*trueEdge, *phiEdge1)) {
          trueNode = loopDFG->get_Node(operandList[1]);
          falseNode = loopDFG->get_Node(operandList[0]);
        } else {
          DEBUG("ERROR, block 0 is false, block 1 is not true\n");
          exit(1);
        }
      } else {
        DEBUG("ERROR, block 0 neither true or false\n");
        exit(1);
      }
      // fix for address generator
      if (trueNode->is_Load_Address_Generator())
        trueNode = trueNode->get_Related_Node();
      if (falseNode->is_Load_Address_Generator())
        falseNode = falseNode->get_Related_Node();

      NODE* condNode = loopDFG->get_Node(condInst);
      if (condNode->is_Load_Address_Generator())
        condNode = condNode->get_Related_Node();
      // here we set the cond node to its corresponding br inst
      condNode->setCondBrId(domIndex);

      // now the 3 edges to a select
      // first fix the order of the 2 operands
      // true op
      ARC* arcOp = loopDFG->get_Arc(trueNode, nodePhi);
      if (arcOp == NULL)
        loopDFG->make_Arc(trueNode, nodePhi, edgeID++, 0, TrueDep, 0); 
      else
        arcOp->SetOperandOrder(0);
      // false op
      arcOp = loopDFG->get_Arc(falseNode, nodePhi);
      if (arcOp == NULL)
        loopDFG->make_Arc(falseNode, nodePhi, edgeID++, 0, TrueDep, 1); 
      else
        arcOp->SetOperandOrder(1);
      // cond
      loopDFG->make_Arc(condNode, nodePhi, edgeID++, 0, PredDep, 2);

      // set the branch idx of this phi node
      nodePhi->setBranchIndex(domIndex);
    } else { // end of BI not in loop header
      // for phi in loop header, all the incoming edges from within the loop should have a distance of 1
      // so, just in case distance is incorrect
      DEBUG("phi inst in loopheader\n");
      DEBUG("  node: " << nodePhi->get_ID() << "\n");
      // get all the incomming nodes
      nodePhi->setBranchIndex(-1);
      std::vector<NODE*> incomingNodes = nodePhi->Get_Prev_Nodes();
      for (int i = 0; i < (int)incomingNodes.size(); i++) {
        ARC* arcIn = loopDFG->get_Arc(incomingNodes[i], nodePhi); 
        if (arcIn->Get_Inter_Iteration_Distance() == 0 && incomingNodes[i]->get_Instruction() != constant) {
          DEBUG("    incoming node: " << incomingNodes[i]->get_ID() << " with wrong distance 0, set to 1\n");
          arcIn->Set_Inter_Iteration_Distance(1); 
        }
      }
    }
  } // end of phi node
  DEBUG("Done updating data dependecy\n");
}


bool
MultiDDGGen::updateLiveOutVariables(Instruction* BI, DFG* loopDFG, Loop* L, unsigned bbIdx)
{
  // get the module this loop is in
  Module* M = L->getLoopPreheader()->getParent()->getParent();
  bool irChanged = false;
  // basic blocks of the loop
  std::vector<BasicBlock *> bbs = L->getBlocks();

  // instructions with no uses (like stores)
  if (BI->use_empty())
    return false;

  // instructions with use only inside loop
  bool usedOutside = false;
  for (auto U : BI->users()) {  // U is of type User*
    if (auto Inst = dyn_cast<Instruction>(U)) {
      // an instruction uses Value
      // Ensure that the use is outside of the BBs of loop
      if (std::find(bbs.begin(), bbs.end(), Inst->getParent()) == bbs.end())
        usedOutside = true;
    }
  }
  if (!usedOutside) 
    return false; 
  
  // skip liveout values already added 
  if (mapLiveoutInstName.find(BI) != mapLiveoutInstName.end() ) {
    return false;
  }
  DEBUG("we are in liveout update\n");
  DEBUG("Ins: " << *BI << "\n");
  // for now, do not care if the value will be stored in a global variable beyond the loop  
  // treat the execution of the CGRA more atomically -> get livein, execute, store liveout
  
  // create the new global variable and its node in the DFG
  std::string gPtrName;
  Value *v = dyn_cast<Value>(BI);
  Type* T = v->getType();
  GlobalVariable *gPtr;
  unsigned alignment = 4;
  llvm::Datatype dt; 
  if (T->isIntegerTy() || T->isFloatingPointTy()) {
    gPtrName = "gVar" + std::to_string(++gVarNo);
    // Create new global pointer
    gPtr = new GlobalVariable(*M, T, false, GlobalValue::CommonLinkage, 0, gPtrName);
    Constant *constVal = Constant::getNullValue(T);
    gPtr->setInitializer(constVal);
    irChanged = true;
    DEBUG("inserget global variable " << gPtrName << "\n");
    if (T->getPrimitiveSizeInBits() % 8 == 0)
      alignment = T->getPrimitiveSizeInBits() / 8;
    else
      alignment = T->getPrimitiveSizeInBits() % 8;
    unsigned dummySize;
    std::tie(dt, dummySize) = getDatatype(T, alignment); 
  } else if (T->isPointerTy()) { 
    gPtrName = "gPtr" + std::to_string(++gPtrNo);
    // find the element type
    PointerType* PT = dyn_cast<PointerType>(T);
    int bit_width;
    if (PT != nullptr) {
      if (PT->getPrimitiveSizeInBits() % 8 == 0)
        bit_width = (PT->getElementType()->getPrimitiveSizeInBits()) / 8;
      else
        bit_width = PT->getElementType()->getPrimitiveSizeInBits() % 8;

      if (bit_width == 4) {
        if (PT->getElementType()->isFloatingPointTy())
          dt = float32;
        else
          dt = int32;  
      } else if (bit_width == 8)
        dt = float64;
      else if (bit_width < 4)
        dt = character;
    } else {
      if (T->getPrimitiveSizeInBits() % 8 == 0)
        bit_width = (T->getPrimitiveSizeInBits()) / 8;
      else
        bit_width = T->getPrimitiveSizeInBits() % 8;
      if (bit_width == 4) {
        if (T->isIntegerTy())
          dt = int32;
        else
          dt = float32;
      }
      else if (bit_width == 8)
        dt = float64;
      else if (bit_width < 4)
        dt = character;
    }
    gPtr = new GlobalVariable(*M, T, false, GlobalValue::CommonLinkage, 0, gPtrName);
    ConstantPointerNull* constPtr = ConstantPointerNull::get(PT);
    gPtr->setInitializer(constPtr);
    DEBUG("inserget global variable " << gPtrName << "\n");
    irChanged = true;
    alignment = T->getPointerAddressSpace();
    if (alignment < 4) 
      alignment = 4;
  } // end of is pointer ty
  DEBUG("  Aligment: " << alignment << ", node datatype: " << dt <<"\n");
  if (alignment == 0)
    DEBUG("WARNING: updateLiveoutVariables: Cannot find size of the memory access for live-out variable\n");
  gPtr->setUnnamedAddr(GlobalValue::UnnamedAddr::Local);
  NODE* nodeGvar = new NODE(constant, 1, ((nodeID++)+100), gPtrName, gPtr, bbIdx);
  nodeGvar->setAlignment(alignment);
  nodeGvar->setDatatype(dt);
  loopDFG->insert_Node(nodeGvar);
  mapLiveoutInstName[BI] = gPtrName;

  // get the liveout data node
  NODE *outputNode = loopDFG->get_Node(BI);
  if (outputNode->get_Instruction() == ld_add) 
    outputNode = outputNode->get_Related_Node();

  
  // TODO: fix this now that with new micro architechture, all 
  // results should be stored in reg in case of rollback so this is
  // not needed

  // Hardware model does not support pred insts (cond_select nodes)
  // to store data to register and hence cannot be used for liveout
  // due to instruction set limitation, following statements add a
  // routing node from such nodes to support liveout data
  if (outputNode->get_Instruction() == cond_select || 
      (outputNode->get_Instruction() == cgra_select && outputNode->getBranchIndex() != -1)) { 
    NODE* routeNode = new NODE(add, 1, nodeID++, "route", NULL, bbIdx);
    routeNode->setDatatype(outputNode->getDatatype());
    loopDFG->insert_Node(routeNode);
    loopDFG->make_Arc(outputNode, routeNode, edgeID++, 0, TrueDep, 0);
    outputNode = routeNode;
  }
  // connect the liveout data to liveout
  DEBUG("  outputNode: " << outputNode->get_Name() << "\n");
  DEBUG("  liveoutNode: " << nodeGvar->get_Name() << "\n");
  loopDFG->make_Arc(outputNode, nodeGvar, edgeID++, 0, LiveOutDataDep, 0);

  unsigned int CGRA_ConstantID = nodeGvar->get_ID();
  unsigned int CGRA_storeAddID = ((nodeID++)+100);
  unsigned int CGRA_storeDataID = ((nodeID++)+100);
  unsigned int liveout_datatype = (unsigned int) (nodeGvar->getDatatype()); 
  //Generate file for live-out
  liveoutNodefile << outputNode->get_ID() << "\t" << outputNode->get_Instruction() << "\t" << outputNode->get_Name() << "\t" << 0 << "\t" <<liveout_datatype << "\n";
  liveoutNodefile << CGRA_ConstantID << "\t" << constant << "\t" << gPtrName << "\t" << 0 << "\t" << liveout_datatype << "\n";
  liveoutNodefile << CGRA_storeAddID << "\t" << st_add << "\t" << "st_add_" + gPtrName << "\t" << alignment << "\t" << liveout_datatype << "\n";
  liveoutNodefile << CGRA_storeDataID << "\t" << st_data << "\t" << "st_data_" + gPtrName << "\t" << 0 << "\t" << liveout_datatype << "\n";

  liveoutEdgefile << CGRA_storeAddID << "\t" << CGRA_storeDataID << "\t0\tSRE\t0\n";
  liveoutEdgefile << outputNode->get_ID() << "\t" << CGRA_storeDataID << "\t0\tTRU\t0\n";
  liveoutEdgefile << CGRA_ConstantID << "\t" << CGRA_storeDataID << "\t0\tTRU\t1\n";

  // here to insert load instructions
  // since there are dedicated loop exits and we onlly deal with one
  // unique exit block now
  // first insert a basic block between latch and the loop exit
  // to be the load block
  if (loadBlock == nullptr) {
    // create a load basic block between latch and exit
    DEBUG("creating a new basic block for loading liveout variables\n");
    BasicBlock* loopExit = L->getUniqueExitBlock();
    loadBlock = loadBlock->Create(loopExit->getContext(), ".acceleration", loopExit->getParent(), nullptr);
    irChanged = true;
    // not considering loop exit with phi nodes since we specified the loop exit will only have
    // a single predecessor
    loadBlock->moveAfter(L->getLoopLatch());
    DEBUG("created load block" << loadBlock->getName() << "\n");
    BranchInst* loadblkBranchInst;
    loadblkBranchInst = loadblkBranchInst->Create(loopExit, loadBlock);
    DEBUG("terminator for load block is " << *loadblkBranchInst <<"\n");
    // need to remember to jump to this block later when removing the loop
  }
  auto *TI = loadBlock->getTerminator();
  LoadInst* loadGlobal = new LoadInst(T, gPtr, "", TI);
  // now replace all the usage outside of loop
  for (auto U : BI->users()) {  // U is  f type User*
    if (auto Inst = dyn_cast<Instruction>(U)) {
      if (std::find(bbs.begin(), bbs.end(), Inst->getParent()) == bbs.end()) {
        DEBUG("inside updating usage\t" << *(Inst) << ": " << *v << "to "<<*(dyn_cast<Value>(loadGlobal)) << "\n");
        Inst->replaceUsesOfWith(dyn_cast<Value>(BI), dyn_cast<Value>(loadGlobal));
        irChanged = true;
      }
    }
  }
  return irChanged;
}


std::tuple<NODE*, bool> 
MultiDDGGen::updateLoopControl (Loop *L, DFG* loopDFG)
{
  DEBUG("Inside updateLoopControl\n");
  // first get the loop exit branch inst
  BranchInst* exitBranchInst = dyn_cast<BranchInst>(L->getLoopLatch()->getTerminator());
  DEBUG(" loop exit branch inst: " << *exitBranchInst << "\n");
  if (exitBranchInst == nullptr) {
    DEBUG("Error, terminator of loop latch not a branch\n");
    exit(1);
  }
  if (exitBranchInst->isUnconditional()) {
    DEBUG("ERROR, unconditional branch for exit\n");
    exit(1);
  }

  // set the whether true dest or false dest is loop exit
  bool exitDest;
  if (!(L->contains(exitBranchInst->getSuccessor(0)))) {
    DEBUG("true dest is exit\n");
    exitDest = true;
  } else if (!(L->contains(exitBranchInst->getSuccessor(1)))) {
    DEBUG("false dest is exit\n");
    exitDest = false;
  } else {
    DEBUG("ERROR: no exit\n");
    exit(1);
  }

  // get the cond instruction of the exit branch
  Instruction* exitCondInst = cast<Instruction>(exitBranchInst->getOperand(0));
  DEBUG(" exit cond instruction: " << *exitCondInst << "\n");
  // get the node for cond inst
  NODE* loopCtrlNode = loopDFG->get_Node(exitCondInst);
  if (loopCtrlNode == NULL) {
    DEBUG("  branch_inst_pred node not found!\n");
    exit(1);
  }
  DEBUG("  loopCtrlNode: " << loopCtrlNode->get_Name() << "\n");
  
  // get all the liveout nodes to add loop control edges
  std::vector<llvm::NODE*> LiveOutNodes;
  for (std::map<Instruction*, std::string>::iterator it = mapLiveoutInstName.begin(); it != mapLiveoutInstName.end(); ++it) {
    NODE* liveoutNode = loopDFG->get_Node(it->first);
    if (liveoutNode->get_Instruction() == ld_data) 
      liveoutNode = liveoutNode->get_Related_Node();
    LiveOutNodes.push_back(liveoutNode);
    DEBUG("  Added: " << liveoutNode->get_Name() << " to liveout nodes\n");
  }
  // also all the stores can be treated as liveout?
  std::vector<NODE*> allNodes = loopDFG->getSetOfVertices();
  for (std::vector<NODE*>::iterator it = allNodes.begin(); it != allNodes.end(); ++it)
    if ((*it)->is_Store_Data_Bus_Write()) {
      LiveOutNodes.push_back(*it);
      DEBUG("  Added from stores: " << (*it)->get_Name() << "\n");
    }


  // add in the loop control edges
  for (std::vector<NODE*>::iterator it = LiveOutNodes.begin(); it != LiveOutNodes.end(); ++it) {
    loopDFG->make_Arc(loopCtrlNode, (*it), edgeID++, 0, LoopControlDep, (*it)->get_Number_of_Pred());
  }

  return std::make_tuple(loopCtrlNode, exitDest);
}


void
MultiDDGGen::splitDFG(DFG* loopDFG, int splitBrId)
{
  DEBUG("splitting the DFG at branch: " << splitBrId << "\n");
  // first for all the nodes in the true and false paths
  if (mapBrIdPaths.find(splitBrId) != mapBrIdPaths.end()) {
    // the brSplit is a branch to split
    std::vector<unsigned> truePath = mapBrIdPaths[splitBrId].truePath;
    std::vector<unsigned> falsePath = mapBrIdPaths[splitBrId].falsePath;
    for (auto nodeIT: loopDFG->getSetOfVertices()) {
      if (std::find(truePath.begin(), truePath.end(), nodeIT->getBasicBlockIdx()) != truePath.end()) {
        // node is part of true path
        nodeIT->setBrPath(true_path);
        DEBUG("node " << nodeIT->get_Name() <<" set to true path\n");
      } else if (std::find(falsePath.begin(), falsePath.end(), nodeIT->getBasicBlockIdx()) != falsePath.end()) {
        // node is part of false path
        nodeIT->setBrPath(false_path);
        DEBUG("node " << nodeIT->get_Name() <<" set to false path\n");
      }
    }
  } else 
    DEBUG("Not a valid br id, DFG won't be split\n");

  // now for all the phi nodes
  for (auto nodeIT: loopDFG->getSetOfVertices()) {
    if (nodeIT->get_Instruction() == cgra_select && nodeIT->getBranchIndex() != -1) {
      // phi nodes inside loop
      if (nodeIT->getBranchIndex() != splitBrId) {
        // phi node not determined by the splitting branch 
        // partial predication tranforms to select
        nodeIT->setOperation(cond_select);
      } else {
        // phi node determined by the splitting branch
        // need to delete the node and connect the nodes directly 
        // to the true and false path 
        DEBUG("node: " << nodeIT->get_Name() << " a phi to be deleted\n");
        // change the arcs from the node to be from both true and false path
        // first get the true node and false node
        NODE* trueNode;
        NODE* falseNode;
        for (auto prevNode : nodeIT->Get_Prev_Nodes()) {
          if (loopDFG->get_Arc(prevNode, nodeIT)->GetOperandOrder() == 0)
            trueNode = prevNode;
          else if (loopDFG->get_Arc(prevNode, nodeIT)->GetOperandOrder() == 1)
            falseNode = prevNode;
        }
        DEBUG("previous true node is " << trueNode->get_Name() << ", ");
        DEBUG("false node is: " << falseNode->get_Name() <<"\n");
        // now connect both to the succ nodes     
        for (auto succNode : nodeIT->Get_Next_Nodes()) {
          DEBUG("successor node of phi node: " << succNode->get_Name() << "\n");
          ARC* arcPhi = loopDFG->get_Arc(nodeIT, succNode);
          
          // here we need to note if this new arc belongs to a path for later mapping
          ARC* arcTrue = loopDFG->make_Arc(trueNode, succNode, edgeID++, arcPhi->Get_Inter_Iteration_Distance(), 
                  arcPhi->Get_Dependency_Type(), arcPhi->GetOperandOrder());
          arcTrue->setPath(true_path);
          ARC* arcFalse = loopDFG->make_Arc(falseNode, succNode, edgeID++, arcPhi->Get_Inter_Iteration_Distance(), 
                  arcPhi->Get_Dependency_Type(), arcPhi->GetOperandOrder());
          arcFalse->setPath(false_path);
        }
        // safe to delete the phi node now
        loopDFG->delete_Node(nodeIT);
      }
    }
  }
  // last, mark the cond node
  for (auto nodeIT: loopDFG->getSetOfVertices()) {
    if (nodeIT->getCondBrId() >= 0) {
      // node is a cond
      if (nodeIT->getCondBrId() == splitBr) {
        nodeIT->setSplitCond(true);
      }
    }
  }
}


void
MultiDDGGen::fuseNodes(DFG* loopDFG)
{
  // for now we can start with the phi node (remember to change 
  // split and save this info), from bottom up traverse the two 
  // path of the DFG tree -> first fuse the two select ops
  // later: for each fused node, fuse based on operand order,
  // if no op to fuse, fuse with noop
  // don't think this is the most optimized method, but it should 
  // work for now
  /****************************************/
  /**
   * from bottom up, give each node a level
   * basically a subgraph technique
  */
  /****************************************/

  // or maybe we could even fuse all nodes in the same level
  // while traversing

  // for a fused node, what it means is that when mapping they exist
  // in the same location (cycle and pe)
  // use scalar evolution to analyse which node to fuse?

  // in the end of this, we are producing a single part of DFG representing
  // both true and false path
  DEBUG("fusing nodes now, not doing anything now, may delete\n");
  // first get the phi node as the base of the split
  NODE* splitBaseNode;
  for (auto nodeIT: loopDFG->getSetOfVertices()) {
    if (nodeIT->get_Instruction() == cgra_select && nodeIT->getBranchIndex() != -1) {
      // phi nodes inside loop
      if (nodeIT->getBranchIndex() == splitBr) {
        // first get the split phi node
        splitBaseNode = nodeIT;
        break;
      }
    }
  }
  // now fuse nodes from bottom up
  // true nodes to be fused
  std::vector<NODE*> trueNodeSet;
  // false nodes to be fused
  std::vector<NODE*> falseNodeSet;
  for (auto prevNode : splitBaseNode->Get_Prev_Nodes()) {
    if (loopDFG->get_Arc(prevNode, splitBaseNode)->GetOperandOrder() == 0)
      trueNodeSet.push_back(prevNode);
    else if (loopDFG->get_Arc(prevNode, splitBaseNode)->GetOperandOrder() == 1)
      falseNodeSet.push_back(prevNode);
  }
  
  // first remove nodes not in path
  for (auto nodeIT = trueNodeSet.begin(); nodeIT != trueNodeSet.end();) {
    if ((*nodeIT)->getBrPath() != true_path) 
      nodeIT = trueNodeSet.erase(nodeIT);
    else
      ++nodeIT;
  }
  for (auto nodeIT = falseNodeSet.begin(); nodeIT != falseNodeSet.end();) {
    if ((*nodeIT)->getBrPath() != false_path) 
      nodeIT = falseNodeSet.erase(nodeIT);
    else
      ++nodeIT;
  }
  //  or we can first map...
  #ifndef NDEBUG_M
    DEBUG("true nodes to be fused include:\n");
    for (auto nodeIT : trueNodeSet) {
      DEBUG(nodeIT->get_Name() << ", " << nodeIT->get_ID() << "\n");
    }
    DEBUG("false nodes to be fused include:\n");
    for (auto nodeIT : falseNodeSet) {
      DEBUG(nodeIT->get_Name() << ", " << nodeIT->get_ID() << "\n");
    }
  #endif
  // when choosing which nodes to fuse, we prioritize minimizing the 
  // total arcs to a fused node

  // arn arc to represent they are fused, 
  // TODO: also consider address generator

  
  // now iterate through the connecting nodes (operands)
  // until not in true or false path 
}


bool 
MultiDDGGen::runOnLoop(Loop *L, LPPassManager &LPM)
{
  bool irChanged = false;
  resetVariables();
  
  // basic blocks of the loop
  std::vector<BasicBlock *> bbs = L->getBlocks();
  // dominator tree analysis
  DominatorTree * DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  // get the module at top because we will be dropping the loop later
  Module* M = L->getLoopPreheader()->getParent()->getParent();

  // check if loop has been marked to be executed on CGRA
  if (!HasCGRAEnablePragma(L) || HasCGRADisablePragma(L)) {
    DEBUG("no pragma for loop: " << L->getLoopID() << ", skipping\n");
    return false;
  }

  // We can only remove the loop if there is a preheader that we can
  // branch from after removing it.
  BasicBlock *Preheader = L->getLoopPreheader();
  if (!Preheader) {
    DEBUG("no preheader function\n");
    return false;
  } else {
    DEBUG("preheader function name: " << Preheader->getParent()->getName() << "\n");
  }

  // If LoopSimplify form is not available, stay out of trouble.
  if (!L->hasDedicatedExits()) {
    DEBUG("exit block has predcessor outside loop\n");
    return false;
  }

  errs() << "loop id: " << L->getLoopID() << "\n";
  // We require that the loop only have a single exit block.  Otherwise, we'd
  // be in the situation of needing to be able to solve statically which exit
  // block will be branched to, or trying to preserve the branching logic in
  // a loop invariant manner.
  BasicBlock *ExitBlk = L->getUniqueExitBlock();
  if (!ExitBlk) {
    DEBUG("no unique exit\n");
    return false;
  } else if (!ExitBlk->getSinglePredecessor()) {
    DEBUG("exit block with multiple predecessors\n");
    return false;
  }

  if (!canExecuteOnCGRA(L,bbs,Preheader,ExitBlk)) {
    // loop cannot be executed on the CGRA
    // maybe should also add metadata about this to the loop
    return false;
  }

  totalLoops++;
  DFG *loopDFG = new DFG();
  
  std::ostringstream osLoopID;
  osLoopID << totalLoops;
  std::string directoryPath = "mkdir -p ./CGRAExec/L" + osLoopID.str();
  system(directoryPath.c_str());


  std::string newPath = "./CGRAExec/L" + osLoopID.str() + "/livein_node.txt";
  liveInNodefile.open(newPath.c_str());
  newPath = "./CGRAExec/L" + osLoopID.str() + "/livein_edge.txt";
  liveInEdgefile.open(newPath.c_str());

  // here to first update the loop's branching information
  updateBranchInfo(L, DT);
  #ifndef NDEBUG_M
    for (auto const &branch : mapBrIdPaths) {
      DEBUG("Printing out all the branches\n");
      DEBUG("BrId: " << branch.first << "\n");
      DEBUG("true path:\n");
      for (auto const i : branch.second.truePath)
        DEBUG(i << "\t");
      DEBUG("\nfalse path:\n");
      for (auto const i: branch.second.falsePath)
        DEBUG(i << "\t");
      DEBUG("\n");
    }
  #endif

  // adding in all the instructions of the loop as nodes in the DFG
  for (int i = 0; i < (int) bbs.size(); i++) {
    DEBUG("runOnLoop::instructions @ bbs[" << i << "]:\n");
    for (BasicBlock::iterator BI = bbs[i]->begin(); BI != bbs[i]->end(); ++BI) {
      DEBUG("\tAdding " << *BI << "\n");
      if (!addNode(&(*BI), loopDFG, i))
        return false;
    }
  }

  #ifndef NDEBUG_M
    DEBUG("Add nodes complete\n");
    for (int i = 0; i < (int) bbs.size(); i++) 
      for (BasicBlock::iterator BI = bbs[i]->begin(); BI != bbs[i]->end(); ++BI) {
        if (loopDFG->get_Node(&(*BI)) != NULL) {
          DEBUG("Ins: " << *BI << "\n");
          DEBUG("node: " << loopDFG->get_Node(&(*BI))->get_ID() << "\n\n");
        }
      }
  #endif


  for (int i = 0; i < (int) bbs.size(); i++) {
    for (BasicBlock::iterator BI = bbs[i]->begin(); BI != bbs[i]->end(); ++BI) {
      if (loopDFG->get_Node(&(*BI)) != NULL) {
        irChanged |= updateLiveInVariables(&(*BI), bbs, Preheader);
      }
    }
  }  
  DEBUG("Update livein complete\n");

  for (int i = 0; i < (int) bbs.size(); i++) {
    for (BasicBlock::iterator BI = bbs[i]->begin(); BI != bbs[i]->end(); ++BI) {
      if (loopDFG->get_Node(&(*BI)) != NULL) {
        updateDataDependencies(&(*BI), loopDFG, L, DT, i);
      }
    }
  }
  DEBUG("completed update data dependency\n");
  liveInNodefile.close();
  liveInEdgefile.close();

  // Write TripCount as 1 in file, done for compatability reasons
  std::ofstream lpTCfile;
  newPath = "./CGRAExec/L" + osLoopID.str() + "/loop_iterations.txt";
  lpTCfile.open(newPath.c_str());
  lpTCfile << 1;
  lpTCfile.close();

  // now for the liveouts
  newPath = "./CGRAExec/L" + osLoopID.str() + "/liveout_node.txt";
  liveoutNodefile.open(newPath.c_str());
  newPath = "./CGRAExec/L" + osLoopID.str() + "/liveout_edge.txt";
  liveoutEdgefile.open(newPath.c_str());
  for (int i = 0; i < (int) bbs.size(); i++) {
    for (BasicBlock::iterator BI = bbs[i]->begin(); BI != bbs[i]->end(); ++BI) {
      if (loopDFG->get_Node(&(*BI)) != NULL) {
        SmallVector<BasicBlock *, 10> ExitBlks;
        std::vector<BasicBlock *> LoopExitBlks;
        L->getExitBlocks(ExitBlks);
        for(unsigned int itt = 0; itt < ExitBlks.size(); itt++) {
          LoopExitBlks.push_back(ExitBlks[itt]);
        } 
        irChanged|= updateLiveOutVariables(&(*BI), loopDFG, L, i);
      }
    }
  } 
  errs() << "completed update liveout\n";
  liveoutNodefile.close();
  liveoutEdgefile.close();

  // maybe some things needs to be done here if there are stores in 
  // conditional branches

  // here for loop exit
  auto [loopCtrlNode, loopExitCond] = updateLoopControl(L, loopDFG);
  std::ofstream LoopCtrlNodeFile;
  std::string filename = "./CGRAExec/L" + osLoopID.str() + "/Control_Node.txt";
  LoopCtrlNodeFile.open(filename.c_str());
  LoopCtrlNodeFile << loopCtrlNode->get_Name() << "\n";
  LoopCtrlNodeFile << loopExitCond << "\n";
  LoopCtrlNodeFile << splitBr << "\n";
  LoopCtrlNodeFile.close();
  DEBUG("Done updating loop control\n");
  
  // this is to split the DFG
  splitDFG(loopDFG, splitBr);
  DEBUG("Done splitting the DFG\n");
  
  // after splitting the DFG, we need to fuse the nodes of the two paths
  //fuseNodes(loopDFG);

  std::ostringstream osNodeID;
  osNodeID << nodeID;
  loopDFG->Dump_Loop("./CGRAExec/L" + osLoopID.str() + "/loop" + osNodeID.str());
  directoryPath = "loop_" + osNodeID.str();
  loopDFG->Dot_Print_DFG(directoryPath.c_str());
  directoryPath = directoryPath + "DFG.dot";
  newPath = "./CGRAExec/L" + osLoopID.str() + "/" + directoryPath;
  std::rename(directoryPath.c_str(), newPath.c_str());
  DEBUG("output DFG complete\n");

  // Now that we know the removal is safe, remove the loop by changing the
  // branch from the preheader to go to the load block inserted earlier.
  DominatorTree &DT1 = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  ScalarEvolution &SE1 = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  LoopInfo &LoopInfo1 = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DEBUG("now deleting the loop\n");
  // Tell ScalarEvolution that the loop is deleted. Do this before
  // deleting the loop so that ScalarEvolution can look at the loop
  // to determine what it needs to clean up.
  SE1.forgetLoop(L);
  // Connect the preheader directly to the the load block.
  auto* TI = Preheader->getTerminator();
  DEBUG("preheader terminator as " << *TI <<"\n");
  TI->replaceUsesOfWith(L->getHeader(), loadBlock);
  // there shouldn't be phi nodes in exit block
  for (BasicBlock::iterator BI = ExitBlk->begin(); BI != ExitBlk->end(); ++BI) {
    if (dyn_cast<PHINode>(BI)) {
      DEBUG("ERROR, there shouldn't be phi nodes in exit block\n");
      exit(1);
    }
  }

  // Update the dominator tree and remove the instructions and blocks that will
  // be deleted from the reference counting scheme.
  DEBUG("Updating dominator tree and removing instructions\n");
  SmallVector<DomTreeNode*, 8> ChildNodes;
  for (Loop::block_iterator LI1 = L->block_begin(), LE = L->block_end();
        LI1 != LE; ++LI1) {
    // Move all of the block's children to be children of the Preheader, which
    // allows us to remove the domtree entry for the block.
    ChildNodes.insert(ChildNodes.begin(), DT1[*LI1]->begin(), DT1[*LI1]->end());
    for (DomTreeNode *ChildNode : ChildNodes) {
      DT1.changeImmediateDominator(ChildNode, DT1[Preheader]);
    }
    BasicBlock * bb = *LI1;
    for (BasicBlock::iterator II = bb->begin(); II != bb->end(); ++II) {
        DEBUG("Dropping references:" << *II << "\n");
        Instruction * insII = &(*II);
        insII->dropAllReferences();
    }
    ChildNodes.clear();
    DT1.eraseNode(*LI1);
    // Remove the block from the reference counting scheme, so that we can
    // delete it freely later.
    (*LI1)->dropAllReferences();
  }
  // Erase the instructions and the blocks without having to worry
  // about ordering because we already dropped the references.
  // NOTE: This iteration is safe because erasing the block does not remove its
  // entry from the loop's block list.  We do that in the next section.
  DEBUG("Erasing instructions without caring about references\n");
  for (Loop::block_iterator LI1 = L->block_begin(), LE = L->block_end();
        LI1 != LE; ++LI1)
    (*LI1)->eraseFromParent();

  // Finally, erase the blocks from loopinfo.
  // This has to happen late because
  // otherwise our loop iterators won't work.
  DEBUG("Erasing loopinfo\n");
  SmallPtrSet<BasicBlock *, 8> blocks;
  blocks.insert(L->block_begin(), L->block_end());
  for (BasicBlock *BB : blocks)
    LoopInfo1.removeBlock(BB);

  //Now we have deleted loop successfully
  //Let's insert function call in place of the loop
  DEBUG("inserting accelerate function in place of loop\n");
  Constant *hookFunc;
  hookFunc = M->getFunction("accelerateOnCGRA");
  DEBUG("hookFunc is " << hookFunc << "\n");
  Function* hook= cast<Function>(hookFunc);
  Value *LoopNumber = ConstantInt::get(Type::getInt32Ty(M->getContext()), totalLoops);
  DEBUG("Loop Number is " << *LoopNumber << "\n");
  Instruction *newInst = CallInst::Create(hook, LoopNumber, "");
  DEBUG("newInst = " << *newInst << "\n");
  loadBlock->getInstList().push_front(newInst);

  // Update total Number of Loops
  std::ofstream totalLoopsfile;
  std::string totalLoopsfilefilename = "./CGRAExec/total_loops.txt";
  totalLoopsfile.open(totalLoopsfilefilename.c_str());
  totalLoopsfile << totalLoops;
  totalLoopsfile.close();

  DEBUG("runOnLoop went well\n"); 

  delete loopDFG;
  return irChanged;

}


char MultiDDGGen::ID = 0;
static RegisterPass<MultiDDGGen> 
X("MultiDDGGen", "Loop Pass to Generate Multi Data Dependency Graph", false, true);
