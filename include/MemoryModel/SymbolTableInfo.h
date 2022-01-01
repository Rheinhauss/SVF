//===- SymbolTableInfo.h -- Symbol information from IR------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * SymbolTableInfo.h
 *
 *  Created on: Nov 11, 2013
 *      Author: Yulei Sui
 */

#ifndef INCLUDE_SVF_FE_SYMBOLTABLEINFO_H_
#define INCLUDE_SVF_FE_SYMBOLTABLEINFO_H_

#include "MemoryModel/SVFSymbols.h"
#include "Util/SVFUtil.h"

namespace SVF
{

/*!
 * Symbol table of the memory model for analysis
 */
class SymbolTableInfo
{
friend class SymbolTableBuilder;

public:
    /// various maps defined
    //{@
    /// llvm value to sym id map
    /// local (%) and global (@) identifiers are pointer types which have a value node id.
    typedef OrderedMap<const Value *, SymID> ValueToIDMapTy;
    /// sym id to memory object map
    typedef OrderedMap<SymID,ObjSym*> IDToMemMapTy;
    /// function to sym id map
    typedef OrderedMap<const Function *, SymID> FunToIDMapTy;
    /// sym id to sym type map
    typedef OrderedSet<const SVFVar*> SymSet;
    /// struct type to struct info map
    typedef OrderedMap<const Type*, StInfo*> TypeToFieldInfoMap;
    typedef Set<CallSite> CallSiteSet;
    typedef OrderedMap<const Instruction*,CallSiteID> CallSiteToIDMapTy;
    typedef OrderedMap<CallSiteID,const Instruction*> IDToCallSiteMapTy;

    //@}

private:
    /// Data layout on a target machine
    static DataLayout *dl;

    ValueToIDMapTy valSymMap;	///< map a value to its sym id
    ValueToIDMapTy objSymMap;	///< map a obj reference to its sym id
    FunToIDMapTy returnSymMap;		///< return  map
    FunToIDMapTy varargSymMap;	    ///< vararg map
    SymSet	symSet;	/// < a set of all symbols
    IDToMemMapTy		objMap;		///< map a memory sym id to its obj

    CallSiteSet callSiteSet;

    // Singleton pattern here to enable instance of SymbolTableInfo can only be created once.
    static SymbolTableInfo* symInfo;

    /// Module
    SVFModule* mod;

    /// Max field limit
    static u32_t maxFieldLimit;

    /// Clean up memory
    void destroy();

    /// Whether to model constants
    bool modelConstants;

    /// total number of symbols
    SymID totalSymNum;

protected:
    /// Constructor
    SymbolTableInfo(void) :
        mod(nullptr), modelConstants(false), totalSymNum(0), maxStruct(nullptr), maxStSize(0)
    {
    }

public:

    /// Singleton design here to make sure we only have one instance during any analysis
    //@{
    static SymbolTableInfo* SymbolInfo();

    static void releaseSymbolInfo()
    {
        delete symInfo;
        symInfo = nullptr;
    }
    virtual ~SymbolTableInfo()
    {
        destroy();
    }
    //@}

    /// Set / Get modelConstants
    //@{
    void setModelConstants(bool _modelConstants)
    {
        modelConstants = _modelConstants;
    }
    bool getModelConstants() const
    {
        return modelConstants;
    }
    //@}

    /// Get callsite set
    //@{
    inline const CallSiteSet& getCallSiteSet() const
    {
        return callSiteSet;
    }
    //@}

    /// Module
    inline SVFModule* getModule()
    {
        return mod;
    }
        /// Module
    inline void setModule(SVFModule* m)
    {
        mod = m;
    }

    /// Get target machine data layout
    inline static DataLayout* getDataLayout(Module* mod)
    {
        if(dl==nullptr)
            return dl = new DataLayout(mod);
        return dl;
    }

    /// Helper method to get the size of the type from target data layout
    //@{
    u32_t getTypeSizeInBytes(const Type* type);
    u32_t getTypeSizeInBytes(const StructType *sty, u32_t field_index);
    //@}

