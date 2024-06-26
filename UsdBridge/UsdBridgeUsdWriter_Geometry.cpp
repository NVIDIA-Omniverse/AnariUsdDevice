// Copyright 2020 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#include "UsdBridgeUsdWriter.h"

#include "UsdBridgeUsdWriter_Common.h"

namespace
{
  template<typename UsdGeomType>
  UsdAttribute UsdGeomGetPointsAttribute(UsdGeomType& usdGeom) { return UsdAttribute(); }

  template<>
  UsdAttribute UsdGeomGetPointsAttribute(UsdGeomMesh& usdGeom) { return usdGeom.GetPointsAttr(); }

  template<>
  UsdAttribute UsdGeomGetPointsAttribute(UsdGeomPoints& usdGeom) { return usdGeom.GetPointsAttr(); }

  template<>
  UsdAttribute UsdGeomGetPointsAttribute(UsdGeomBasisCurves& usdGeom) { return usdGeom.GetPointsAttr(); }

  template<>
  UsdAttribute UsdGeomGetPointsAttribute(UsdGeomPointInstancer& usdGeom) { return usdGeom.GetPositionsAttr(); }

  template<typename GeomDataType>
  void CreateUsdGeomColorPrimvars(UsdGeomPrimvarsAPI& primvarApi, const GeomDataType& geomData, const UsdBridgeSettings& settings, const TimeEvaluator<GeomDataType>* timeEval = nullptr)
  {
    using DMI = typename GeomDataType::DataMemberId;

    bool timeVarChecked = true;
    if(timeEval)
    {
      timeVarChecked = timeEval->IsTimeVarying(DMI::COLORS);
    }

    if (timeVarChecked)
    {
      primvarApi.CreatePrimvar(UsdBridgeTokens->color, SdfValueTypeNames->Float4Array);
    }
    else
    {
      primvarApi.RemovePrimvar(UsdBridgeTokens->color);
    }
  }

  template<typename GeomDataType>
  void CreateUsdGeomTexturePrimvars(UsdGeomPrimvarsAPI& primvarApi, const GeomDataType& geomData, const UsdBridgeSettings& settings, const TimeEvaluator<GeomDataType>* timeEval = nullptr)
  {
    using DMI = typename GeomDataType::DataMemberId;

    bool timeVarChecked = true;
    if(timeEval)
    {
      timeVarChecked = timeEval->IsTimeVarying(DMI::ATTRIBUTE0);
    }

    if (timeVarChecked)
      primvarApi.CreatePrimvar(UsdBridgeTokens->st, SdfValueTypeNames->TexCoord2fArray);
    else if (timeEval)
      primvarApi.RemovePrimvar(UsdBridgeTokens->st);
  }

  template<typename GeomDataType>
  void CreateUsdGeomAttributePrimvar(UsdBridgeUsdWriter* writer, UsdGeomPrimvarsAPI& primvarApi, const GeomDataType& geomData, uint32_t attribIndex, const TimeEvaluator<GeomDataType>* timeEval = nullptr)
  {
    using DMI = typename GeomDataType::DataMemberId;

    const UsdBridgeAttribute& attrib = geomData.Attributes[attribIndex];
    if(attrib.DataType != UsdBridgeType::UNDEFINED)
    {
      bool timeVarChecked = true;
      if(timeEval)
      {
        DMI attributeId = DMI::ATTRIBUTE0 + attribIndex;
        timeVarChecked = timeEval->IsTimeVarying(attributeId);
      }

      TfToken attribToken = attrib.Name ? writer->AttributeNameToken(attrib.Name) : AttribIndexToToken(attribIndex);

      if(timeVarChecked)
      {
        SdfValueTypeName primvarType = GetPrimvarArrayType(attrib.DataType);
        if(primvarType == SdfValueTypeNames->BoolArray)
        {
          UsdBridgeLogMacro(writer->LogObject, UsdBridgeLogLevel::WARNING, "UsdGeom Attribute<" << attribIndex << "> primvar does not support source data type: " << attrib.DataType);
        }

        // Allow for attrib primvar types to change
        UsdGeomPrimvar primvar = primvarApi.GetPrimvar(attribToken);
        if(primvar && primvar.GetTypeName() != primvarType)
        {
          primvarApi.RemovePrimvar(attribToken);
        }
        primvarApi.CreatePrimvar(attribToken, primvarType);
      }
      else if(timeEval)
      {
        primvarApi.RemovePrimvar(attribToken);
      }
    }
  }

  template<typename GeomDataType>
  void CreateUsdGeomAttributePrimvars(UsdBridgeUsdWriter* writer, UsdGeomPrimvarsAPI& primvarApi, const GeomDataType& geomData, const TimeEvaluator<GeomDataType>* timeEval = nullptr)
  {
    for(uint32_t attribIndex = 0; attribIndex < geomData.NumAttributes; ++attribIndex)
    {
      CreateUsdGeomAttributePrimvar(writer, primvarApi, geomData, attribIndex, timeEval);
    }
  }

