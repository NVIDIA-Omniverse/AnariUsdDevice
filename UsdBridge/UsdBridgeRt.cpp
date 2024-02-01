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
    UsdUtilsStageCache& stageCache = PXR_NS::UsdUtilsStageCache::Get();

    // If stage doesn't exist in stagecache, insert
    PXR_NS::UsdStageCache::Id usdStageId = stageCache.GetId(usdStage);
    if(!usdStageId.IsValid())
      usdStageId = PXR_NS::UsdUtilsStageCache::Get().Insert(usdStage);

    omni::fabric::UsdStageId stageId(usdStageId.ToLongInt());

    return usdrt::UsdStage::Attach(stageId);
#else
    return usdStage;
#endif
  }
}

class UsdBridgeRtInternals
{
  UsdBridgeRtInternals(const PXR_NS::UsdStagePtr& sceneStage, const PXR_NS::UsdStagePtr& timeVarStage)
  {
    SceneStage = ToUsdRtStage(sceneStage);
    TimeVarStage = ToUsdRtStage(timeVarStage);
  }

  UsdRtStageRefPtrType SceneStage;
  UsdRtStageRefPtrType TimeVarStage;
};

UsdBridgeRt::UsdBridgeRt(const PXR_NS::UsdStagePtr& sceneStage, const PXR_NS::UsdStagePtr& timeVarStage)
  Internals(new UsdBridgeRtInternals(sceneStage, timeVarStage))
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

/*
{

  // To avoid data duplication when using of clip stages, we need to potentially use the scenestage prim for time-uniform data.
  UsdGeomMesh uniformGeom = UsdGeomMesh::Get(this->SceneStage, meshPath);
  assert(uniformGeom);
  UsdGeomPrimvarsAPI uniformPrimvars(uniformGeom);

  UsdGeomMesh timeVarGeom = UsdGeomMesh::Get(timeVarStage, meshPath);
  assert(timeVarGeom);
  UsdGeomPrimvarsAPI timeVarPrimvars(timeVarGeom);

}
*/