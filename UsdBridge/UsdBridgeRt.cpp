// Copyright 2020 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#include "usd.h"
#include "UsdBridgeRt.h"
#define USE_USDRT

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
#else
using UsdRtStageRefPtrType = PXR_NS::UsdStagePtr;
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
    UsdBridgeRtInternals(const PXR_NS::UsdStagePtr& sceneStage, const PXR_NS::UsdStagePtr& timeVarStage)
    {
      SceneStage = ToUsdRtStage(sceneStage);
      TimeVarStage = ToUsdRtStage(timeVarStage);
    }

    UsdRtStageRefPtrType SceneStage;
    UsdRtStageRefPtrType TimeVarStage;
};

UsdBridgeRt::UsdBridgeRt(const PXR_NS::UsdStagePtr& sceneStage, const PXR_NS::UsdStagePtr& timeVarStage)
  : Internals(new UsdBridgeRtInternals(sceneStage, timeVarStage))
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


void UsdBridgeRt::geomUpdate(const PXR_NS::SdfPath& geomPath)
{

  UsdGeomMesh uniformGeom = UsdGeomMesh::Get(this->SceneStage, geomPath);
  assert(uniformGeom);
  UsdGeomPrimvarsAPI uniformPrimvars(uniformGeom);

  UsdGeomMesh timeVarGeom = UsdGeomMesh::Get(this->TimeVarStage, geomPath);
  assert(timeVarGeom);
  UsdGeomPrimvarsAPI timeVarPrimvars(timeVarGeom);



}