  void InitializeUsdGeometryTimeVar(UsdBridgeUsdWriter* writer, UsdGeomMesh& meshGeom, const UsdBridgeMeshData& meshData, const UsdBridgeSettings& settings,
    const TimeEvaluator<UsdBridgeMeshData>* timeEval = nullptr)
  {
    typedef UsdBridgeMeshData::DataMemberId DMI;
    UsdGeomPrimvarsAPI primvarApi(meshGeom);

    UsdGeomMesh& attribCreatePrim = meshGeom;
    UsdPrim attribRemovePrim = meshGeom.GetPrim();

    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::POINTS, CreatePointsAttr, UsdBridgeTokens->points);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::POINTS, CreateExtentAttr, UsdBridgeTokens->extent);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::INDICES, CreateFaceVertexIndicesAttr, UsdBridgeTokens->faceVertexCounts);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::INDICES, CreateFaceVertexCountsAttr, UsdBridgeTokens->faceVertexIndices);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::NORMALS, CreateNormalsAttr, UsdBridgeTokens->normals);

    CreateUsdGeomColorPrimvars(primvarApi, meshData, settings, timeEval);

    if(settings.EnableStTexCoords)
      CreateUsdGeomTexturePrimvars(primvarApi, meshData, settings, timeEval);

    CreateUsdGeomAttributePrimvars(writer, primvarApi, meshData, timeEval);
  }

  void InitializeUsdGeometryTimeVar(UsdBridgeUsdWriter* writer, UsdGeomPoints& pointsGeom, const UsdBridgeInstancerData& instancerData, const UsdBridgeSettings& settings,
    const TimeEvaluator<UsdBridgeInstancerData>* timeEval = nullptr)
  {
    typedef UsdBridgeInstancerData::DataMemberId DMI;
    UsdGeomPrimvarsAPI primvarApi(pointsGeom);

    UsdGeomPoints& attribCreatePrim = pointsGeom;
    UsdPrim attribRemovePrim = pointsGeom.GetPrim();

    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::POINTS, CreatePointsAttr, UsdBridgeTokens->points);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::POINTS, CreateExtentAttr, UsdBridgeTokens->extent);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::INSTANCEIDS, CreateIdsAttr, UsdBridgeTokens->ids);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::ORIENTATIONS, CreateNormalsAttr, UsdBridgeTokens->normals);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::SCALES, CreateWidthsAttr, UsdBridgeTokens->widths);

    CreateUsdGeomColorPrimvars(primvarApi, instancerData, settings, timeEval);

    if(settings.EnableStTexCoords)
      CreateUsdGeomTexturePrimvars(primvarApi, instancerData, settings, timeEval);

    CreateUsdGeomAttributePrimvars(writer, primvarApi, instancerData, timeEval);
  }

  void InitializeUsdGeometryTimeVar(UsdBridgeUsdWriter* writer, UsdGeomPointInstancer& pointsGeom, const UsdBridgeInstancerData& instancerData, const UsdBridgeSettings& settings,
    const TimeEvaluator<UsdBridgeInstancerData>* timeEval = nullptr)
  {
    typedef UsdBridgeInstancerData::DataMemberId DMI;
    UsdGeomPrimvarsAPI primvarApi(pointsGeom);

    UsdGeomPointInstancer& attribCreatePrim = pointsGeom;
    UsdPrim attribRemovePrim = pointsGeom.GetPrim();

    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::POINTS, CreatePositionsAttr, UsdBridgeTokens->positions);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::POINTS, CreateExtentAttr, UsdBridgeTokens->extent);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::SHAPEINDICES, CreateProtoIndicesAttr, UsdBridgeTokens->protoIndices);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::INSTANCEIDS, CreateIdsAttr, UsdBridgeTokens->ids);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::ORIENTATIONS, CreateOrientationsAttr, UsdBridgeTokens->orientations);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::SCALES, CreateScalesAttr, UsdBridgeTokens->scales);

    CreateUsdGeomColorPrimvars(primvarApi, instancerData, settings, timeEval);

    if(settings.EnableStTexCoords)
      CreateUsdGeomTexturePrimvars(primvarApi, instancerData, settings, timeEval);

    CreateUsdGeomAttributePrimvars(writer, primvarApi, instancerData, timeEval);

    //CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::LINEARVELOCITIES, CreateVelocitiesAttr, UsdBridgeTokens->velocities);
    //CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::ANGULARVELOCITIES, CreateAngularVelocitiesAttr, UsdBridgeTokens->angularVelocities);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::INVISIBLEIDS, CreateInvisibleIdsAttr, UsdBridgeTokens->invisibleIds);
  }

  void InitializeUsdGeometryTimeVar(UsdBridgeUsdWriter* writer, UsdGeomBasisCurves& curveGeom, const UsdBridgeCurveData& curveData, const UsdBridgeSettings& settings,
    const TimeEvaluator<UsdBridgeCurveData>* timeEval = nullptr)
  {
    typedef UsdBridgeCurveData::DataMemberId DMI;
    UsdGeomPrimvarsAPI primvarApi(curveGeom);

    UsdGeomBasisCurves& attribCreatePrim = curveGeom;
    UsdPrim attribRemovePrim = curveGeom.GetPrim();

    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::POINTS, CreatePointsAttr, UsdBridgeTokens->positions);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::POINTS, CreateExtentAttr, UsdBridgeTokens->extent);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::CURVELENGTHS, CreateCurveVertexCountsAttr, UsdBridgeTokens->curveVertexCounts);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::NORMALS, CreateNormalsAttr, UsdBridgeTokens->normals);
    CREATE_REMOVE_TIMEVARYING_ATTRIB_QUALIFIED(DMI::SCALES, CreateWidthsAttr, UsdBridgeTokens->widths);

    CreateUsdGeomColorPrimvars(primvarApi, curveData, settings, timeEval);

    if(settings.EnableStTexCoords)
      CreateUsdGeomTexturePrimvars(primvarApi, curveData, settings, timeEval);

    CreateUsdGeomAttributePrimvars(writer, primvarApi, curveData, timeEval);

  }

  UsdPrim InitializeUsdGeometry_Impl(UsdBridgeUsdWriter* writer, UsdStageRefPtr geometryStage, const SdfPath& geomPath, const UsdBridgeMeshData& meshData, bool uniformPrim,
    const UsdBridgeSettings& settings,
    TimeEvaluator<UsdBridgeMeshData>* timeEval = nullptr)
  {
    UsdGeomMesh geomMesh = GetOrDefinePrim<UsdGeomMesh>(geometryStage, geomPath);

    InitializeUsdGeometryTimeVar(writer, geomMesh, meshData, settings, timeEval);

    if (uniformPrim)
    {
      geomMesh.CreateDoubleSidedAttr(VtValue(true));
      geomMesh.CreateSubdivisionSchemeAttr().Set(UsdGeomTokens->none);
    }

    return geomMesh.GetPrim();
  }

  UsdPrim InitializeUsdGeometry_Impl(UsdBridgeUsdWriter* writer, UsdStageRefPtr geometryStage, const SdfPath& geomPath, const UsdBridgeInstancerData& instancerData, bool uniformPrim,
    const UsdBridgeSettings& settings,
    TimeEvaluator<UsdBridgeInstancerData>* timeEval = nullptr)
  {
    if (instancerData.UseUsdGeomPoints)
    {
      UsdGeomPoints geomPoints = GetOrDefinePrim<UsdGeomPoints>(geometryStage, geomPath);

      InitializeUsdGeometryTimeVar(writer, geomPoints, instancerData, settings, timeEval);

      if (uniformPrim)
      {
        geomPoints.CreateDoubleSidedAttr(VtValue(true));
      }

      return geomPoints.GetPrim();
    }
    else
    {
      UsdGeomPointInstancer geomPoints = GetOrDefinePrim<UsdGeomPointInstancer>(geometryStage, geomPath);

      InitializeUsdGeometryTimeVar(writer, geomPoints, instancerData, settings, timeEval);

      return geomPoints.GetPrim();
    }
  }

  UsdPrim InitializeUsdGeometry_Impl(UsdBridgeUsdWriter* writer, UsdStageRefPtr geometryStage, const SdfPath& geomPath, const UsdBridgeCurveData& curveData, bool uniformPrim,
    const UsdBridgeSettings& settings,
    TimeEvaluator<UsdBridgeCurveData>* timeEval = nullptr)
  {
    UsdGeomBasisCurves geomCurves = GetOrDefinePrim<UsdGeomBasisCurves>(geometryStage, geomPath);

    InitializeUsdGeometryTimeVar(writer, geomCurves, curveData, settings, timeEval);

    if (uniformPrim)
    {
      geomCurves.CreateDoubleSidedAttr(VtValue(true));
      geomCurves.GetTypeAttr().Set(UsdGeomTokens->linear);
    }

    return geomCurves.GetPrim();
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomPoints(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::POINTS);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::POINTS);

    ClearUsdAttributes(UsdGeomGetPointsAttribute(uniformGeom), UsdGeomGetPointsAttribute(timeVarGeom), timeVaryingUpdate);
    ClearUsdAttributes(uniformGeom.GetExtentAttr(), timeVarGeom.GetExtentAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      if (!geomData.Points)
      {
        UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "GeomData requires points.");
      }
      else
      {
        UsdGeomType* outGeom = timeVaryingUpdate ? &timeVarGeom : &uniformGeom;
        UsdTimeCode timeCode = timeEval.Eval(DMI::POINTS);

        // Points
        UsdAttribute pointsAttr = UsdGeomGetPointsAttribute(*outGeom);

        const void* arrayData = geomData.Points;
        size_t arrayNumElements = geomData.NumPoints;
        UsdAttribute arrayPrimvar = pointsAttr;
        VtVec3fArray& usdVerts = GetStaticTempArray<VtVec3fArray>();
        bool setPrimvar = true;

        switch (geomData.PointsType)
        {
        case UsdBridgeType::FLOAT3: {ASSIGN_PRIMVAR_CUSTOM_ARRAY_MACRO(VtVec3fArray, usdVerts); break; }
        case UsdBridgeType::DOUBLE3: {ASSIGN_PRIMVAR_CONVERT_CUSTOM_ARRAY_MACRO(VtVec3fArray, GfVec3d, usdVerts); break; }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom PointsAttr should be FLOAT3 or DOUBLE3."); break; }
        }

        // Usd requires extent.
        GfRange3f extent;
        for (const auto& pt : usdVerts) {
          extent.UnionWith(pt);
        }
        VtVec3fArray extentArray(2);
        extentArray[0] = extent.GetMin();
        extentArray[1] = extent.GetMax();

        outGeom->GetExtentAttr().Set(extentArray, timeCode);
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomIndices(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::INDICES);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::INDICES);

    ClearUsdAttributes(uniformGeom.GetFaceVertexIndicesAttr(), timeVarGeom.GetFaceVertexIndicesAttr(), timeVaryingUpdate);
    ClearUsdAttributes(uniformGeom.GetFaceVertexCountsAttr(), timeVarGeom.GetFaceVertexCountsAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType* outGeom = timeVaryingUpdate ? &timeVarGeom : &uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::INDICES);

      uint64_t numIndices = geomData.NumIndices;

      VtArray<int>& usdVertexCounts = GetStaticTempArray<VtIntArray>();
      usdVertexCounts.resize(numPrims);
      int vertexCount = numIndices / numPrims;
      for (uint64_t i = 0; i < numPrims; ++i)
        usdVertexCounts[i] = vertexCount;//geomData.FaceVertCounts[i];

      // Face Vertex counts
      UsdAttribute faceVertCountsAttr = outGeom->GetFaceVertexCountsAttr();
      faceVertCountsAttr.Set(usdVertexCounts, timeCode);

      if (!geomData.Indices)
      {
        VtIntArray& tempIndices = GetStaticTempArray<VtIntArray>();
        tempIndices.resize(numIndices);
        for (uint64_t i = 0; i < numIndices; ++i)
          tempIndices[i] = (int)i;

        UsdAttribute arrayPrimvar = outGeom->GetFaceVertexIndicesAttr();
        arrayPrimvar.Set(tempIndices, timeCode);
      }
      else
      {
        // Face indices
        const void* arrayData = geomData.Indices;
        size_t arrayNumElements = numIndices;
        UsdAttribute arrayPrimvar = outGeom->GetFaceVertexIndicesAttr();
        bool setPrimvar = true;

        switch (geomData.IndicesType)
        {
        case UsdBridgeType::ULONG: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtIntArray, uint64_t); break; }
        case UsdBridgeType::LONG: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtIntArray, int64_t); break; }
        case UsdBridgeType::INT: {ASSIGN_PRIMVAR_MACRO(VtIntArray); break; }
        case UsdBridgeType::UINT: {ASSIGN_PRIMVAR_MACRO(VtIntArray); break; }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom FaceVertexIndicesAttr should be (U)LONG or (U)INT."); break; }
        }
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomNormals(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::NORMALS);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::NORMALS);

    ClearUsdAttributes(uniformGeom.GetNormalsAttr(), timeVarGeom.GetNormalsAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType* outGeom = timeVaryingUpdate ? &timeVarGeom : &uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::NORMALS);

      UsdAttribute normalsAttr = outGeom->GetNormalsAttr();

      if (geomData.Normals != nullptr)
      {
        const void* arrayData = geomData.Normals;
        size_t arrayNumElements = geomData.PerPrimNormals ? numPrims : geomData.NumPoints;
        UsdAttribute arrayPrimvar = normalsAttr;
        bool setPrimvar = true;

        switch (geomData.NormalsType)
        {
        case UsdBridgeType::FLOAT3: {ASSIGN_PRIMVAR_MACRO(VtVec3fArray); break; }
        case UsdBridgeType::DOUBLE3: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtVec3fArray, GfVec3d); break; }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom NormalsAttr should be FLOAT3 or DOUBLE3."); break; }
        }

        // Per face or per-vertex interpolation. This will break timesteps that have been written before.
        TfToken normalInterpolation = geomData.PerPrimNormals ? UsdGeomTokens->uniform : UsdGeomTokens->vertex;
        uniformGeom.SetNormalsInterpolation(normalInterpolation);
      }
      else
      {
        normalsAttr.Set(SdfValueBlock(), timeCode);
      }
    }
  }

  template<typename GeomDataType>
  void UpdateUsdGeomTexCoords(UsdBridgeUsdWriter* writer, UsdGeomPrimvarsAPI& timeVarPrimvars, UsdGeomPrimvarsAPI& uniformPrimvars, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::ATTRIBUTE0);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::ATTRIBUTE0);

    UsdGeomPrimvar uniformPrimvar = uniformPrimvars.GetPrimvar(UsdBridgeTokens->st);
    UsdGeomPrimvar timeVarPrimvar = timeVarPrimvars.GetPrimvar(UsdBridgeTokens->st);

    ClearUsdAttributes(uniformPrimvar.GetAttr(), timeVarPrimvar.GetAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdTimeCode timeCode = timeEval.Eval(DMI::ATTRIBUTE0);

      UsdAttribute texcoordPrimvar = timeVaryingUpdate ? timeVarPrimvar : uniformPrimvar;
      assert(texcoordPrimvar);

      const UsdBridgeAttribute& texCoordAttrib = geomData.Attributes[0];

      if (texCoordAttrib.Data != nullptr)
      {
        const void* arrayData = texCoordAttrib.Data;
        size_t arrayNumElements = texCoordAttrib.PerPrimData ? numPrims : geomData.NumPoints;
        UsdAttribute arrayPrimvar = texcoordPrimvar;
        bool setPrimvar = true;

        switch (texCoordAttrib.DataType)
        {
        case UsdBridgeType::FLOAT2: { ASSIGN_PRIMVAR_MACRO(VtVec2fArray); break; }
        case UsdBridgeType::DOUBLE2: { ASSIGN_PRIMVAR_CONVERT_MACRO(VtVec2fArray, GfVec2d); break; }
        default: { UsdBridgeLogMacro(writer->LogObject, UsdBridgeLogLevel::ERR, "UsdGeom st primvar should be FLOAT2 or DOUBLE2."); break; }
        }

        // Per face or per-vertex interpolation. This will break timesteps that have been written before.
        TfToken texcoordInterpolation = texCoordAttrib.PerPrimData ? UsdGeomTokens->uniform : UsdGeomTokens->vertex;
        uniformPrimvar.SetInterpolation(texcoordInterpolation);
      }
      else
      {
        texcoordPrimvar.Set(SdfValueBlock(), timeCode);
      }
    }
  }

  template<typename GeomDataType>
  void UpdateUsdGeomAttribute(UsdBridgeUsdWriter* writer, UsdGeomPrimvarsAPI& timeVarPrimvars, UsdGeomPrimvarsAPI& uniformPrimvars, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval, uint32_t attribIndex)
  {
    assert(attribIndex < geomData.NumAttributes);
    const UsdBridgeAttribute& bridgeAttrib = geomData.Attributes[attribIndex];

    TfToken attribToken = bridgeAttrib.Name ? writer->AttributeNameToken(bridgeAttrib.Name) : AttribIndexToToken(attribIndex);
    UsdGeomPrimvar uniformPrimvar = uniformPrimvars.GetPrimvar(attribToken);
    // The uniform primvar has to exist, otherwise any timevarying data will be ignored as well
    if(!uniformPrimvar || uniformPrimvar.GetTypeName() != GetPrimvarArrayType(bridgeAttrib.DataType))
    {
      CreateUsdGeomAttributePrimvar(writer, uniformPrimvars, geomData, attribIndex); // No timeEval, to force attribute primvar creation on the uniform api
    }

    UsdGeomPrimvar timeVarPrimvar = timeVarPrimvars.GetPrimvar(attribToken);
    if(!timeVarPrimvar || timeVarPrimvar.GetTypeName() != GetPrimvarArrayType(bridgeAttrib.DataType)) // even though new clipstages initialize the correct primvar type/name, it may still be wrong for existing ones (or primstages if so configured)
    {
      CreateUsdGeomAttributePrimvar(writer, timeVarPrimvars, geomData, attribIndex, &timeEval);
    }

    using DMI = typename GeomDataType::DataMemberId;
    DMI attributeId = DMI::ATTRIBUTE0 + attribIndex;
    bool performsUpdate = updateEval.PerformsUpdate(attributeId);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(attributeId);

    ClearUsdAttributes(uniformPrimvar.GetAttr(), timeVarPrimvar.GetAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdTimeCode timeCode = timeEval.Eval(attributeId);

      UsdAttribute attributePrimvar = timeVaryingUpdate ? timeVarPrimvar : uniformPrimvar;

      if(!attributePrimvar)
      {
        UsdBridgeLogMacro(writer->LogObject, UsdBridgeLogLevel::ERR, "UsdGeom Attribute<Index> primvar not found, was the attribute at requested index valid during initialization of the prim? Index is " << attribIndex);
      }
      else
      {
        if (bridgeAttrib.Data != nullptr)
        {
          const void* arrayData = bridgeAttrib.Data;
          size_t arrayNumElements = bridgeAttrib.PerPrimData ? numPrims : geomData.NumPoints;
          UsdAttribute arrayPrimvar = attributePrimvar;

          AssignAttribArrayToPrimvar(writer->LogObject, arrayData, bridgeAttrib.DataType, arrayNumElements, arrayPrimvar, timeCode);

          // Per face or per-vertex interpolation. This will break timesteps that have been written before.
          TfToken attribInterpolation = bridgeAttrib.PerPrimData ? UsdGeomTokens->uniform : UsdGeomTokens->vertex;
          uniformPrimvar.SetInterpolation(attribInterpolation);
        }
        else
        {
          attributePrimvar.Set(SdfValueBlock(), timeCode);
        }
      }
    }
  }

  template<typename GeomDataType>
  void UpdateUsdGeomAttributes(UsdBridgeUsdWriter* writer, UsdGeomPrimvarsAPI& timeVarPrimvars, UsdGeomPrimvarsAPI& uniformPrimvars, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    uint32_t startIdx = 0;
    for(uint32_t attribIndex = startIdx; attribIndex < geomData.NumAttributes; ++attribIndex)
    {
      const UsdBridgeAttribute& attrib = geomData.Attributes[attribIndex];
      if(attrib.DataType != UsdBridgeType::UNDEFINED)
        UpdateUsdGeomAttribute(writer, timeVarPrimvars, uniformPrimvars, geomData, numPrims, updateEval, timeEval, attribIndex);
    }
  }

  template<typename GeomDataType>
  void UpdateUsdGeomColors(UsdBridgeUsdWriter* writer, UsdGeomPrimvarsAPI& timeVarPrimvars, UsdGeomPrimvarsAPI& uniformPrimvars, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::COLORS);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::COLORS);

    UsdGeomPrimvar uniformDispPrimvar = uniformPrimvars.GetPrimvar(UsdBridgeTokens->color);
    UsdGeomPrimvar timeVarDispPrimvar = timeVarPrimvars.GetPrimvar(UsdBridgeTokens->color);

    ClearUsdAttributes(uniformDispPrimvar.GetAttr(), timeVarDispPrimvar.GetAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdTimeCode timeCode = timeEval.Eval(DMI::COLORS);

      UsdGeomPrimvar colorPrimvar = timeVaryingUpdate ? timeVarDispPrimvar : uniformDispPrimvar;

      if (geomData.Colors != nullptr)
      {
        size_t arrayNumElements = geomData.PerPrimColors ? numPrims : geomData.NumPoints;
        assert(colorPrimvar);

        AssignColorArrayToPrimvar(writer->LogObject, geomData.Colors, arrayNumElements, geomData.ColorsType, timeEval.Eval(DMI::COLORS), colorPrimvar.GetAttr());

        // Per face or per-vertex interpolation. This will break timesteps that have been written before.
        TfToken colorInterpolation = geomData.PerPrimColors ? UsdGeomTokens->uniform : UsdGeomTokens->vertex;
        uniformDispPrimvar.SetInterpolation(colorInterpolation);
      }
      else
      {
        colorPrimvar.GetAttr().Set(SdfValueBlock(), timeCode);
      }
    }
  }


  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomInstanceIds(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::INSTANCEIDS);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::INSTANCEIDS);

    ClearUsdAttributes(uniformGeom.GetIdsAttr(), timeVarGeom.GetIdsAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType* outGeom = timeVaryingUpdate ? &timeVarGeom : &uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::INSTANCEIDS);

      UsdAttribute idsAttr = outGeom->GetIdsAttr();

      if (geomData.InstanceIds)
      {
        const void* arrayData = geomData.InstanceIds;
        size_t arrayNumElements = geomData.NumPoints;
        UsdAttribute arrayPrimvar = idsAttr;
        bool setPrimvar = true;

        switch (geomData.InstanceIdsType)
        {
        case UsdBridgeType::UINT: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtInt64Array, unsigned int); break; }
        case UsdBridgeType::INT: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtInt64Array, int); break; }
        case UsdBridgeType::LONG: {ASSIGN_PRIMVAR_MACRO(VtInt64Array); break; }
        case UsdBridgeType::ULONG: {ASSIGN_PRIMVAR_MACRO(VtInt64Array); break; }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom IdsAttribute should be (U)LONG or (U)INT."); break; }
        }
      }
      else
      {
        idsAttr.Set(SdfValueBlock(), timeCode);
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomWidths(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::SCALES);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::SCALES);

    ClearUsdAttributes(uniformGeom.GetWidthsAttr(), timeVarGeom.GetWidthsAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType& outGeom = timeVaryingUpdate ? timeVarGeom : uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::SCALES);

      UsdAttribute widthsAttribute = outGeom.GetWidthsAttr();
      assert(widthsAttribute);
      if (geomData.Scales)
      {
        const void* arrayData = geomData.Scales;
        size_t arrayNumElements = geomData.NumPoints;
        UsdAttribute arrayPrimvar = widthsAttribute;
        bool setPrimvar = false;

        auto doubleFn = [](VtFloatArray& usdArray) { for(auto& x : usdArray) { x *= 2.0f; } };
        switch (geomData.ScalesType)
        {
        case UsdBridgeType::FLOAT: {ASSIGN_PRIMVAR_MACRO(VtFloatArray); doubleFn(usdArray); arrayPrimvar.Set(usdArray, timeCode); break; }
        case UsdBridgeType::DOUBLE: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtFloatArray, double); doubleFn(usdArray); arrayPrimvar.Set(usdArray, timeCode); break; }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom WidthsAttribute should be FLOAT or DOUBLE."); break; }
        }
      }
      else
      {
        // Remember that widths define a diameter, so a default width (1.0) corresponds to a scale of 0.5.
        if(geomData.getUniformScale() != 0.5f)
        {
          VtFloatArray& usdWidths = GetStaticTempArray<VtFloatArray>();
          usdWidths.resize(geomData.NumPoints);
          for(auto& x : usdWidths) x = geomData.getUniformScale() * 2.0f;
          widthsAttribute.Set(usdWidths, timeCode);
        }
        else
        {
          widthsAttribute.Set(SdfValueBlock(), timeCode);
        }
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomScales(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::SCALES);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::SCALES);

    ClearUsdAttributes(uniformGeom.GetScalesAttr(), timeVarGeom.GetScalesAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType& outGeom = timeVaryingUpdate ? timeVarGeom : uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::SCALES);

      UsdAttribute scalesAttribute = outGeom.GetScalesAttr();
      assert(scalesAttribute);
      if (geomData.Scales)
      {
        const void* arrayData = geomData.Scales;
        size_t arrayNumElements = geomData.NumPoints;
        UsdAttribute arrayPrimvar = scalesAttribute;
        bool setPrimvar = true;

        switch (geomData.ScalesType)
        {
        case UsdBridgeType::FLOAT: {ASSIGN_PRIMVAR_MACRO_1EXPAND3(VtVec3fArray, float); break;}
        case UsdBridgeType::DOUBLE: {ASSIGN_PRIMVAR_MACRO_1EXPAND3(VtVec3fArray, double); break;}
        case UsdBridgeType::FLOAT3: {ASSIGN_PRIMVAR_MACRO(VtVec3fArray); break; }
        case UsdBridgeType::DOUBLE3: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtVec3fArray, GfVec3d); break; }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom ScalesAttribute should be FLOAT(3) or DOUBLE(3)."); break; }
        }
      }
      else
      {
        if(!usdbridgenumerics::isIdentity(geomData.Scale))
        {
          GfVec3f defaultScale(geomData.Scale.Data);
          VtVec3fArray& usdScales = GetStaticTempArray<VtVec3fArray>();
          usdScales.resize(geomData.NumPoints);
          for(auto& x : usdScales) x = defaultScale;
          scalesAttribute.Set(usdScales, timeCode);
        }
        else
        {
          scalesAttribute.Set(SdfValueBlock(), timeCode);
        }
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomOrientNormals(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::ORIENTATIONS);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::ORIENTATIONS);

    ClearUsdAttributes(uniformGeom.GetNormalsAttr(), timeVarGeom.GetNormalsAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType& outGeom = timeVaryingUpdate ? timeVarGeom : uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::ORIENTATIONS);

      UsdAttribute normalsAttribute = outGeom.GetNormalsAttr();
      assert(normalsAttribute);
      if (geomData.Orientations)
      {
        const void* arrayData = geomData.Orientations;
        size_t arrayNumElements = geomData.NumPoints;
        UsdAttribute arrayPrimvar = normalsAttribute;
        bool setPrimvar = true;

        switch (geomData.OrientationsType)
        {
        case UsdBridgeType::FLOAT3: {ASSIGN_PRIMVAR_MACRO(VtVec3fArray); break; }
        case UsdBridgeType::DOUBLE3: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtVec3fArray, GfVec3d); break; }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom NormalsAttribute (orientations) should be FLOAT3 or DOUBLE3."); break; }
        }
      }
      else
      {
        //GfVec3f defaultNormal(1, 0, 0);
        //VtVec3fArray& usdNormals = GetStaticTempArray<VtVec3fArray>();
        //usdNormals.resize(geomData.NumPoints);
        //for(auto& x : usdNormals) x = defaultNormal;
        //normalsAttribute.Set(usdNormals, timeCode);
        normalsAttribute.Set(SdfValueBlock(), timeCode);
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomOrientations(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::ORIENTATIONS);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::ORIENTATIONS);

    ClearUsdAttributes(uniformGeom.GetOrientationsAttr(), timeVarGeom.GetOrientationsAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType& outGeom = timeVaryingUpdate ? timeVarGeom : uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::ORIENTATIONS);

      // Orientations
      UsdAttribute orientationsAttribute = outGeom.GetOrientationsAttr();
      assert(orientationsAttribute);
      VtQuathArray& usdOrients = GetStaticTempArray<VtQuathArray>();
      if (geomData.Orientations)
      {
        usdOrients.resize(geomData.NumPoints);
        switch (geomData.OrientationsType)
        {
        case UsdBridgeType::FLOAT3: { ConvertNormalsToQuaternions<float>(usdOrients, geomData.Orientations, geomData.NumPoints); break; }
        case UsdBridgeType::DOUBLE3: { ConvertNormalsToQuaternions<double>(usdOrients, geomData.Orientations, geomData.NumPoints); break; }
        case UsdBridgeType::FLOAT4:
          {
            // Note that ANARI quaternion arrays are in IJKW order
            for (uint64_t i = 0; i < geomData.NumPoints; ++i)
            {
              const float* orients = reinterpret_cast<const float*>(geomData.Orientations);
              usdOrients[i] = GfQuath(orients[i * 4 + 3], orients[i * 4], orients[i * 4 + 1], orients[i * 4 + 2]);
            }
            orientationsAttribute.Set(usdOrients, timeCode);
            break;
          }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom OrientationsAttribute should be FLOAT3, DOUBLE3 or FLOAT4."); break; }
        }
        orientationsAttribute.Set(usdOrients, timeCode);
      }
      else
      {
        if(!usdbridgenumerics::isIdentity(geomData.Orientation))
        {
          // Note that ANARI quaternion arrays are in IJKW order
          GfQuath defaultOrient(geomData.Orientation.Data[3], geomData.Orientation.Data[0], geomData.Orientation.Data[1], geomData.Orientation.Data[2]);
          usdOrients.resize(geomData.NumPoints);
          for(auto& x : usdOrients) x = defaultOrient;
          orientationsAttribute.Set(usdOrients, timeCode);
        }
        else
        {
          orientationsAttribute.Set(SdfValueBlock(), timeCode);
        }
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomProtoIndices(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::SHAPEINDICES);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::SHAPEINDICES);


    if (performsUpdate)
    {
      UsdTimeCode timeCode = timeEval.Eval(DMI::SHAPEINDICES);
      UsdGeomType* outGeom = timeCode.IsDefault() ? &uniformGeom : &timeVarGeom;

      UsdAttribute protoIndexAttr = outGeom->GetProtoIndicesAttr();
      assert(protoIndexAttr);

      //Shape indices
      if(geomData.ShapeIndices)
      {
        const void* arrayData = geomData.ShapeIndices;
        size_t arrayNumElements = geomData.NumPoints;
        UsdAttribute arrayPrimvar = protoIndexAttr;
        bool setPrimvar = true;

        switch (geomData.OrientationsType)
        {
        case UsdBridgeType::INT: {ASSIGN_PRIMVAR_MACRO(VtIntArray); break; }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom ProtoIndicesAttr (ShapeIndices) should be INT."); break; }
        }
      }
      else
      {
        VtIntArray& protoIndices = GetStaticTempArray<VtIntArray>();
        protoIndices.resize(geomData.NumPoints);
        for(auto& x : protoIndices) x = 0;
        protoIndexAttr.Set(protoIndices, timeCode);
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomLinearVelocities(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::LINEARVELOCITIES);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::LINEARVELOCITIES);

    ClearUsdAttributes(uniformGeom.GetVelocitiesAttr(), timeVarGeom.GetVelocitiesAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType& outGeom = timeVaryingUpdate ? timeVarGeom : uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::LINEARVELOCITIES);

      // Linear velocities
      UsdAttribute linearVelocitiesAttribute = outGeom.GetVelocitiesAttr();
      assert(linearVelocitiesAttribute);
      if (geomData.LinearVelocities)
      {
        GfVec3f* linVels = (GfVec3f*)geomData.LinearVelocities;

        VtVec3fArray& usdVelocities = GetStaticTempArray<VtVec3fArray>();
        usdVelocities.assign(linVels, linVels + geomData.NumPoints);
        linearVelocitiesAttribute.Set(usdVelocities, timeCode);
      }
      else
      {
        linearVelocitiesAttribute.Set(SdfValueBlock(), timeCode);
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomAngularVelocities(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::ANGULARVELOCITIES);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::ANGULARVELOCITIES);

    ClearUsdAttributes(uniformGeom.GetAngularVelocitiesAttr(), timeVarGeom.GetAngularVelocitiesAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType& outGeom = timeVaryingUpdate ? timeVarGeom : uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::ANGULARVELOCITIES);

      // Angular velocities
      UsdAttribute angularVelocitiesAttribute = outGeom.GetAngularVelocitiesAttr();
      assert(angularVelocitiesAttribute);
      if (geomData.AngularVelocities)
      {
        GfVec3f* angVels = (GfVec3f*)geomData.AngularVelocities;

        VtVec3fArray& usdAngularVelocities = GetStaticTempArray<VtVec3fArray>();
        usdAngularVelocities.assign(angVels, angVels + geomData.NumPoints);
        angularVelocitiesAttribute.Set(usdAngularVelocities, timeCode);
      }
      else
      {
        angularVelocitiesAttribute.Set(SdfValueBlock(), timeCode);
      }
    }
  }

  template<typename UsdGeomType, typename GeomDataType>
  void UpdateUsdGeomInvisibleIds(const UsdBridgeLogObject& logObj, UsdGeomType& timeVarGeom, UsdGeomType& uniformGeom, const GeomDataType& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const GeomDataType>& updateEval, TimeEvaluator<GeomDataType>& timeEval)
  {
    using DMI = typename GeomDataType::DataMemberId;
    bool performsUpdate = updateEval.PerformsUpdate(DMI::INVISIBLEIDS);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::INVISIBLEIDS);

    ClearUsdAttributes(uniformGeom.GetInvisibleIdsAttr(), timeVarGeom.GetInvisibleIdsAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomType& outGeom = timeVaryingUpdate ? timeVarGeom : uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::INVISIBLEIDS);

      // Invisible ids
      UsdAttribute invisIdsAttr = outGeom.GetInvisibleIdsAttr();
      assert(invisIdsAttr);
      uint64_t numInvisibleIds = geomData.NumInvisibleIds;
      if (numInvisibleIds)
      {
        const void* arrayData = geomData.InvisibleIds;
        size_t arrayNumElements = numInvisibleIds;
        UsdAttribute arrayPrimvar = invisIdsAttr;
        bool setPrimvar = true;

        switch (geomData.InvisibleIdsType)
        {
        case UsdBridgeType::UINT: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtInt64Array, unsigned int); break; }
        case UsdBridgeType::INT: {ASSIGN_PRIMVAR_CONVERT_MACRO(VtInt64Array, int); break; }
        case UsdBridgeType::LONG: {ASSIGN_PRIMVAR_MACRO(VtInt64Array); break; }
        case UsdBridgeType::ULONG: {ASSIGN_PRIMVAR_MACRO(VtInt64Array); break; }
        default: { UsdBridgeLogMacro(logObj, UsdBridgeLogLevel::ERR, "UsdGeom GetInvisibleIdsAttr should be (U)LONG or (U)INT."); break; }
        }
      }
      else
      {
        invisIdsAttr.Set(SdfValueBlock(), timeCode);
      }
    }
  }

  static void UpdateUsdGeomCurveLengths(const UsdBridgeLogObject& logObj, UsdGeomBasisCurves& timeVarGeom, UsdGeomBasisCurves& uniformGeom, const UsdBridgeCurveData& geomData, uint64_t numPrims,
    UsdBridgeUpdateEvaluator<const UsdBridgeCurveData>& updateEval, TimeEvaluator<UsdBridgeCurveData>& timeEval)
  {
    using DMI = typename UsdBridgeCurveData::DataMemberId;
    // Fill geom prim and geometry layer with data.
    bool performsUpdate = updateEval.PerformsUpdate(DMI::CURVELENGTHS);
    bool timeVaryingUpdate = timeEval.IsTimeVarying(DMI::CURVELENGTHS);

    ClearUsdAttributes(uniformGeom.GetCurveVertexCountsAttr(), timeVarGeom.GetCurveVertexCountsAttr(), timeVaryingUpdate);

    if (performsUpdate)
    {
      UsdGeomBasisCurves& outGeom = timeVaryingUpdate ? timeVarGeom : uniformGeom;
      UsdTimeCode timeCode = timeEval.Eval(DMI::POINTS);

      UsdAttribute vertCountAttr = outGeom.GetCurveVertexCountsAttr();
      assert(vertCountAttr);

      const void* arrayData = geomData.CurveLengths;
      size_t arrayNumElements = geomData.NumCurveLengths;
      UsdAttribute arrayPrimvar = vertCountAttr;
      bool setPrimvar = true;

      { ASSIGN_PRIMVAR_MACRO(VtIntArray); }
    }
  }

  void UpdateUsdGeomPrototypes(const UsdBridgeLogObject& logObj, const UsdStagePtr& sceneStage, UsdGeomPointInstancer& uniformGeom,
    const UsdBridgeInstancerRefData& geomRefData, const SdfPrimPathList& protoGeomPaths,
    const char* protoShapePathRp)
  {
    using DMI = typename UsdBridgeInstancerData::DataMemberId;

    SdfPath protoBasePath = uniformGeom.GetPath().AppendPath(SdfPath(protoShapePathRp));
    sceneStage->RemovePrim(protoBasePath);

    UsdRelationship protoRel = uniformGeom.GetPrototypesRel();

    for(int shapeIdx = 0; shapeIdx < geomRefData.NumShapes; ++shapeIdx)
    {
      SdfPath shapePath;
      if(geomRefData.Shapes[shapeIdx] != UsdBridgeInstancerRefData::SHAPE_MESH)
      {
        std::string protoName = constring::protoShapePf + std::to_string(shapeIdx);
        shapePath = protoBasePath.AppendPath(SdfPath(protoName.c_str()));
      }
      else
      {
        int protoGeomIdx = static_cast<int>(geomRefData.Shapes[shapeIdx]); // The mesh shape value is an index into protoGeomPaths
        assert(protoGeomIdx < protoGeomPaths.size());
        shapePath = protoGeomPaths[protoGeomIdx];
      }

      UsdGeomXformable geomXformable;
      switch (geomRefData.Shapes[shapeIdx])
      {
        case UsdBridgeInstancerRefData::SHAPE_SPHERE:
        {
          geomXformable = UsdGeomSphere::Define(sceneStage, shapePath);
          break;
        }
        case UsdBridgeInstancerRefData::SHAPE_CYLINDER:
        {
          geomXformable = UsdGeomCylinder::Define(sceneStage, shapePath);
          break;
        }
        case UsdBridgeInstancerRefData::SHAPE_CONE:
        {
          geomXformable = UsdGeomCone::Define(sceneStage, shapePath);
          break;
        }
        default:
        {
          geomXformable = UsdGeomXformable::Get(sceneStage, shapePath);
          break;
        }
      }

      // Add a transform
      geomXformable.ClearXformOpOrder();
      if(!usdbridgenumerics::isIdentity(geomRefData.ShapeTransform))
      {
        const float* transform = geomRefData.ShapeTransform.Data;
        GfMatrix4d transMat;
        transMat.SetRow(0, GfVec4d(GfVec4f(&transform[0])));
        transMat.SetRow(1, GfVec4d(GfVec4f(&transform[4])));
        transMat.SetRow(2, GfVec4d(GfVec4f(&transform[8])));
        transMat.SetRow(3, GfVec4d(GfVec4f(&transform[12])));

        geomXformable.AddTransformOp().Set(transMat);
      }

      protoRel.AddTarget(shapePath);
    }
  }
}

