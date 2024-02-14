// Copyright 2020 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#include "usd.h"
#include "UsdBridgeRt.h"

#ifdef USE_USDRT
#include <usdrt/scenegraph/base/vt/array.h>
#include <usdrt/scenegraph/base/tf/token.h>
#include <usdrt/scenegraph/usd/sdf/path.h>
#include <usdrt/scenegraph/usd/usd/stage.h>
#include <usdrt/scenegraph/usd/usd/prim.h>
#include <usdrt/scenegraph/usd/usd/attribute.h>
#include <usdrt/scenegraph/usd/usdGeom/tokens.h>
#include <usdrt/scenegraph/usd/usdGeom/primvarsAPI.h>
#endif

#ifdef USE_USDRT
using UsdRtStageRefPtrType = usdrt::UsdStageRefPtr;
using UsdRtPrimType = usdrt::UsdPrim;
using UsdRtSdfPathType = usdrt::SdfPath;
#else
using UsdRtStageRefPtrType = PXR_NS::UsdStagePtr;
using UsdRtPrimType = PXR_NS::UsdPrim;
using UsdRtSdfPathType = PXR_NS::SdfPath;
#endif

namespace
{
  UsdRtStageRefPtrType ToUsdRtStage(const PXR_NS::UsdStagePtr& usdStage)
  {
#ifdef USE_USDRT
    PXR_NS::UsdStageCache& stageCache = PXR_NS::UsdUtilsStageCache::Get();

    usdrt::UsdStageRefPtr stage = usdrt::UsdStage::Open("./data/test.usda");

    // If stage doesn't exist in stagecache, insert
    PXR_NS::UsdStageCache::Id usdStageId = stageCache.GetId(usdStage);
    if(!usdStageId.IsValid())
      usdStageId = stageCache.Insert(usdStage);
    assert(usdStageId.IsValid());

    omni::fabric::UsdStageId stageId(usdStageId.ToLongInt());

    return usdrt::UsdStage::Attach(stageId);
#else
    return usdStage;
#endif
  }
}

class UsdBridgeRtInternals
{
  public:
    UsdBridgeRtInternals(const PXR_NS::UsdStagePtr& sceneStage, const PXR_NS::UsdStagePtr& timeVarStage, const PXR_NS::SdfPath& primPath)
    {
      SceneStage = ToUsdRtStage(sceneStage);
      TimeVarStage = ToUsdRtStage(timeVarStage);

#ifdef USE_USDRT
      RtPrimPath = primPath.GetString();
#else
      RtPrimPath = primPath;
#endif

      UniformPrim = this->SceneStage->GetPrimAtPath(RtPrimPath);
      assert(UniformPrim);

      TimeVarPrim = this->TimeVarStage->GetPrimAtPath(RtPrimPath);
      assert(TimeVarPrim);
    }

    UsdRtStageRefPtrType SceneStage;
    UsdRtStageRefPtrType TimeVarStage;
    UsdRtSdfPathType RtPrimPath;
    UsdRtPrimType UniformPrim;
    UsdRtPrimType TimeVarPrim;
};

UsdBridgeRt::UsdBridgeRt(const PXR_NS::UsdStagePtr& sceneStage, const PXR_NS::UsdStagePtr& timeVarStage, const PXR_NS::SdfPath& primPath)
  : Internals(new UsdBridgeRtInternals(sceneStage, timeVarStage, primPath))
{
}

UsdBridgeRt::~UsdBridgeRt()
{
  delete Internals;
}

#ifdef USE_USDRT
using namespace usdrt;
#else
PXR_NAMESPACE_USING_DIRECTIVE
#endif

#define USE_USDRT_ELTTYPE // To get the right elementtype type def for arrays (not the same between usdrt and usd)
#include "UsdBridgeUsdWriter_Arrays.h"

#ifdef USE_FABRIC

#include <usdrt_only/omni/fabric/stage/StageReaderWriter.h>

struct UsdBridgeFabricSpanInit
{
  UsdBridgeFabricSpanInit(
    size_t numElements,
    omni::fabric::Path& primPath,
    omni::fabric::Token& attrName,
    omni::fabric::StageReaderWriter& stageReaderWriter)
    : PrimPath(primPath)
    , AttrName(attrName)
    , StageReaderWriter(stageReaderWriter)
  {
    stageReaderWriter.setArrayAttributeSize(primPath, attrName, numElements);
  }

  omni::fabric::Path& PrimPath;
  omni::fabric::Token& AttrName;
  omni::fabric::StageReaderWriter& StageReaderWriter;
};

template<typename EltType>
class UsdBridgeFabricSpan : public UsdBridgeSpanWrI<EltType>
{
  public:
    UsdBridgeFabricSpan(UsdBridgeFabricSpanInit& spanInit)
      : AttribSpan(spanInit.StageReaderWriter.getArrayAttributeWr<EltType>(spanInit.PrimPath, spanInit.AttrName))
    {}

    EltType* begin() override
    {
      if(AttribSpan.size())
        return &AttribSpan[0];
      return nullptr;
    }

    EltType* end() override
    {
      if(AttribSpan.size())
        return &AttribSpan[0]+AttribSpan.size();
      return nullptr;
    }

    size_t size() const override
    {
      return AttribSpan.size();
    }

    void assignToPrimvar()
    {}

    gsl::span<EltType> AttribSpan;
};
#endif

void UsdBridgeRt::UpdateAttributes(const UsdBridgeLogObject& logObj, const void* arrayData, UsdBridgeType arrayDataType, size_t arrayNumElements, 
  const ::PXR_NS::UsdAttribute& attrib, const ::PXR_NS::UsdTimeCode& timeCode, bool timeVaryingUpdate)
{
#ifdef USE_FABRIC

  omni::fabric::StageReaderWriterId stageReaderWriterId = (timeVaryingUpdate ? Internals->TimeVarStage : Internals->SceneStage)->GetStageReaderWriterId();
  omni::fabric::StageReaderWriter stageReaderWriter(stageReaderWriterId);

  omni::fabric::Path fPrimPath(attrib.GetPrimPath().GetString().c_str());
  omni::fabric::Token fAttribName(attrib.GetName().GetString().c_str());

  UsdBridgeFabricSpanInit fabricSpanInit(arrayNumElements, fPrimPath, fAttribName, stageReaderWriter);
  AssignAttribArrayToPrimvar<UsdBridgeFabricSpanInit, UsdBridgeFabricSpan>(
    logObj, arrayData, arrayDataType, arrayNumElements, fabricSpanInit);

#else

#ifdef USE_USDRT
  UsdPrim rtPrim = timeVaryingUpdate ? Internals->TimeVarPrim : Internals->UniformPrim;
  const std::string& attribName = attrib.GetName().GetString();
    
  UsdAttribute rtAttrib = rtPrim.GetAttribute(attribName);
  UsdTimeCode rtTimeCode(timeCode.GetValue());
#else
  UsdAttribute rtAttrib = attrib;
  UsdTimeCode rtTimeCode = timeCode;
#endif
  assert(rtAttrib);

  AttribSpanInit spanInit(arrayNumElements, &rtAttrib, &rtTimeCode);
  AssignAttribArrayToPrimvar(logObj, arrayData, arrayDataType, arrayNumElements, spanInit);

#endif
}
