// Copyright 2020 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#ifndef UsdBridgeUsdWriterArrays_h
#define UsdBridgeUsdWriterArrays_h

#include "UsdBridgeData.h"
#include "UsdBridgeUtils.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <functional>
#include <type_traits>

#if defined(USE_USDRT) && defined(USE_USDRT_ELTTYPE)
#define USDBRIDGE_ARRAYTYPE_ELEMENTTYPE using ElementType = typename ArrayType::element_type;
#else
#define USDBRIDGE_ARRAYTYPE_ELEMENTTYPE using ElementType = typename ArrayType::ElementType;
#endif

struct UsdBridgeNoneType
{
};

namespace UsdBridgeTypeTraits
{

  DEFINE_USDBRIDGETYPE_SCALARTYPE(HALF, GfHalf)
  DEFINE_USDBRIDGETYPE_SCALARTYPE(HALF2, GfHalf)
  DEFINE_USDBRIDGETYPE_SCALARTYPE(HALF3, GfHalf)
  DEFINE_USDBRIDGETYPE_SCALARTYPE(HALF4, GfHalf)

  template<typename Type>
  struct UsdBaseToBridgeType
  {};
  #define USDBASE_TO_USDBRIDGE_TYPE(TypeUsdBase, TypeBridge)\
    template<>\
    struct UsdBaseToBridgeType<TypeUsdBase>\
    {\
      static constexpr UsdBridgeType Type = TypeBridge;\
    };
  USDBASE_TO_USDBRIDGE_TYPE(bool, UsdBridgeType::BOOL);
  USDBASE_TO_USDBRIDGE_TYPE(uint8_t, UsdBridgeType::UCHAR);
  USDBASE_TO_USDBRIDGE_TYPE(int32_t, UsdBridgeType::INT);
  USDBASE_TO_USDBRIDGE_TYPE(uint32_t, UsdBridgeType::UINT);
  USDBASE_TO_USDBRIDGE_TYPE(int64_t, UsdBridgeType::LONG);
  USDBASE_TO_USDBRIDGE_TYPE(uint64_t, UsdBridgeType::ULONG);
  USDBASE_TO_USDBRIDGE_TYPE(GfHalf, UsdBridgeType::HALF);
  USDBASE_TO_USDBRIDGE_TYPE(float, UsdBridgeType::FLOAT);
  USDBASE_TO_USDBRIDGE_TYPE(double, UsdBridgeType::DOUBLE);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec2i, UsdBridgeType::INT2);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec2h, UsdBridgeType::HALF2);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec2f, UsdBridgeType::FLOAT2);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec2d, UsdBridgeType::DOUBLE2);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec3i, UsdBridgeType::INT3);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec3h, UsdBridgeType::HALF3);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec3f, UsdBridgeType::FLOAT3);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec3d, UsdBridgeType::DOUBLE3);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec4i, UsdBridgeType::INT4);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec4h, UsdBridgeType::HALF4);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec4f, UsdBridgeType::FLOAT4);
  USDBASE_TO_USDBRIDGE_TYPE(GfVec4d, UsdBridgeType::DOUBLE4);
  USDBASE_TO_USDBRIDGE_TYPE(GfQuath, UsdBridgeType::HALF4);
  USDBASE_TO_USDBRIDGE_TYPE(GfQuatf, UsdBridgeType::FLOAT4);
  USDBASE_TO_USDBRIDGE_TYPE(GfQuatd, UsdBridgeType::DOUBLE4);

  template<typename PxrEltType>
  struct PxrToRtEltType
  {};
  #define PXR_ELTTYPE_TO_RT_ELTTYPE(TypePxrElt, TypeRtElt)\
    template<>\
    struct PxrToRtEltType<TypePxrElt>\
    {\
      using Type = TypeRtElt;\
    };
  PXR_ELTTYPE_TO_RT_ELTTYPE(UsdBridgeNoneType, UsdBridgeNoneType)
  PXR_ELTTYPE_TO_RT_ELTTYPE(::PXR_NS::GfVec3f, GfVec3f)
}

namespace
{
  template<typename T0, typename T1>
  struct CanCopyType
  {
    static constexpr bool Value = std::is_same<T0, T1>::value;
  };
  #define DEFINE_CANCOPY_SIGNED_UNSIGNED(Type0, Type1)\
    template<>\
    struct CanCopyType<Type0, Type1>\
    {\
      static constexpr bool Value = true;\
    };\
    template<>\
    struct CanCopyType<Type1, Type0>\
    {\
      static constexpr bool Value = true;\
    };
  DEFINE_CANCOPY_SIGNED_UNSIGNED(char, unsigned char)
  DEFINE_CANCOPY_SIGNED_UNSIGNED(short, unsigned short)
  DEFINE_CANCOPY_SIGNED_UNSIGNED(int, unsigned int)
  DEFINE_CANCOPY_SIGNED_UNSIGNED(int64_t, uint64_t)


  template<typename ArrayType>
  ArrayType& GetStaticTempArray(size_t numElements)
  {
    thread_local static ArrayType array;
    array.resize(numElements);
    return array;
  }