UsdPrim UsdBridgeUsdWriter::InitializeUsdGeometry(UsdStageRefPtr geometryStage, const SdfPath& geomPath, const UsdBridgeMeshData& meshData, bool uniformPrim)
{
  return InitializeUsdGeometry_Impl(this, geometryStage, geomPath, meshData, uniformPrim, Settings);
}

UsdPrim UsdBridgeUsdWriter::InitializeUsdGeometry(UsdStageRefPtr geometryStage, const SdfPath& geomPath, const UsdBridgeInstancerData& instancerData, bool uniformPrim)
{
  return InitializeUsdGeometry_Impl(this, geometryStage, geomPath, instancerData, uniformPrim, Settings);
}

UsdPrim UsdBridgeUsdWriter::InitializeUsdGeometry(UsdStageRefPtr geometryStage, const SdfPath& geomPath, const UsdBridgeCurveData& curveData, bool uniformPrim)
{
  return InitializeUsdGeometry_Impl(this, geometryStage, geomPath, curveData, uniformPrim, Settings);
}

#ifdef VALUE_CLIP_RETIMING
void UsdBridgeUsdWriter::UpdateUsdGeometryManifest(const UsdBridgePrimCache* cacheEntry, const UsdBridgeMeshData& meshData)
{
  TimeEvaluator<UsdBridgeMeshData> timeEval(meshData);
  InitializeUsdGeometry_Impl(this, cacheEntry->ManifestStage.second, cacheEntry->PrimPath, meshData, false,
    Settings, &timeEval);

  if(this->EnableSaving)
    cacheEntry->ManifestStage.second->Save();
}

