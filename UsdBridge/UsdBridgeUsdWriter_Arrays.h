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

#if defined(USE_USDRT) && defined(USE_USDRT_ELTTYPE)
#define USDBRIDGE_ARRAYTYPE_ELEMENTTYPE using ElementType = typename ArrayType::element_type;
#else
#define USDBRIDGE_ARRAYTYPE_ELEMENTTYPE using ElementType = typename ArrayType::ElementType;
#endif

namespace
{
  template<typename ArrayType>
  ArrayType& GetStaticTempArray(size_t numElements)
  {
    static ArrayType array;
    array.resize(numElements);
    return array;
  }

  template<typename ElementType>
  void WriteToSpan(const void* data, size_t numElements, UsdBridgeType dataType, UsdBridgeSpanWrI<ElementType>& destSpan)
  {
    ElementType* typedData = (ElementType*)data;
    std::copy(typedData, typedData+numElements, destSpan.begin());
  }

  template<typename ElementType>
  void WriteToSpanFlatten(const void* data, size_t numElements, UsdBridgeType dataType, UsdBridgeSpanWrI<ElementType>& destSpan)
  {
    int elementMultiplier = UsdBridgeTypeNumComponents(dataType);
    size_t numFlattenedElements = numElements * elementMultiplier;

    WriteToSpan<ElementType>(data, numFlattenedElements, dataType, destSpan);
  }

  template<class DestType, class SourceType>
  void WriteToSpanConvert(const void* data, size_t numElements, UsdBridgeType dataType, UsdBridgeSpanWrI<DestType>& destSpan)
  {
    SourceType* typedData = (SourceType*)data;
    DestType* destArray = destSpan.begin();
    for (int i = 0; i < numElements; ++i)
    {
      destArray[i] = DestType(typedData[i]);
    }
  }

  template<class DestType, class SourceType>
  void WriteToSpanConvertFlatten(const void* data, size_t numElements, UsdBridgeType dataType, UsdBridgeSpanWrI<DestType>& destSpan)
  {
    int elementMultiplier = UsdBridgeTypeNumComponents(dataType);
    size_t numFlattenedElements = numElements * elementMultiplier;

    WriteToSpanConvert<DestType, SourceType>(data, numFlattenedElements, dataType, destSpan);
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

#define CREATE_REMOVE_TIMEVARYING_ATTRIB(_prim, _dataMemberId, _token, _type) \
  if(!timeEval || timeEval->IsTimeVarying(_dataMemberId)) \
    _prim.CreateAttribute(_token, _type); \
  else \
    _prim.RemoveProperty(_token);

#define CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(_dataMemberId, _CreateFunc, _token) \
  if(!timeEval || timeEval->IsTimeVarying(_dataMemberId)) \
    attribCreatePrim._CreateFunc(); \
  else \
    attribRemovePrim.RemoveProperty(_token);

#define SET_TIMEVARYING_ATTRIB(_timeVaryingUpdate, _timeVarAttrib, _uniformAttrib, _value) \
  if(_timeVaryingUpdate) \
    _timeVarAttrib.Set(_value, timeEval.TimeCode); \
  else \
    _uniformAttrib.Set(_value, timeEval.Default());

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

      case UsdBridgeType::INT2: { result = SdfValueTypeNames->Int2Array; break; }
      case UsdBridgeType::HALF2: { result = SdfValueTypeNames->Half2Array; break; }
      case UsdBridgeType::FLOAT2: { result = SdfValueTypeNames->Float2Array; break; }
      case UsdBridgeType::DOUBLE2: { result = SdfValueTypeNames->Double2Array; break; }

      case UsdBridgeType::INT3: { result = SdfValueTypeNames->Int3Array; break; }
      case UsdBridgeType::HALF3: { result = SdfValueTypeNames->Half3Array; break; }
      case UsdBridgeType::FLOAT3: { result = SdfValueTypeNames->Float3Array; break; }
      case UsdBridgeType::DOUBLE3: { result = SdfValueTypeNames->Double3Array; break; }

      case UsdBridgeType::INT4: { result = SdfValueTypeNames->Int4Array; break; }
      case UsdBridgeType::HALF4: { result = SdfValueTypeNames->Half4Array; break; }
      case UsdBridgeType::FLOAT4: { result = SdfValueTypeNames->Float4Array; break; }
      case UsdBridgeType::DOUBLE4: { result = SdfValueTypeNames->Double4Array; break; }

      case UsdBridgeType::UCHAR2:
      case UsdBridgeType::UCHAR3:
      case UsdBridgeType::UCHAR4: { result = SdfValueTypeNames->UCharArray; break; }
      case UsdBridgeType::CHAR2:
      case UsdBridgeType::CHAR3:
      case UsdBridgeType::CHAR4: { result = SdfValueTypeNames->UCharArray; break; }
      case UsdBridgeType::USHORT2:
      case UsdBridgeType::USHORT3:
      case UsdBridgeType::USHORT4: { result = SdfValueTypeNames->UIntArray; break; }
      case UsdBridgeType::SHORT2:
      case UsdBridgeType::SHORT3:
      case UsdBridgeType::SHORT4: { result = SdfValueTypeNames->IntArray; break; }
      case UsdBridgeType::UINT2:
      case UsdBridgeType::UINT3:
      case UsdBridgeType::UINT4: { result = SdfValueTypeNames->UIntArray; break; }
      case UsdBridgeType::LONG2:
      case UsdBridgeType::LONG3:
      case UsdBridgeType::LONG4: { result = SdfValueTypeNames->Int64Array; break; }
      case UsdBridgeType::ULONG2:
      case UsdBridgeType::ULONG3:
      case UsdBridgeType::ULONG4: { result = SdfValueTypeNames->UInt64Array; break; }

      default: break; //unsupported defaults to bool
    };