    /// special value
    // @{
    static bool isNullPtrSym(const Value *val);

    static bool isBlackholeSym(const Value *val);

    bool isConstantObjSym(const Value *val);

    static inline bool isBlkPtr(NodeID id)
    {
        return (id == BlkPtr);
    }
    static inline bool isNullPtr(NodeID id)
    {
        return (id == NullPtr);
    }
    static inline bool isBlkObj(NodeID id)
    {
        return (id == BlackHole);
    }
    static inline bool isConstantObj(NodeID id)
    {
        return (id == ConstantObj);
    }
    static inline bool isBlkObjOrConstantObj(NodeID id)
    {
        return (isBlkObj(id) || isConstantObj(id));
    }

    inline ObjSym* createBlkObj(SymID symId)
    {
        assert(isBlkObj(symId));
        assert(objMap.find(symId)==objMap.end());
        ObjSym* obj = new BlackHoleSym(symId, createObjTypeInfo());
        objMap[symId] = obj;
        return obj;
    }

    inline ObjSym* createConstantObj(SymID symId)
    {
        assert(isConstantObj(symId));
        assert(objMap.find(symId)==objMap.end());
        ObjSym* obj = new ConstantObjSym(symId, createObjTypeInfo());
        objMap[symId] = obj;
        return obj;
    }

    inline ObjSym* getBlkObj() const
    {
        return getObj(blackholeSymID());
    }
    inline ObjSym* getConstantObj() const
    {
        return getObj(constantSymID());
    }

    inline SymID blkPtrSymID() const
    {
        return BlkPtr;
    }

    inline SymID nullPtrSymID() const
    {
        return NullPtr;
    }

    inline SymID constantSymID() const
    {
        return ConstantObj;
    }

    inline SymID blackholeSymID() const
    {
        return BlackHole;
    }

    /// Can only be invoked by PAG::addDummyNode() when creaing PAG from file.
    inline const ObjSym* createDummyObj(SymID symId, const Type* type)
    {
        assert(objMap.find(symId)==objMap.end() && "this dummy obj has been created before");
        ObjSym* memObj = new ObjSym(symId, createObjTypeInfo(type));
        objMap[symId] = memObj;
        return memObj;
    }
    // @}

    /// Get different kinds of syms
    //@{
    SymID getValSym(const Value *val)
    {

        if(isNullPtrSym(val))
            return nullPtrSymID();
        else if(isBlackholeSym(val))
            return blkPtrSymID();
        else
        {
            ValueToIDMapTy::const_iterator iter =  valSymMap.find(val);
            assert(iter!=valSymMap.end() &&"value sym not found");
            return iter->second;
        }
    }

    inline bool hasValSym(const Value* val)
    {
        if (isNullPtrSym(val) || isBlackholeSym(val))
            return true;
        else
            return (valSymMap.find(val) != valSymMap.end());
    }

    inline SymID getObjSym(const Value *val) const
    {
        ValueToIDMapTy::const_iterator iter = objSymMap.find(SVFUtil::getGlobalRep(val));
        assert(iter!=objSymMap.end() && "obj sym not found");
        return iter->second;
    }

    inline ObjSym* getObj(SymID id) const
    {
        IDToMemMapTy::const_iterator iter = objMap.find(id);
        assert(iter!=objMap.end() && "obj not found");
        return iter->second;
    }

    inline SymID getRetSym(const Function *val) const
    {
        FunToIDMapTy::const_iterator iter =  returnSymMap.find(val);
        assert(iter!=returnSymMap.end() && "ret sym not found");
        return iter->second;
    }

    inline SymID getVarargSym(const Function *val) const
    {
        FunToIDMapTy::const_iterator iter =  varargSymMap.find(val);
        assert(iter!=varargSymMap.end() && "vararg sym not found");
        return iter->second;
    }
    //@}

 
    /// Statistics
    //@{
    inline Size_t getTotalSymNum() const
    {
        return totalSymNum;
    }
    inline u32_t getMaxStructSize() const
    {
        return maxStSize;
    }
    //@}