void UsdBridgeUsdWriter::UpdateUsdGeometryManifest(const UsdBridgePrimCache* cacheEntry, const UsdBridgeInstancerData& instancerData)
{
  TimeEvaluator<UsdBridgeInstancerData> timeEval(instancerData);
  InitializeUsdGeometry_Impl(this, cacheEntry->ManifestStage.second, cacheEntry->PrimPath, instancerData, false,
    Settings, &timeEval);

  if(this->EnableSaving)
    cacheEntry->ManifestStage.second->Save();
}

void UsdBridgeUsdWriter::UpdateUsdGeometryManifest(const UsdBridgePrimCache* cacheEntry, const UsdBridgeCurveData& curveData)
{
  TimeEvaluator<UsdBridgeCurveData> timeEval(curveData);
  InitializeUsdGeometry_Impl(this, cacheEntry->ManifestStage.second, cacheEntry->PrimPath, curveData, false,
    Settings, &timeEval);

  if(this->EnableSaving)
    cacheEntry->ManifestStage.second->Save();
}
#endif

#define UPDATE_USDGEOM_ARRAYS(FuncDef) \
  FuncDef(this->LogObject, timeVarGeom, uniformGeom, geomData, numPrims, updateEval, timeEval)

#define UPDATE_USDGEOM_PRIMVAR_ARRAYS(FuncDef) \
  FuncDef(this, timeVarPrimvars, uniformPrimvars, geomData, numPrims, updateEval, timeEval)

