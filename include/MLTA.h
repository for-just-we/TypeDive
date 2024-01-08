//
// Created by prophe cheng on 2024/1/3.
//

#ifndef TYPEDIVE_MLTA_H
#define TYPEDIVE_MLTA_H

#include "Analyzer.h"
#include "Config.h"
#include "llvm/IR/Operator.h"

typedef pair<Type*, int> typeidx_t;
pair<Type *, int> typeidx_c(Type *Ty, int Idx);

typedef pair<size_t, int> hashidx_t;
pair<size_t, int> hashidx_c(size_t Hash, int Idx);

string getInstructionText(Value* inst);

string getInstructionText(Type* type);

class MLTA {
protected:
    // Global信息
    GlobalContext *Ctx;

    ////////////////////////////////////////////////////////////////
    // Important data structures for type confinement, propagation,
    // and escapes.
    ////////////////////////////////////////////////////////////////
    DenseMap<size_t, map<int, FuncSet>> typeIdxFuncsMap; // type-func set
    map<size_t, map<int, set<hashidx_t>>> typeIdxPropMap; // type-prop set，表示type t_key的第i个field会被type t_value的第j个field传值
    set<size_t> typeEscapeSet;
    // Cap type: We cannot know where the type can be futher
    // propagated to. Do not include idx in the hash
    set<size_t> typeCapSet;

    ////////////////////////////////////////////////////////////////
    // Other data structures
    ////////////////////////////////////////////////////////////////
    // Cache matched functions for CallInst
    DenseMap<size_t, FuncSet> MatchedFuncsMap;
    // Applying to virtuall-function, which requires extra efforts because it frequently cast to general types such as char*
    DenseMap<Value *, FuncSet> VTableFuncsMap;

    set<size_t> srcLnHashSet;
    set<size_t> addrTakenFuncHashSet;

    map<size_t, set<size_t>> calleesSrcMap;
    map<size_t, set<size_t>> L1CalleesSrcMap;

    // Matched icall types -- to avoid repeatation
    DenseMap<size_t, FuncSet> MatchedICallTypeMap;

    // Set of target types
    set<size_t>TTySet;

    // Functions that are actually stored to variables
    FuncSet StoredFuncs;
    // Special functions like syscalls
    FuncSet OutScopeFuncs;

    // Alias struct pointer of a general pointer
    // 保存Function F中char*, void*到其它结构体类型的转换，每个function只记录一种
    map<Function*, map<Value*, Value*>> AliasStructPtrMap;

    ////////////////////////////////////////////////////////////////
    // Type-related basic functions
    ////////////////////////////////////////////////////////////////
    bool fuzzyTypeMatch(Type *Ty1, Type *Ty2, Module *M1, Module *M2);

    void escapeType(Value *V);
    void propagateType(Value *ToV, Type *FromTy, int Idx = -1);

    Type *getBaseType(Value *V, set<Value *> &Visited);
    Type *_getPhiBaseType(PHINode *PN, set<Value *> &Visited);
    Function *getBaseFunction(Value *V);
    bool nextLayerBaseType(Value *V, list<typeidx_t> &TyList,
                           Value * &NextV, set<Value *> &Visited);

    bool getGEPLayerTypes(GEPOperator *GEP, list<typeidx_t> &TyList);
    bool getBaseTypeChain(list<typeidx_t> &Chain, Value *V,
                          bool &Complete);
    bool getDependentTypes(Type *Ty, int Idx, set<hashidx_t> &PropSet);


    ////////////////////////////////////////////////////////////////
    // Target-related basic functions
    ////////////////////////////////////////////////////////////////
    void confineTargetFunction(Value *V, Function *F);
    void intersectFuncSets(FuncSet &FS1, FuncSet &FS2,
                           FuncSet &FS);
    bool typeConfineInInitializer(GlobalVariable *GV);
    bool typeConfineInFunction(Function *F);
    bool typePropInFunction(Function *F);
    void collectAliasStructPtr(Function *F);

    ////////////////////////////////////////////////////////////////
    // API functions
    ////////////////////////////////////////////////////////////////
    // Use type-based analysis to find targets of indirect calls
    void findCalleesWithType(CallInst*, FuncSet&);
    bool findCalleesWithMLTA(CallInst *CI, FuncSet &FS);
    bool getTargetsWithLayerType(size_t TyHash, int Idx,
                                 FuncSet &FS);


    ////////////////////////////////////////////////////////////////
    // Util functions
    ////////////////////////////////////////////////////////////////
    bool isCompositeType(Type *Ty);
    Type *getFuncPtrType(Value *V);
    Value *recoverBaseType(Value *V);


public:
    // General pointer types like char * and void *
    map<Module *, Type *>Int8PtrTy;
    // long interger type
    map<Module *, Type *>IntPtrTy;
    map<Module *, const DataLayout *>DLMap;

    MLTA(GlobalContext *Ctx_) {
        Ctx = Ctx_;
    }

    string getValueInfo(Value* value) {
        string inst_name;
        raw_string_ostream rso(inst_name);
        value->print(rso);
        return rso.str();
    }

    string getTypeInfo(Type* type) {
        string inst_name;
        raw_string_ostream rso(inst_name);
        type->print(rso);
        return rso.str();
    }
};

#endif //TYPEDIVE_MLTA_H