    return result;
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
  class AttribSpan : public UsdBridgeSpanWrI<EltType>
  {
    public:
      AttribSpan(AttribSpanInit& spanInit)
        : SpanInit(spanInit)
        , AttribArray(GetStaticTempArray<VtArray<EltType>>(spanInit.NumElements))
      {}

      EltType* begin() override
      {
        if(AttribArray.size())
          return &AttribArray[0];
        return nullptr;
      }

      EltType* end() override
      {
        if(AttribArray.size())
          return &AttribArray[0]+SpanInit.NumElements;
        return nullptr;
      }

      size_t size() const override
      {
        return AttribArray.size();
      }

      void assignToPrimvar()
      {
        if(SpanInit.Attrib)
          SpanInit.Attrib->Set(AttribArray, SpanInit.TimeCode ? *(SpanInit.TimeCode) : UsdTimeCode::Default());
      }

      AttribSpanInit& SpanInit;
      VtArray<EltType>& AttribArray;
  };

  template<typename SpanInitType, template <typename T> class SpanI, typename SpanEltType>
  void WriteSpanToPrimvar(SpanInitType& destSpanInit, const void* arrayData, size_t arrayNumElements, UsdBridgeType arrayDataType,
    std::function<void(const void*, size_t, UsdBridgeType, UsdBridgeSpanWrI<SpanEltType>&)> writeFn)
  {
    SpanI<SpanEltType> destSpan(destSpanInit); 
    writeFn(arrayData, arrayNumElements, arrayDataType, destSpan); 
    destSpan.assignToPrimvar();
  }
  #define WRITE_SPAN_TO_PRIMVAR_MACRO(SpanEltType)\
    WriteSpanToPrimvar<SpanInitType, SpanI, SpanEltType>(destSpanInit, arrayData, arrayNumElements, arrayDataType, WriteToSpan<SpanEltType>)
  #define WRITE_SPAN_TO_PRIMVAR_FLATTEN_MACRO(SpanEltType)\
    WriteSpanToPrimvar<SpanInitType, SpanI, SpanEltType>(destSpanInit, arrayData, arrayNumElements, arrayDataType, WriteToSpanFlatten<SpanEltType>)
  #define WRITE_SPAN_TO_PRIMVAR_CONVERT_MACRO(SpanEltType, SourceType)\
    WriteSpanToPrimvar<SpanInitType, SpanI, SpanEltType>(destSpanInit, arrayData, arrayNumElements, arrayDataType, WriteToSpanConvert<SpanEltType, SourceType>)
  #define WRITE_SPAN_TO_PRIMVAR_CONVERT_FLATTEN_MACRO(SpanEltType, SourceType)\
    WriteSpanToPrimvar<SpanInitType, SpanI, SpanEltType>(destSpanInit, arrayData, arrayNumElements, arrayDataType, WriteToSpanConvertFlatten<SpanEltType, SourceType>)