  template<typename SpanInitType, template <typename T> class SpanI, typename AttributeUsdBaseType>
  SpanI<AttributeUsdBaseType>& GetStaticSpan(SpanInitType& destSpanInit)
  {
    thread_local static SpanI<AttributeUsdBaseType> destSpan(destSpanInit);
    destSpan = SpanI<AttributeUsdBaseType>(destSpanInit);
    return destSpan;
  }

  template<typename ElementType>
  void WriteToSpanCopy(const void* data, size_t numElements, UsdBridgeSpanI<ElementType>& destSpan)
  {
    ElementType* typedData = (ElementType*)data;
    std::copy(typedData, typedData+numElements, destSpan.begin());
  }

  template<typename DestType, typename SourceType>
  void WriteToSpanConvert(const void* data, size_t numElements, UsdBridgeSpanI<DestType>& destSpan)
  {
    SourceType* typedData = (SourceType*)data;
    DestType* destArray = destSpan.begin();
    for (int i = 0; i < numElements; ++i)
    {
      destArray[i] = DestType(typedData[i]);
    }
  }

  template<typename DestType, typename SourceType>
  void WriteToSpanConvertQuat(const void* data, size_t numElements, UsdBridgeSpanI<DestType>& destSpan)
  {
    using DestScalarType = typename DestType::ScalarType;
    SourceType* typedSrcData = (SourceType*)data;
    DestType* destArray = destSpan.begin();
    for (int i = 0; i < numElements; ++i)
    {
      destArray[i] = DestType(
        DestScalarType(typedSrcData[i*4+3]), // note that the real component comes first in the quat's constructor
        DestScalarType(typedSrcData[i*4]),
        DestScalarType(typedSrcData[i*4+1]),
        DestScalarType(typedSrcData[i*4+2]));
    }
  }

  template<typename DestType, typename SourceScalarType, int NumComponents>
  void WriteToSpanConvertVector(const void* data, size_t numElements, UsdBridgeSpanI<DestType>& destSpan)
  {
    using DestScalarType = typename DestType::ScalarType;
    const SourceScalarType* typedSrcData = (SourceScalarType*)data;
    DestType* destArray = destSpan.begin();
    for (int i = 0; i < numElements; ++i)
    {
      for(int j = 0; j < NumComponents; ++j)
        destArray[i][j] = DestScalarType(*(typedSrcData++));
    }
  }





    // Array assignment
  template<class ArrayType>
  void AssignArrayToPrimvar(const void* data, size_t numElements, const UsdTimeCode& timeCode, ArrayType* usdArray)
  {
    USDBRIDGE_ARRAYTYPE_ELEMENTTYPE
    ElementType* typedData = (ElementType*)data;
    std::copy(typedData, typedData+numElements, usdArray->begin());
  }

  template<class ArrayType>
  void AssignArrayToPrimvarFlatten(const void* data, UsdBridgeType dataType, size_t numElements, const UsdTimeCode& timeCode, ArrayType* usdArray)
  {
    int elementMultiplier = UsdBridgeTypeNumComponents(dataType);
    size_t numFlattenedElements = numElements * elementMultiplier;

    AssignArrayToPrimvar<ArrayType>(data, numFlattenedElements, timeCode, usdArray);
  }

  template<class ArrayType, class EltType>
  void AssignArrayToPrimvarConvert(const void* data, size_t numElements, const UsdTimeCode& timeCode, ArrayType* usdArray)
  {
    USDBRIDGE_ARRAYTYPE_ELEMENTTYPE
    EltType* typedData = (EltType*)data;

    for (int i = 0; i < numElements; ++i)
    {
      (*usdArray)[i] = ElementType(typedData[i]);
    }
  }

  template<class ArrayType, class EltType>
  void AssignArrayToPrimvarConvertFlatten(const void* data, UsdBridgeType dataType, size_t numElements, const UsdTimeCode& timeCode, ArrayType* usdArray)
  {
    int elementMultiplier = UsdBridgeTypeNumComponents(dataType);
    size_t numFlattenedElements = numElements * elementMultiplier;

    AssignArrayToPrimvarConvert<ArrayType, EltType>(data, numFlattenedElements, timeCode, usdArray);
  }

  template<typename ArrayType, typename EltType>
  void Expand1ToVec3(const void* data, uint64_t numElements, const UsdTimeCode& timeCode, ArrayType* usdArray)
  {
    const EltType* typedInput = reinterpret_cast<const EltType*>(data);
    for (int i = 0; i < numElements; ++i)
    {
      (*usdArray)[i] = typename ArrayType::ElementType(typedInput[i], typedInput[i], typedInput[i]);
    }
  }

  template<typename InputEltType, int numComponents>
  void ExpandToColor(const void* data, uint64_t numElements, const UsdTimeCode& timeCode, VtArray<GfVec4f>* usdArray)
  {
    const InputEltType* typedInput = reinterpret_cast<const InputEltType*>(data);
    // No memcopies, as input is not guaranteed to be of float type
    if(numComponents == 1)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(typedInput[i], 0.0f, 0.0f, 1.0f);
    if(numComponents == 2)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(typedInput[i*2], typedInput[i*2+1], 0.0f, 1.0f);
    if(numComponents == 3)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(typedInput[i*3], typedInput[i*3+1], typedInput[i*3+2], 1.0f);
  }

