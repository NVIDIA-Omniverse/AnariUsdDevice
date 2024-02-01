// Copyright 2020 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

class UsdBridgeRtInternals;

class UsdBridgeRt
{
  public:

    UsdBridgeRt(const PXR_NS::UsdStagePtr& sceneStage, const PXR_NS::UsdStagePtr& timeVarStage);
    ~UsdBridgeRt();

  protected:

    UsdBridgeRtInternals* Internals = nullptr;
};