  template<typename SpanInitType = AttribSpanInit, template <typename T> class SpanI = AttribSpan>
  void AssignAttribArrayToPrimvar(const UsdBridgeLogObject& logObj, const void* arrayData, UsdBridgeType arrayDataType, size_t arrayNumElements,
    SpanInitType& destSpanInit = SpanInitType(arrayNumElements))
  {
    switch (arrayDataType)
    {
      case UsdBridgeType::UCHAR: { WRITE_SPAN_TO_PRIMVAR_MACRO(unsigned char); break; }
      case UsdBridgeType::UCHAR_SRGB_R: { WRITE_SPAN_TO_PRIMVAR_MACRO(unsigned char); break; }
      case UsdBridgeType::CHAR: { WRITE_SPAN_TO_PRIMVAR_MACRO(unsigned char); break; }
      case UsdBridgeType::USHORT: { WRITE_SPAN_TO_PRIMVAR_CONVERT_MACRO(unsigned int, short); break; }
      case UsdBridgeType::SHORT: { WRITE_SPAN_TO_PRIMVAR_CONVERT_MACRO(int, unsigned short); break; }
      case UsdBridgeType::UINT: { WRITE_SPAN_TO_PRIMVAR_MACRO(unsigned int); break; }
      case UsdBridgeType::INT: { WRITE_SPAN_TO_PRIMVAR_MACRO(int); break; }
      case UsdBridgeType::LONG: { WRITE_SPAN_TO_PRIMVAR_MACRO(int64_t); break; }
      case UsdBridgeType::ULONG: { WRITE_SPAN_TO_PRIMVAR_MACRO(uint64_t); break; }
      case UsdBridgeType::HALF: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfHalf); break; }
      case UsdBridgeType::FLOAT: { WRITE_SPAN_TO_PRIMVAR_MACRO(float); break; }
      case UsdBridgeType::DOUBLE: { WRITE_SPAN_TO_PRIMVAR_MACRO(double); break; }

      case UsdBridgeType::INT2: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfVec2i); break; }
      case UsdBridgeType::FLOAT2: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfVec2f); break; }
      case UsdBridgeType::DOUBLE2: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfVec2d); break; }

      case UsdBridgeType::INT3: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfVec3i); break; }
      case UsdBridgeType::FLOAT3: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfVec3f); break; }
      case UsdBridgeType::DOUBLE3: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfVec3d); break; }

      case UsdBridgeType::INT4: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfVec4i); break; }
      case UsdBridgeType::FLOAT4: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfVec4f); break; }
      case UsdBridgeType::DOUBLE4: { WRITE_SPAN_TO_PRIMVAR_MACRO(GfVec4d); break; }

      case UsdBridgeType::UCHAR2:
      case UsdBridgeType::UCHAR3: 
      case UsdBridgeType::UCHAR4: { WRITE_SPAN_TO_PRIMVAR_FLATTEN_MACRO(unsigned char); break; }
      case UsdBridgeType::UCHAR_SRGB_RA:
      case UsdBridgeType::UCHAR_SRGB_RGB: 
      case UsdBridgeType::UCHAR_SRGB_RGBA: { WRITE_SPAN_TO_PRIMVAR_FLATTEN_MACRO(unsigned char); break; }
      case UsdBridgeType::CHAR2:
      case UsdBridgeType::CHAR3: 
      case UsdBridgeType::CHAR4: { WRITE_SPAN_TO_PRIMVAR_FLATTEN_MACRO(unsigned char); break; }
      case UsdBridgeType::USHORT2:
      case UsdBridgeType::USHORT3:
      case UsdBridgeType::USHORT4: { WRITE_SPAN_TO_PRIMVAR_CONVERT_FLATTEN_MACRO(unsigned int, short); break; }
      case UsdBridgeType::SHORT2:
      case UsdBridgeType::SHORT3: 
      case UsdBridgeType::SHORT4: { WRITE_SPAN_TO_PRIMVAR_CONVERT_FLATTEN_MACRO(int, unsigned short); break; }
      case UsdBridgeType::UINT2:
      case UsdBridgeType::UINT3: 
      case UsdBridgeType::UINT4: { WRITE_SPAN_TO_PRIMVAR_FLATTEN_MACRO(unsigned int); break; }
      case UsdBridgeType::LONG2:
      case UsdBridgeType::LONG3: 
      case UsdBridgeType::LONG4: { WRITE_SPAN_TO_PRIMVAR_FLATTEN_MACRO(int64_t); break; }
      case UsdBridgeType::ULONG2:
      case UsdBridgeType::ULONG3: 
      case UsdBridgeType::ULONG4: { WRITE_SPAN_TO_PRIMVAR_FLATTEN_MACRO(uint64_t); break; }
      case UsdBridgeType::HALF2:
      case UsdBridgeType::HALF3: 
      case UsdBridgeType::HALF4: { WRITE_SPAN_TO_PRIMVAR_FLATTEN_MACRO(GfHalf); break; }

      default: {
        const char* typeStr = ubutils::UsdBridgeTypeToString(arrayDataType);
        UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom Attribute<Index> primvar copy does not support source data type: " << typeStr) 
        break; }
    };
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