void UsdBridgeUsdWriter::UpdateUsdGeometry(const UsdStagePtr& timeVarStage, const SdfPath& meshPath, const UsdBridgeMeshData& geomData, double timeStep)
{
  // To avoid data duplication when using of clip stages, we need to potentially use the scenestage prim for time-uniform data.
  UsdGeomMesh uniformGeom = UsdGeomMesh::Get(this->SceneStage, meshPath);
  assert(uniformGeom);
  UsdGeomPrimvarsAPI uniformPrimvars(uniformGeom);

  UsdGeomMesh timeVarGeom = UsdGeomMesh::Get(timeVarStage, meshPath);
  assert(timeVarGeom);
  UsdGeomPrimvarsAPI timeVarPrimvars(timeVarGeom);

  // Update the mesh
  UsdBridgeUpdateEvaluator<const UsdBridgeMeshData> updateEval(geomData);
  TimeEvaluator<UsdBridgeMeshData> timeEval(geomData, timeStep);

  assert((geomData.NumIndices % geomData.FaceVertexCount) == 0);
  uint64_t numPrims = int(geomData.NumIndices) / geomData.FaceVertexCount;

  UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomPoints);
  UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomNormals);
  if( Settings.EnableStTexCoords && UsdGeomDataHasTexCoords(geomData) )
    { UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomTexCoords); }
  UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomAttributes);
  UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomColors);
  UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomIndices);
}

