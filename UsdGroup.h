// Copyright 2020 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "UsdBridgedBaseObject.h"
#include "UsdBridge.h"

class UsdDevice;
class UsdDataArray;

enum class UsdGroupComponents
{
  SURFACES = 0,
  VOLUMES
};

struct UsdGroupData
{
  UsdSharedString* name = nullptr;
  UsdSharedString* usdName = nullptr;

  int timeVarying = 0xFFFFFFFF; // Bitmask indicating which attributes are time-varying.
  UsdDataArray* surfaces = nullptr;
  UsdDataArray* volumes = nullptr;
};

class UsdGroup : public UsdBridgedBaseObject<UsdGroup, UsdGroupData, UsdGroupHandle, UsdGroupComponents>
{
  public:
    UsdGroup(const char* name, UsdDevice* device);
    ~UsdGroup();

    void remove(UsdDevice* device) override;

    static constexpr ComponentPair componentParamNames[] = {
      ComponentPair(UsdGroupComponents::SURFACES, "surface"),
      ComponentPair(UsdGroupComponents::VOLUMES, "volume")};

  protected:
    bool deferCommit(UsdDevice* device) override;
    bool doCommitData(UsdDevice* device) override;
    void doCommitRefs(UsdDevice* device) override;

    std::vector<UsdSurfaceHandle> surfaceHandles; // for convenience
    std::vector<UsdVolumeHandle> volumeHandles; // for convenience
    std::vector<int> instanceableValues; // for convenience
};

typedef void (UsdBridge::*SetRefFunc)(UsdGroupHandle, const UsdSurfaceHandle*, uint64_t, bool, double);

template<int ChildAnariTypeEnum, typename ChildAnariType, typename ChildUsdType, 
  typename ParentHandleType, typename ChildHandleType>
void ManageRefArray(ParentHandleType parentHandle, UsdDataArray* childArray, bool refsTimeVarying, double timeStep,
  std::vector<ChildHandleType>& tempChildHandles, std::vector<int>& tempInstanceableValues,
  void (UsdBridge::*SetRefFunc)(ParentHandleType, const ChildHandleType*, uint64_t, bool, double, const int*), void (UsdBridge::*DeleteRefFunc)(ParentHandleType, bool, double), 
  UsdBridge* usdBridge, UsdLogInfo& logInfo, const char* typeErrorMsg)
{
  bool validRefs = AssertArrayType(childArray, ChildAnariTypeEnum, logInfo, typeErrorMsg);

  if(validRefs)
  {
    if (childArray)
    {
      const ChildAnariType* children = reinterpret_cast<const ChildAnariType*>(childArray->getData());

      uint64_t numChildren = childArray->getLayout().numItems1;
      tempChildHandles.resize(numChildren);
      tempInstanceableValues.resize(numChildren);
      for (uint64_t i = 0; i < numChildren; ++i)
      {
        const ChildUsdType* usdChild = reinterpret_cast<const ChildUsdType*>(children[i]);
        tempChildHandles[i] = usdChild->getUsdHandle();
        tempInstanceableValues[i] = usdChild->isInstanceable() ? 1 : 0;
      }

      (usdBridge->*SetRefFunc)(parentHandle, tempChildHandles.data(), numChildren, refsTimeVarying, timeStep, tempInstanceableValues.data());
    }
    else
    {
      (usdBridge->*DeleteRefFunc)(parentHandle, refsTimeVarying, timeStep);
    }
  }
}