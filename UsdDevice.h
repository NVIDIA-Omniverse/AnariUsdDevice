// Copyright 2020 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "anari/backend/DeviceImpl.h"
#include "anari/backend/LibraryImpl.h"
#include "UsdBaseObject.h"

#include <vector>
#include <memory>

#ifdef _WIN32
#ifdef anari_library_usd_EXPORTS
#define USDDevice_INTERFACE __declspec(dllexport)
#else
#define USDDevice_INTERFACE __declspec(dllimport)
#endif
#endif

extern "C"
{
#ifdef _WIN32
  USDDevice_INTERFACE void __cdecl anari_library_usd_init();
#else
  void anari_library_usd_init();
#endif
}

class UsdDevice;
class UsdDeviceInternals;
class UsdBaseObject;
class UsdVolume;

struct UsdDeviceData
{
  UsdSharedString* hostName = nullptr;
  UsdSharedString* outputPath = nullptr;
  bool createNewSession = true;
  bool outputBinary = false;
  bool writeAtCommit = false;

  double timeStep = 0.0;

  bool outputMaterial = true;
  bool outputPreviewSurfaceShader = true;
  bool outputMdlShader = true;
};

class UsdDevice : public anari::DeviceImpl, public UsdParameterizedBaseObject<UsdDevice, UsdDeviceData>
{
  public:

    UsdDevice();
    UsdDevice(ANARILibrary library);
    ~UsdDevice();

    ///////////////////////////////////////////////////////////////////////////
    // Main virtual interface to accepting API calls
    ///////////////////////////////////////////////////////////////////////////

    // Data Arrays ////////////////////////////////////////////////////////////

    ANARIArray1D newArray1D(const void *appMemory,
      ANARIMemoryDeleter deleter,
      const void *userdata,
      ANARIDataType,
      uint64_t numItems1) override;

    ANARIArray2D newArray2D(const void *appMemory,
      ANARIMemoryDeleter deleter,
      const void *userdata,
      ANARIDataType,
      uint64_t numItems1,
      uint64_t numItems2) override;

    ANARIArray3D newArray3D(const void *appMemory,
      ANARIMemoryDeleter deleter,
      const void *userdata,
      ANARIDataType,
      uint64_t numItems1,
      uint64_t numItems2,
      uint64_t numItems3) override;

    void* mapArray(ANARIArray) override;
    void unmapArray(ANARIArray) override;

    // Renderable Objects /////////////////////////////////////////////////////

    ANARILight newLight(const char *type) override;

    ANARICamera newCamera(const char *type) override;

    ANARIGeometry newGeometry(const char *type);
    ANARISpatialField newSpatialField(const char *type) override;

    ANARISurface newSurface() override;
    ANARIVolume newVolume(const char *type) override;

    // Model Meta-Data ////////////////////////////////////////////////////////

    ANARIMaterial newMaterial(const char *material_type) override;

    ANARISampler newSampler(const char *type) override;

    // Instancing /////////////////////////////////////////////////////////////

    ANARIGroup newGroup() override;

    ANARIInstance newInstance(const char *type) override;

    // Top-level Worlds ///////////////////////////////////////////////////////

    ANARIWorld newWorld() override;

    // Query functions ////////////////////////////////////////////////////////

    const char ** getObjectSubtypes(ANARIDataType objectType) override;
    const void* getObjectInfo(ANARIDataType objectType,
        const char* objectSubtype,
        const char* infoName,
        ANARIDataType infoType) override;
    const void* getParameterInfo(ANARIDataType objectType,
        const char* objectSubtype,
        const char* parameterName,
        ANARIDataType parameterType,
        const char* infoName,
        ANARIDataType infoType) override;

    // Object + Parameter Lifetime Management /////////////////////////////////

    void setParameter(ANARIObject object,
      const char *name,
      ANARIDataType type,
      const void *mem) override;

    void unsetParameter(ANARIObject object, const char *name) override;
    void unsetAllParameters(ANARIObject o) override;

    void* mapParameterArray1D(ANARIObject o,
        const char* name,
        ANARIDataType dataType,
        uint64_t numElements1,
        uint64_t *elementStride) override;
    void* mapParameterArray2D(ANARIObject o,
        const char* name,
        ANARIDataType dataType,
        uint64_t numElements1,
        uint64_t numElements2,
        uint64_t *elementStride) override;
    void* mapParameterArray3D(ANARIObject o,
        const char* name,
        ANARIDataType dataType,
        uint64_t numElements1,
        uint64_t numElements2,
        uint64_t numElements3,
        uint64_t *elementStride) override;
    void unmapParameterArray(ANARIObject o,
        const char* name) override;

    void commitParameters(ANARIObject object) override;

    void release(ANARIObject _obj) override;
    void retain(ANARIObject _obj) override;

    // Object Query Interface /////////////////////////////////////////////////

    int getProperty(ANARIObject object,
      const char *name,
      ANARIDataType type,
      void *mem,
      uint64_t size,
      uint32_t mask) override;