  template<typename InputEltType, int numComponents>
  void ExpandToColorNormalize(const void* data, uint64_t numElements, const UsdTimeCode& timeCode, VtArray<GfVec4f>* usdArray)
  {
    const InputEltType* typedInput = reinterpret_cast<const InputEltType*>(data);
    double normFactor = 1.0 / (double)std::numeric_limits<InputEltType>::max(); // float may not be enough for uint32_t
    // No memcopies, as input is not guaranteed to be of float type
    if(numComponents == 1)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(typedInput[i]*normFactor, 0.0f, 0.0f, 1.0f);
    if(numComponents == 2)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(typedInput[i*2]*normFactor, typedInput[i*2+1]*normFactor, 0.0f, 1.0f);
    if(numComponents == 3)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(typedInput[i*3]*normFactor, typedInput[i*3+1]*normFactor, typedInput[i*3+2]*normFactor, 1.0f);
    if(numComponents == 4)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(typedInput[i*4]*normFactor, typedInput[i*4+1]*normFactor, typedInput[i*4+2]*normFactor, typedInput[i*4+3]*normFactor);
  }

  template<int numComponents>
  void ExpandSRGBToColor(const void* data, uint64_t numElements, const UsdTimeCode& timeCode, VtArray<GfVec4f>* usdArray)
  {
    const unsigned char* typedInput = reinterpret_cast<const unsigned char*>(data);
    float normFactor = 1.0f / 255.0f;
    const float* srgbTable = ubutils::SrgbToLinearTable();
    // No memcopies, as input is not guaranteed to be of float type
    if(numComponents == 1)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(srgbTable[typedInput[i]], 0.0f, 0.0f, 1.0f);
    if(numComponents == 2)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(srgbTable[typedInput[i*2]], 0.0f, 0.0f, typedInput[i*2+1]*normFactor); // Alpha is linear
    if(numComponents == 3)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(srgbTable[typedInput[i*3]], srgbTable[typedInput[i*3+1]], srgbTable[typedInput[i*3+2]], 1.0f);
    if(numComponents == 4)
      for (int i = 0; i < numElements; ++i)
        (*usdArray)[i] = GfVec4f(srgbTable[typedInput[i*4]], srgbTable[typedInput[i*4+1]], srgbTable[typedInput[i*4+2]], typedInput[i*4+3]*normFactor);
  }
}