void UsdBridgeUsdWriter::UpdateUsdGeometry(const UsdStagePtr& timeVarStage, const SdfPath& instancerPath, const UsdBridgeInstancerData& geomData, double timeStep)
{
  UsdBridgeUpdateEvaluator<const UsdBridgeInstancerData> updateEval(geomData);
  TimeEvaluator<UsdBridgeInstancerData> timeEval(geomData, timeStep);

  bool useGeomPoints = geomData.UseUsdGeomPoints;

  uint64_t numPrims = geomData.NumPoints;

  if (useGeomPoints)
  {
    UsdGeomPoints uniformGeom = UsdGeomPoints::Get(this->SceneStage, instancerPath);
    assert(uniformGeom);
    UsdGeomPrimvarsAPI uniformPrimvars(uniformGeom);

    UsdGeomPoints timeVarGeom = UsdGeomPoints::Get(timeVarStage, instancerPath);
    assert(timeVarGeom);
    UsdGeomPrimvarsAPI timeVarPrimvars(timeVarGeom);

    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomPoints);
    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomInstanceIds);
    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomWidths);
    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomOrientNormals);
    if( Settings.EnableStTexCoords && UsdGeomDataHasTexCoords(geomData) )
      { UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomTexCoords); }
    UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomAttributes);
    UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomColors);
  }
  else
  {
    UsdGeomPointInstancer uniformGeom = UsdGeomPointInstancer::Get(this->SceneStage, instancerPath);
    assert(uniformGeom);
    UsdGeomPrimvarsAPI uniformPrimvars(uniformGeom);

    UsdGeomPointInstancer timeVarGeom = UsdGeomPointInstancer::Get(timeVarStage, instancerPath);
    assert(timeVarGeom);
    UsdGeomPrimvarsAPI timeVarPrimvars(timeVarGeom);

    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomPoints);
    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomInstanceIds);
    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomScales);
    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomOrientations);
    if( Settings.EnableStTexCoords && UsdGeomDataHasTexCoords(geomData) )
      { UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomTexCoords); }
    UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomAttributes);
    UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomColors);
    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomProtoIndices);
    //UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomLinearVelocities);
    //UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomAngularVelocities);
    UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomInvisibleIds);
  }
}

