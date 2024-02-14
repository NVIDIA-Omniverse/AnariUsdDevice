// Copyright 2020 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#ifndef UsdBridgeRt_h
#define UsdBridgeRt_h

#include "UsdBridgeData.h"

class UsdBridgeRtInternals;

class UsdBridgeRt
{
  public:
    UsdBridgeRt(const ::PXR_NS::UsdStagePtr& sceneStage, const ::PXR_NS::UsdStagePtr& timeVarStage, const ::PXR_NS::SdfPath& primPath);
    ~UsdBridgeRt();

    void UpdateAttributes(const UsdBridgeLogObject& logObj, const void* arrayData, UsdBridgeType arrayDataType, size_t arrayNumElements, 
      const ::PXR_NS::UsdAttribute& attrib, const ::PXR_NS::UsdTimeCode& timeCode, bool timeVaryingUpdate);

  protected:

    UsdBridgeRtInternals* Internals = nullptr;
};

#endif