#define ASSIGN_SET_PRIMVAR if(setPrimvar) arrayPrimvar.Set(usdArray, timeCode)
#define ASSIGN_PRIMVAR_MACRO(ArrayType) \
  ArrayType& usdArray = GetStaticTempArray<ArrayType>(arrayNumElements); AssignArrayToPrimvar<ArrayType>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_FLATTEN_MACRO(ArrayType) \
  ArrayType& usdArray = GetStaticTempArray<ArrayType>(arrayNumElements); AssignArrayToPrimvarFlatten<ArrayType>(arrayData, arrayDataType, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_CONVERT_MACRO(ArrayType, EltType) \
  ArrayType& usdArray = GetStaticTempArray<ArrayType>(arrayNumElements); AssignArrayToPrimvarConvert<ArrayType, EltType>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_CONVERT_FLATTEN_MACRO(ArrayType, EltType) \
  ArrayType& usdArray = GetStaticTempArray<ArrayType>(arrayNumElements); AssignArrayToPrimvarConvertFlatten<ArrayType, EltType>(arrayData, arrayDataType, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_CUSTOM_ARRAY_MACRO(ArrayType, customArray) \
  ArrayType& usdArray = customArray; AssignArrayToPrimvar<ArrayType>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_CONVERT_CUSTOM_ARRAY_MACRO(ArrayType, EltType, customArray) \
  ArrayType& usdArray = customArray; AssignArrayToPrimvarConvert<ArrayType, EltType>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_1EXPAND3(ArrayType, EltType) \
  ArrayType& usdArray = GetStaticTempArray<ArrayType>(arrayNumElements); Expand1ToVec3<ArrayType, EltType>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_1EXPAND_COL(EltType) \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandToColor<EltType, 1>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_2EXPAND_COL(EltType) \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandToColor<EltType, 2>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_3EXPAND_COL(EltType) \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandToColor<EltType, 3>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_1EXPAND_NORMALIZE_COL(EltType) \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandToColorNormalize<EltType, 1>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_2EXPAND_NORMALIZE_COL(EltType) \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandToColorNormalize<EltType, 2>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_3EXPAND_NORMALIZE_COL(EltType) \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandToColorNormalize<EltType, 3>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_4EXPAND_NORMALIZE_COL(EltType) \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandToColorNormalize<EltType, 4>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_1EXPAND_SGRB() \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandSRGBToColor<1>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_2EXPAND_SGRB() \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandSRGBToColor<2>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_3EXPAND_SGRB() \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandSRGBToColor<3>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR
#define ASSIGN_PRIMVAR_MACRO_4EXPAND_SGRB() \
  VtArray<GfVec4f>& usdArray = GetStaticTempArray<VtArray<GfVec4f>>(arrayNumElements); ExpandSRGBToColor<4>(arrayData, arrayNumElements, timeCode, &usdArray); ASSIGN_SET_PRIMVAR

#define GET_USDARRAY_REF usdArrayRef = &usdArray

namespace
{
  // Assigns color data array to VtArray<GfVec4f> primvar
  VtArray<GfVec4f>* AssignColorArrayToPrimvar(const UsdBridgeLogObject& logObj, const void* arrayData, size_t arrayNumElements, UsdBridgeType arrayType, UsdTimeCode timeCode, UsdAttribute& arrayPrimvar, bool setPrimvar = true)
  {
    VtArray<GfVec4f>* usdArrayRef = nullptr;
    switch (arrayType)
    {
      case UsdBridgeType::UCHAR: {ASSIGN_PRIMVAR_MACRO_1EXPAND_NORMALIZE_COL(uint8_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UCHAR2: {ASSIGN_PRIMVAR_MACRO_2EXPAND_NORMALIZE_COL(uint8_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UCHAR3: {ASSIGN_PRIMVAR_MACRO_3EXPAND_NORMALIZE_COL(uint8_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UCHAR4: {ASSIGN_PRIMVAR_MACRO_4EXPAND_NORMALIZE_COL(uint8_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UCHAR_SRGB_R: {ASSIGN_PRIMVAR_MACRO_1EXPAND_SGRB(); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UCHAR_SRGB_RA: {ASSIGN_PRIMVAR_MACRO_2EXPAND_SGRB(); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UCHAR_SRGB_RGB: {ASSIGN_PRIMVAR_MACRO_3EXPAND_SGRB(); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UCHAR_SRGB_RGBA: {ASSIGN_PRIMVAR_MACRO_4EXPAND_SGRB(); GET_USDARRAY_REF; break; }
      case UsdBridgeType::USHORT: {ASSIGN_PRIMVAR_MACRO_1EXPAND_NORMALIZE_COL(uint16_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::USHORT2: {ASSIGN_PRIMVAR_MACRO_2EXPAND_NORMALIZE_COL(uint16_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::USHORT3: {ASSIGN_PRIMVAR_MACRO_3EXPAND_NORMALIZE_COL(uint16_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::USHORT4: {ASSIGN_PRIMVAR_MACRO_4EXPAND_NORMALIZE_COL(uint16_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UINT: {ASSIGN_PRIMVAR_MACRO_1EXPAND_NORMALIZE_COL(uint32_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UINT2: {ASSIGN_PRIMVAR_MACRO_2EXPAND_NORMALIZE_COL(uint32_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UINT3: {ASSIGN_PRIMVAR_MACRO_3EXPAND_NORMALIZE_COL(uint32_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::UINT4: {ASSIGN_PRIMVAR_MACRO_4EXPAND_NORMALIZE_COL(uint32_t); GET_USDARRAY_REF; break; }
      case UsdBridgeType::FLOAT: {ASSIGN_PRIMVAR_MACRO_1EXPAND_COL(float); GET_USDARRAY_REF; break; }
      case UsdBridgeType::FLOAT2: {ASSIGN_PRIMVAR_MACRO_2EXPAND_COL(float); GET_USDARRAY_REF; break; }
      case UsdBridgeType::FLOAT3: {ASSIGN_PRIMVAR_MACRO_3EXPAND_COL(float); GET_USDARRAY_REF; break; }
      case UsdBridgeType::FLOAT4: {ASSIGN_PRIMVAR_MACRO(VtArray<GfVec4f>); GET_USDARRAY_REF; break; }
      case UsdBridgeType::DOUBLE: {ASSIGN_PRIMVAR_MACRO_1EXPAND_COL(double); GET_USDARRAY_REF; break; }
      case UsdBridgeType::DOUBLE2: {ASSIGN_PRIMVAR_MACRO_2EXPAND_COL(double); GET_USDARRAY_REF; break; }
      case UsdBridgeType::DOUBLE3: {ASSIGN_PRIMVAR_MACRO_3EXPAND_COL(double); GET_USDARRAY_REF; break; }
      case UsdBridgeType::DOUBLE4: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtArray<GfVec4f>, GfVec4d); GET_USDARRAY_REF; break; }
      default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom color primvar is not of type (UCHAR/USHORT/UINT/FLOAT/DOUBLE)(1/2/3/4) or UCHAR_SRGB_<X>."); break; }
    }

    return usdArrayRef;
  }




  struct AttribSpanInit
  {
    AttribSpanInit(size_t numElements,
      UsdAttribute* attrib = nullptr,
      UsdTimeCode* timeCode = nullptr)
      : NumElements(numElements)
      , Attrib(attrib)
      , TimeCode(timeCode)
    {}

    size_t NumElements;
    UsdAttribute* Attrib;
    UsdTimeCode* TimeCode;
  };

  template<typename EltType>
  class AttribSpan : public UsdBridgeSpanI<EltType>
  {
    public:
      AttribSpan(AttribSpanInit& spanInit)
        : SpanInit(spanInit)
        , AttribArray(&GetStaticTempArray<VtArray<EltType>>(spanInit.NumElements))
      {}

      EltType* begin() override
      {
        if(AttribArray->size())
          return &(*AttribArray)[0];
        return nullptr;
      }

      const EltType* begin() const override
      {
        if(AttribArray->size())
          return &(*AttribArray)[0];
        return nullptr;
      }

      EltType* end() override
      {
        if(AttribArray->size())
          return &(*AttribArray)[0]+SpanInit.NumElements;
        return nullptr;
      }

      const EltType* end() const override
      {
        if(AttribArray->size())
          return &(*AttribArray)[0]+SpanInit.NumElements;
        return nullptr;
      }

      size_t size() const override
      {
        return AttribArray->size();
      }

      void assignToPrimvar()
      {
        if(SpanInit.Attrib)
          SpanInit.Attrib->Set(*AttribArray, SpanInit.TimeCode ? *(SpanInit.TimeCode) : UsdTimeCode::Default());
      }

      AttribSpanInit SpanInit;
      VtArray<EltType>* AttribArray;
  };

  template<typename SpanInitType, template <typename T> class SpanI, typename AttributeUsdBaseType, UsdBridgeType ArrayEltType>
  UsdBridgeSpanI<AttributeUsdBaseType>* WriteSpanToPrimvar(const UsdBridgeLogObject& logObj, SpanInitType& destSpanInit, const void* arrayData, size_t arrayNumElements)
  {
    constexpr UsdBridgeType AttributeEltType = typename UsdBridgeTypeTraits::UsdBaseToBridgeType<AttributeUsdBaseType>::Type; 
    constexpr int attributeNumComponents = UsdBridgeTypeTraits::NumComponents<AttributeEltType>::Value;
    constexpr int arrayNumComponents = UsdBridgeTypeTraits::NumComponents<ArrayEltType>::Value;
    using AttributeScalarType = typename UsdBridgeTypeTraits::ScalarType<AttributeEltType>::Type;
    using ArrayScalarType = typename UsdBridgeTypeTraits::ScalarType<ArrayEltType>::Type;

    if constexpr(arrayNumComponents == attributeNumComponents)
    {
      SpanI<AttributeUsdBaseType>& destSpan = GetStaticSpan<SpanInitType, SpanI, AttributeUsdBaseType>(destSpanInit);

      if(arrayNumElements > 0)// Explicitly don't write/assign, just return the span
      {
        constexpr bool canDirectCopy = CanCopyType<ArrayScalarType, AttributeScalarType>::Value;
        constexpr bool canConvert = std::is_constructible<AttributeScalarType, ArrayScalarType>::value;

        if constexpr(canDirectCopy)
          WriteToSpanCopy<AttributeUsdBaseType>(arrayData, arrayNumElements, destSpan);
        else if constexpr(canConvert)
        {
          if constexpr(arrayNumComponents == 1)
            WriteToSpanConvert<AttributeUsdBaseType, ArrayScalarType>(arrayData, arrayNumElements, destSpan);
          else if constexpr(
            std::is_same<AttributeUsdBaseType, GfQuath>::value || 
            std::is_same<AttributeUsdBaseType, GfQuatf>::value ||
            std::is_same<AttributeUsdBaseType, GfQuatd>::value)
            WriteToSpanConvertQuat<AttributeUsdBaseType, ArrayScalarType>(arrayData, arrayNumElements, destSpan);
          else
            WriteToSpanConvertVector<AttributeUsdBaseType, ArrayScalarType, arrayNumComponents>(arrayData, arrayNumElements, destSpan);
        }
        else
        {
          UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "Cannot convert array data of type: " << ubutils::UsdBridgeTypeToString(ArrayEltType) << " to attribute data type: " << ubutils::UsdBridgeTypeToString(AttributeEltType))
          return nullptr;
        }
        destSpan.assignToPrimvar();
      }
      return &destSpan;
    }
    else
    {
      UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "Different number of components between array data type: " << ubutils::UsdBridgeTypeToString(ArrayEltType) << " and attribute data type: " << ubutils::UsdBridgeTypeToString(AttributeEltType));
    }
    return nullptr;
  }

    // Make sure types correspond to AssignAttribArrayToPrimvar()
  SdfValueTypeName GetPrimvarAttribArrayType(UsdBridgeType eltType)
  {
    assert(eltType != UsdBridgeType::UNDEFINED);

    SdfValueTypeName result = SdfValueTypeNames->BoolArray;

    switch (eltType)
    {
      case UsdBridgeType::UCHAR: 
      case UsdBridgeType::CHAR: { result = SdfValueTypeNames->UCharArray; break;}
      case UsdBridgeType::USHORT: { result = SdfValueTypeNames->UIntArray; break; }
      case UsdBridgeType::SHORT: { result = SdfValueTypeNames->IntArray; break; }
      case UsdBridgeType::UINT: { result = SdfValueTypeNames->UIntArray; break; }
      case UsdBridgeType::INT: { result = SdfValueTypeNames->IntArray; break; }
      case UsdBridgeType::LONG: { result = SdfValueTypeNames->Int64Array; break; }
      case UsdBridgeType::ULONG: { result = SdfValueTypeNames->UInt64Array; break; }
      case UsdBridgeType::HALF: { result = SdfValueTypeNames->HalfArray; break; }
      case UsdBridgeType::FLOAT: { result = SdfValueTypeNames->FloatArray; break; }
      case UsdBridgeType::DOUBLE: { result = SdfValueTypeNames->DoubleArray; break; }

      case UsdBridgeType::UCHAR2:
      case UsdBridgeType::CHAR2:
      case UsdBridgeType::USHORT2:
      case UsdBridgeType::SHORT2:
      case UsdBridgeType::UINT2:
      case UsdBridgeType::INT2: { result = SdfValueTypeNames->Int2Array; break; }
      case UsdBridgeType::HALF2: { result = SdfValueTypeNames->Half2Array; break; }
      case UsdBridgeType::FLOAT2: { result = SdfValueTypeNames->Float2Array; break; }
      case UsdBridgeType::DOUBLE2: { result = SdfValueTypeNames->Double2Array; break; }

      case UsdBridgeType::UCHAR3:
      case UsdBridgeType::CHAR3:
      case UsdBridgeType::USHORT3:
      case UsdBridgeType::SHORT3:
      case UsdBridgeType::UINT3:
      case UsdBridgeType::INT3: { result = SdfValueTypeNames->Int3Array; break; }
      case UsdBridgeType::HALF3: { result = SdfValueTypeNames->Half3Array; break; }
      case UsdBridgeType::FLOAT3: { result = SdfValueTypeNames->Float3Array; break; }
      case UsdBridgeType::DOUBLE3: { result = SdfValueTypeNames->Double3Array; break; }

      case UsdBridgeType::UCHAR4:
      case UsdBridgeType::CHAR4:
      case UsdBridgeType::USHORT4:
      case UsdBridgeType::SHORT4:
      case UsdBridgeType::UINT4:
      case UsdBridgeType::INT4: { result = SdfValueTypeNames->Int4Array; break; }
      case UsdBridgeType::HALF4: { result = SdfValueTypeNames->Half4Array; break; }
      case UsdBridgeType::FLOAT4: { result = SdfValueTypeNames->Float4Array; break; }
      case UsdBridgeType::DOUBLE4: { result = SdfValueTypeNames->Double4Array; break; }

      default: break; //unsupported defaults to bool
    };

    return result;
  }

  template<typename SpanInitType, template <typename T> class SpanI,
    typename AttributeUsdBaseType, UsdBridgeType ArrayEltType, typename ReturnEltType>
  void WriteSpanToPrimvar_Filter(const UsdBridgeLogObject& logObj, 
    SpanInitType& destSpanInit, const void* arrayData, size_t arrayNumElements, UsdBridgeSpanI<ReturnEltType>*& resultSpan)
  {
    UsdBridgeSpanI<AttributeUsdBaseType>* writeSpan = WriteSpanToPrimvar<SpanInitType, SpanI, AttributeUsdBaseType, ArrayEltType>
      (logObj, destSpanInit, arrayData, arrayNumElements);

    if constexpr(std::is_same<ReturnEltType, AttributeUsdBaseType>::value)
      resultSpan = writeSpan;
    else if constexpr(!std::is_same<ReturnEltType, UsdBridgeNoneType>::value)
    {
      UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, 
        "Implementation error: Assumed element type of requested attribute span does not equal type of attribute itself. No span will be returned.")
    }
  }

  #define WRITE_SPAN_TO_PRIMVAR(ArrayEltType)\
    case ArrayEltType:\
    {\
      WriteSpanToPrimvar_Filter<SpanInitType, SpanI, AttributeUsdBaseType, ArrayEltType, ReturnEltType>\
        (logObj, destSpanInit, arrayData, arrayNumElements, resultSpan);\
      break;\
    }

  template<typename AttributeUsdBaseType, typename SpanInitType, template <typename T> class SpanI, typename ReturnEltType>
  UsdBridgeSpanI<ReturnEltType>* AssignArrayToTypedAttribute(const UsdBridgeLogObject& logObj, 
    SpanInitType& destSpanInit, const void* arrayData, UsdBridgeType arrayDataType, size_t arrayNumElements)
  {
    UsdBridgeSpanI<ReturnEltType>* resultSpan = nullptr;
    switch (arrayDataType)
    {
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::UCHAR)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::CHAR)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::USHORT)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::SHORT)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::UINT)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::INT)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::LONG)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::ULONG)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::HALF)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::FLOAT)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::DOUBLE)

      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::UCHAR2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::CHAR2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::USHORT2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::SHORT2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::INT2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::UINT2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::LONG2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::ULONG2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::HALF2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::FLOAT2)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::DOUBLE2)

      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::UCHAR3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::CHAR3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::USHORT3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::SHORT3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::INT3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::UINT3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::LONG3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::ULONG3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::HALF3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::FLOAT3)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::DOUBLE3)

      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::UCHAR4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::CHAR4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::USHORT4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::SHORT4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::INT4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::UINT4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::LONG4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::ULONG4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::HALF4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::FLOAT4)
      WRITE_SPAN_TO_PRIMVAR(UsdBridgeType::DOUBLE4)

      default:
      {
        UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "No support for writing array data of type: " << ubutils::UsdBridgeTypeToString(arrayDataType) << " to a UsdAttribute.")
        break;
      }
    };
    return resultSpan;
  }

  #define MAP_VALUETYPE_TO_BASETYPE(SdfVTN, TypeUsdBase)\
    ValueTypeToAssignArrayMap.emplace(::PXR_NS::SdfVTN.GetType(), AssignArrayToTypedAttribute<TypeUsdBase, SpanInitType, SpanI, ReturnEltType>);

  // The assign array map essentially determines for any runtime attribute sdfvaluetype, which span element type should be employed (completing the span type)
  template<typename SpanInitType, template <typename T> class SpanI, typename ReturnEltType>
  class AssignArrayMapType
  {
    public:
      using ArrayToTypeAttributeFunc = std::function<UsdBridgeSpanI<ReturnEltType>*(const UsdBridgeLogObject& logObj, SpanInitType& destSpanInit, const void* arrayData, UsdBridgeType arrayDataType, size_t arrayNumElements)>;
      std::map<::PXR_NS::TfType, ArrayToTypeAttributeFunc> ValueTypeToAssignArrayMap;

      AssignArrayMapType()
      {
        // See: https://docs.omniverse.nvidia.com/usd/latest/technical_reference/usd-types.html
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->BoolArray, bool)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->UCharArray, uint8_t)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->IntArray, int32_t)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->UIntArray, uint32_t)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Int64Array, int64_t)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->UInt64Array, uint64_t)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->HalfArray, GfHalf)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->FloatArray, float)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->DoubleArray, double)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Int2Array, GfVec2i)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Int3Array, GfVec3i)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Int4Array, GfVec4i)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Half2Array, GfVec2h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Half3Array, GfVec3h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Half4Array, GfVec4h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Float2Array, GfVec2f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Float3Array, GfVec3f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Float4Array, GfVec4f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Double2Array, GfVec2d)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Double3Array, GfVec3d)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Double4Array, GfVec4d)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Point3hArray, GfVec3h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Point3fArray, GfVec3f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Point3dArray, GfVec3d)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Vector3hArray, GfVec3h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Vector3fArray, GfVec3f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Vector3dArray, GfVec3d)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Normal3hArray, GfVec3h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Normal3fArray, GfVec3f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Normal3dArray, GfVec3d)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Color3hArray, GfVec3h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Color3fArray, GfVec3f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Color3dArray, GfVec3d)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Color4hArray, GfVec4h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Color4fArray, GfVec4f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->Color4dArray, GfVec4d)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->QuathArray, GfQuath)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->QuatfArray, GfQuatf)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->QuatdArray, GfQuatd)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->TexCoord2hArray, GfVec2h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->TexCoord2fArray, GfVec2f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->TexCoord2dArray, GfVec2d)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->TexCoord3hArray, GfVec3h)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->TexCoord3fArray, GfVec3f)
        MAP_VALUETYPE_TO_BASETYPE(SdfValueTypeNames->TexCoord3dArray, GfVec3d)
      }
  };

  template<typename SpanInitType = AttribSpanInit, template <typename T> class SpanI = AttribSpan, typename ReturnEltType = UsdBridgeNoneType>
  UsdBridgeSpanI<ReturnEltType>* AssignArrayToAttribute(const UsdBridgeLogObject& logObj, const void* arrayData, UsdBridgeType arrayDataType, size_t arrayNumElements, 
    const ::PXR_NS::SdfValueTypeName& attribValueType, SpanInitType& destSpanInit = SpanInitType(arrayNumElements))
  {
    static AssignArrayMapType<SpanInitType, SpanI, ReturnEltType> assignArrayMap;

    // Find the array assignment specialization for the runtime attribute value type, and execute it if available
    auto assignArrayEntry = assignArrayMap.ValueTypeToAssignArrayMap.find(attribValueType.GetType());
    if(assignArrayEntry != assignArrayMap.ValueTypeToAssignArrayMap.end())
    {
      auto resultSpan = assignArrayEntry->second(logObj, destSpanInit, arrayData, arrayDataType, arrayNumElements);

      // If the caller expects a span and none is returned, raise an error message
      if constexpr(!std::is_same<ReturnEltType, UsdBridgeNoneType>::value)
      {
        if(!resultSpan)
        {
          UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, 
            "UsdGeom PointsAttr did not return writeable memory after update;\
            data array and attribute types cannot be converted or are not supported, or the span and attribute types do not match - see previous errors");
        }
      }
      return resultSpan;
    }
    else
    {
      UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "No support for writing to UsdAttribute with SdfValueTypeName: " << attribValueType.GetAsToken().GetText())
    }

    return nullptr;
  }




  // Make sure types correspond to GetPrimvarAttribArrayType()
  void AssignAttribArrayToPrimvar(const UsdBridgeLogObject& logObj, const void* arrayData, UsdBridgeType arrayDataType, size_t arrayNumElements, UsdAttribute& arrayPrimvar, const UsdTimeCode& timeCode)
  {
    bool setPrimvar = true;
    switch (arrayDataType)
    {
      case UsdBridgeType::UCHAR: { ASSIGN_PRIMVAR_MACRO(VtArray<unsigned char>); break; }
      case UsdBridgeType::UCHAR_SRGB_R: { ASSIGN_PRIMVAR_MACRO(VtArray<unsigned char>); break; }
      case UsdBridgeType::CHAR: { ASSIGN_PRIMVAR_MACRO(VtArray<unsigned char>); break; }
      case UsdBridgeType::USHORT: { ASSIGN_PRIMVAR_CONVERT_MACRO(VtArray<unsigned int>, short); break; }
      case UsdBridgeType::SHORT: { ASSIGN_PRIMVAR_CONVERT_MACRO(VtArray<int>, unsigned short); break; }
      case UsdBridgeType::UINT: { ASSIGN_PRIMVAR_MACRO(VtArray<unsigned int>); break; }
      case UsdBridgeType::INT: { ASSIGN_PRIMVAR_MACRO(VtArray<int>); break; }
      case UsdBridgeType::LONG: { ASSIGN_PRIMVAR_MACRO(VtArray<int64_t>); break; }
      case UsdBridgeType::ULONG: { ASSIGN_PRIMVAR_MACRO(VtArray<uint64_t>); break; }
      case UsdBridgeType::HALF: { ASSIGN_PRIMVAR_MACRO(VtArray<GfHalf>); break; }
      case UsdBridgeType::FLOAT: { ASSIGN_PRIMVAR_MACRO(VtArray<float>); break; }
      case UsdBridgeType::DOUBLE: { ASSIGN_PRIMVAR_MACRO(VtArray<double>); break; }

      case UsdBridgeType::INT2: { ASSIGN_PRIMVAR_MACRO(VtArray<GfVec2i>); break; }
      case UsdBridgeType::FLOAT2: { ASSIGN_PRIMVAR_MACRO(VtArray<GfVec2f>); break; }
      case UsdBridgeType::DOUBLE2: { ASSIGN_PRIMVAR_MACRO(VtArray<GfVec2d>); break; }

      case UsdBridgeType::INT3: { ASSIGN_PRIMVAR_MACRO(VtArray<GfVec3i>); break; }
      case UsdBridgeType::FLOAT3: { ASSIGN_PRIMVAR_MACRO(VtArray<GfVec3f>); break; }
      case UsdBridgeType::DOUBLE3: { ASSIGN_PRIMVAR_MACRO(VtArray<GfVec3d>); break; }

      case UsdBridgeType::INT4: { ASSIGN_PRIMVAR_MACRO(VtArray<GfVec4i>); break; }
      case UsdBridgeType::FLOAT4: { ASSIGN_PRIMVAR_MACRO(VtArray<GfVec4f>); break; }
      case UsdBridgeType::DOUBLE4: { ASSIGN_PRIMVAR_MACRO(VtArray<GfVec4d>); break; }

      case UsdBridgeType::UCHAR2:
      case UsdBridgeType::UCHAR3: 
      case UsdBridgeType::UCHAR4: { ASSIGN_PRIMVAR_FLATTEN_MACRO(VtArray<unsigned char>); break; }
      case UsdBridgeType::UCHAR_SRGB_RA:
      case UsdBridgeType::UCHAR_SRGB_RGB: 
      case UsdBridgeType::UCHAR_SRGB_RGBA: { ASSIGN_PRIMVAR_FLATTEN_MACRO(VtArray<unsigned char>); break; }
      case UsdBridgeType::CHAR2:
      case UsdBridgeType::CHAR3: 
      case UsdBridgeType::CHAR4: { ASSIGN_PRIMVAR_FLATTEN_MACRO(VtArray<unsigned char>); break; }
      case UsdBridgeType::USHORT2:
      case UsdBridgeType::USHORT3:
      case UsdBridgeType::USHORT4: { ASSIGN_PRIMVAR_CONVERT_FLATTEN_MACRO(VtArray<unsigned int>, short); break; }
      case UsdBridgeType::SHORT2:
      case UsdBridgeType::SHORT3: 
      case UsdBridgeType::SHORT4: { ASSIGN_PRIMVAR_CONVERT_FLATTEN_MACRO(VtArray<int>, unsigned short); break; }
      case UsdBridgeType::UINT2:
      case UsdBridgeType::UINT3: 
      case UsdBridgeType::UINT4: { ASSIGN_PRIMVAR_FLATTEN_MACRO(VtArray<unsigned int>); break; }
      case UsdBridgeType::LONG2:
      case UsdBridgeType::LONG3: 
      case UsdBridgeType::LONG4: { ASSIGN_PRIMVAR_FLATTEN_MACRO(VtArray<int64_t>); break; }
      case UsdBridgeType::ULONG2:
      case UsdBridgeType::ULONG3: 
      case UsdBridgeType::ULONG4: { ASSIGN_PRIMVAR_FLATTEN_MACRO(VtArray<uint64_t>); break; }
      case UsdBridgeType::HALF2:
      case UsdBridgeType::HALF3: 
      case UsdBridgeType::HALF4: { ASSIGN_PRIMVAR_FLATTEN_MACRO(VtArray<GfHalf>); break; }

      default: {
        const char* typeStr = ubutils::UsdBridgeTypeToString(arrayDataType);
        UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom Attribute<Index> primvar copy does not support source data type: " << typeStr) 
        break; }
    };
  }
}

#endif