    /// Get different kinds of syms maps
    //@{
    inline ValueToIDMapTy& valSyms()
    {
        return valSymMap;
    }

    inline ValueToIDMapTy& objSyms()
    {
        return objSymMap;
    }

    inline IDToMemMapTy& idToObjMap()
    {
        return objMap;
    }

    inline FunToIDMapTy& retSyms()
    {
        return returnSymMap;
    }

    inline FunToIDMapTy& varargSyms()
    {
        return varargSymMap;
    }

    //@}

    /// Get struct info
    //@{
    ///Get an iterator for StructInfo, designed as internal methods
    TypeToFieldInfoMap::iterator getStructInfoIter(const Type *T)
    {
        assert(T);
        TypeToFieldInfoMap::iterator it = typeToFieldInfo.find(T);
        if (it != typeToFieldInfo.end())
            return it;
        else
        {
            collectTypeInfo(T);
            return typeToFieldInfo.find(T);
        }
    }

    ///Get a reference to StructInfo.
    inline StInfo* getStructInfo(const Type *T)
    {
        return getStructInfoIter(T)->second;
    }

    ///Get a reference to the components of struct_info.
    const inline std::vector<u32_t>& getFattenFieldIdxVec(const Type *T)
    {
        return getStructInfoIter(T)->second->getFieldIdxVec();
    }
    const inline std::vector<u32_t>& getFattenFieldOffsetVec(const Type *T)
    {
        return getStructInfoIter(T)->second->getFieldOffsetVec();
    }
    const inline std::vector<FieldInfo>& getFlattenFieldInfoVec(const Type *T)
    {
        return getStructInfoIter(T)->second->getFlattenFieldInfoVec();
    }
    const inline Type* getOrigSubTypeWithFldInx(const Type* baseType, u32_t field_idx)
    {
        return getStructInfoIter(baseType)->second->getFieldTypeWithFldIdx(field_idx);
    }
    const inline Type* getOrigSubTypeWithByteOffset(const Type* baseType, u32_t byteOffset)
    {
        return getStructInfoIter(baseType)->second->getFieldTypeWithByteOffset(byteOffset);
    }
    //@}

    /// Compute gep offset
    virtual bool computeGepOffset(const User *V, LocationSet& ls);
    /// Get the base type and max offset
    const Type *getBaseTypeAndFlattenedFields(const Value *V, std::vector<LocationSet> &fields);
    /// Replace fields with flatten fields of T if the number of its fields is larger than msz.
    u32_t getFields(std::vector<LocationSet>& fields, const Type* T, u32_t msz);
    /// Collect type info
    void collectTypeInfo(const Type* T);
    /// Given an offset from a Gep Instruction, return it modulus offset by considering memory layout
    virtual LocationSet getModulusOffset(const ObjSym* obj, const LocationSet& ls);

    /// Debug method
    void printFlattenFields(const Type* type);

    static std::string toString(SYMTYPE symtype);

    /// Another debug method
    virtual void dump();

protected:
    /// Collect the struct info
    virtual void collectStructInfo(const StructType *T);
    /// Collect the array info
    virtual void collectArrayInfo(const ArrayType* T);
    /// Collect simple type (non-aggregate) info
    virtual void collectSimpleTypeInfo(const Type* T);

    /// Create an objectInfo based on LLVM type (value is null, and type could be null, representing a dummy object)
    ObjTypeInfo* createObjTypeInfo(const Type *type = nullptr);

    /// Every type T is mapped to StInfo
    /// which contains size (fsize) , offset(foffset)
    /// fsize[i] is the number of fields in the largest such struct, else fsize[i] = 1.
    /// fsize[0] is always the size of the expanded struct.
    TypeToFieldInfoMap typeToFieldInfo;

    ///The struct type with the most fields
    const Type* maxStruct;

    ///The number of fields in max_struct
    u32_t maxStSize;
};


} // End namespace SVF

#endif /* INCLUDE_SVF_FE_SYMBOLTABLEINFO_H_ */