void UsdBridgeUsdWriter::UpdateUsdGeometry(const UsdStagePtr& timeVarStage, const SdfPath& curvePath, const UsdBridgeCurveData& geomData, double timeStep)
{
  // To avoid data duplication when using of clip stages, we need to potentially use the scenestage prim for time-uniform data.
  UsdGeomBasisCurves uniformGeom = UsdGeomBasisCurves::Get(this->SceneStage, curvePath);
  assert(uniformGeom);
  UsdGeomPrimvarsAPI uniformPrimvars(uniformGeom);

  UsdGeomBasisCurves timeVarGeom = UsdGeomBasisCurves::Get(timeVarStage, curvePath);
  assert(timeVarGeom);
  UsdGeomPrimvarsAPI timeVarPrimvars(timeVarGeom);

  // Update the curve
  UsdBridgeUpdateEvaluator<const UsdBridgeCurveData> updateEval(geomData);
  TimeEvaluator<UsdBridgeCurveData> timeEval(geomData, timeStep);

  uint64_t numPrims = geomData.NumCurveLengths;

  UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomPoints);
  UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomNormals);
  if( Settings.EnableStTexCoords && UsdGeomDataHasTexCoords(geomData) )
    { UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomTexCoords); }
  UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomAttributes);
  UPDATE_USDGEOM_PRIMVAR_ARRAYS(UpdateUsdGeomColors);
  UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomWidths);
  UPDATE_USDGEOM_ARRAYS(UpdateUsdGeomCurveLengths);
}

void UsdBridgeUsdWriter::UpdateUsdInstancerPrototypes(const SdfPath& instancerPath, const UsdBridgeInstancerRefData& geomRefData,
  const SdfPrimPathList& refProtoGeomPrimPaths, const char* protoShapePathRp)
{
  UsdGeomPointInstancer uniformGeom = UsdGeomPointInstancer::Get(this->SceneStage, instancerPath);
  if(!uniformGeom)
  {
    UsdBridgeLogMacro(this->LogObject, UsdBridgeLogLevel::WARNING, "Attempt to perform update of prototypes on a prim that is not a UsdGeomPointInstancer.");
    return;
  }

  // Very basic rel update, without any timevarying aspects
  UpdateUsdGeomPrototypes(this->LogObject, this->SceneStage, uniformGeom, geomRefData, refProtoGeomPrimPaths, protoShapePathRp);
}