    // FrameBuffer Manipulation ///////////////////////////////////////////////

    ANARIFrame newFrame() override;

    const void *frameBufferMap(ANARIFrame fb,
        const char *channel,
        uint32_t *width,
        uint32_t *height,
        ANARIDataType *pixelType) override;

    void frameBufferUnmap(ANARIFrame fb, const char *channel) override;

    // Frame Rendering ////////////////////////////////////////////////////////

    ANARIRenderer newRenderer(const char *type) override;

    void renderFrame(ANARIFrame frame) override;
    int frameReady(ANARIFrame, ANARIWaitMask) override { return 1; }
    void discardFrame(ANARIFrame) override {}

    // UsdParameterizedBaseObject interface ///////////////////////////////////////////////////////////

    void filterSetParam(
      const char *name,
      ANARIDataType type,
      const void *mem,
      UsdDevice* device) override;

    void filterResetParam(
      const char *name) override;

    void commit(UsdDevice* device) override;

    void remove(UsdDevice* device) override {}

    // USD Specific ///////////////////////////////////////////////////////////

    bool isInitialized() { return getUsdBridge() != nullptr; }
    UsdBridge* getUsdBridge();

    bool nameExists(const char* name);

    void addToCommitList(UsdBaseObject* object, bool commitData);
    bool isFlushingCommitList() const { return lockCommitList; }

    void addToVolumeList(UsdVolume* volume);
    void removeFromVolumeList(UsdVolume* volume);

    // Allows for selected strings to persist,
    // so their pointers can be cached beyond their containing objects' lifetimes,
    // to be used for garbage collecting resource files.
    void addToResourceStringList(UsdSharedString* sharedString);

#ifdef CHECK_MEMLEAKS
    // Memleak checking
    void logObjAllocation(const UsdBaseObject* ptr);
    void logObjDeallocation(const UsdBaseObject* ptr);
    std::vector<const UsdBaseObject*> allocatedObjects;

    void logStrAllocation(const UsdSharedString* ptr);
    void logStrDeallocation(const UsdSharedString* ptr);
    std::vector<const UsdSharedString*> allocatedStrings;

    void logRawAllocation(const void* ptr);
    void logRawDeallocation(const void* ptr);
    std::vector<const void*> allocatedRawMemory;

    bool isObjAllocated(const UsdBaseObject* ptr) const;
    bool isStrAllocated(const UsdSharedString* ptr) const;
    bool isRawAllocated(const void* ptr) const;
#endif

    void reportStatus(void* source,
      ANARIDataType sourceType,
      ANARIStatusSeverity severity,
      ANARIStatusCode statusCode,
      const char *format, ...);
    void reportStatus(void* source,
      ANARIDataType sourceType,
      ANARIStatusSeverity severity,
      ANARIStatusCode statusCode,
      const char *format,
      va_list& arglist);

  protected:
    UsdBaseObject* getBaseObjectPtr(ANARIObject object);

    // UsdParameterizedBaseObject interface ///////////////////////////////////////////////////////////

    bool deferCommit(UsdDevice* device) { return false; };
    bool doCommitData(UsdDevice* device) { return false; };
    void doCommitRefs(UsdDevice* device) {};

    // USD Specific Cleanup /////////////////////////////////////////////////////////////

    void clearCommitList();
    void flushCommitList();
    void clearDeviceParameters();
    void clearResourceStringList();

    void initializeBridge();

    const char* makeUniqueName(const char* name);

    ANARIArray CreateDataArray(const void *appMemory,
      ANARIMemoryDeleter deleter,
      const void *userData,
      ANARIDataType dataType,
      uint64_t numItems1,
      int64_t byteStride1,
      uint64_t numItems2,
      int64_t byteStride2,
      uint64_t numItems3,
      int64_t byteStride3);

    template<int typeInt>
    void writeTypeToUsd();

    void removePrimsFromUsd(bool onlyRemoveHandles = false);

    std::unique_ptr<UsdDeviceInternals> internals;

    bool bridgeInitAttempt = false;

    // Using object pointers as basis for deferred commits; another option would be to traverse
    // the bridge's internal cache handles, but a handle may map to multiple objects (with the same name)
    // so that's not 1-1 with the effects of a non-deferred commit order.
    using CommitListType = std::pair<helium::IntrusivePtr<UsdBaseObject>, bool>;
    std::vector<CommitListType> commitList;
    std::vector<UsdBaseObject*> removeList;
    std::vector<UsdVolume*> volumeList; // Tracks all volumes to auto-commit when child fields have been committed
    bool lockCommitList = false;

    std::vector<helium::IntrusivePtr<UsdSharedString>> resourceStringList;

    ANARIStatusCallback statusFunc = nullptr;
    const void* statusUserData = nullptr;
    ANARIStatusCallback userSetStatusFunc = nullptr;
    const void* userSetStatusUserData = nullptr;
    std::vector<char> lastStatusMessage;
};

