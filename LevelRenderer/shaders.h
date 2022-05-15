#pragma once
#include <string>

namespace Shaders
{
    const char* vertexShader = R"(
    #pragma pack_matrix(row_major)
    #define MAX_SUBMESH_PER_DRAW 1024
    struct OBJ_ATTRIBUTES
    {
        float3 Kd;
        float d;
        float3 Ks;
        float Ns;
        float3 Ka;
        float shaprness;
        float3 Tf;
        float Ni;
        float3 Ke;
        int illum;
    };

    struct SHADER_MODEL_DATA
    {
        float4 ligthDirection;
        float4 lightColor;

        matrix viewMatrix;
        matrix projectionMatrix;

        matrix matricies[MAX_SUBMESH_PER_DRAW];
        OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
	    float4 sunAmbient;
	    float4 camPos;
    };

    StructuredBuffer<SHADER_MODEL_DATA> SceneData;

    [[vk::push_constant]]
    cbuffer MESH_INDEX
    {
	    matrix wMatrix;
	    uint ID;
    };

    struct OBJ_VERT
    {
        float3 pos : POSITION;
	    float3 uvw : UVW;
	    float3 nrm : NORMAL;
    };

    struct OBJ_VERT_OUT
    {
        float4 pos : SV_POSITION;
	    float4 nrm : NORMAL;
	    float4 wPos : POSITION;
    };

    OBJ_VERT_OUT main(OBJ_VERT inputVertex)
    {
        OBJ_VERT_OUT result;
        result.pos = float4(inputVertex.pos, 1);
	    result.nrm = float4(inputVertex.nrm, 0);

	    result.pos = mul(result.pos, wMatrix);
	    result.wPos = result.pos;
        result.pos = mul(result.pos, SceneData[0].viewMatrix);
        result.pos = mul(result.pos, SceneData[0].projectionMatrix);
	    result.nrm = mul(result.nrm, SceneData[0].matricies[ID]);
		

        return result;
    }
)";
    // Simple Pixel Shader
    const char* pixelShader = R"(
    #define MAX_SUBMESH_PER_DRAW 1024
    struct OBJ_ATTRIBUTES
    {
        float3 Kd;
        float d;
        float3 Ks;
        float Ns;
        float3 Ka;
        float shaprness;
        float3 Tf;
        float Ni;
        float3 Ke;
        int illum;
    };

    struct SHADER_MODEL_DATA
    {
        float4 ligthDirection;
        float4 sunColor;

        matrix viewMatrix;
        matrix projectionMatrix;

        matrix matricies[MAX_SUBMESH_PER_DRAW];
        OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
	
        float4 sunAmbient;
        float4 camPos;
    };

    StructuredBuffer<SHADER_MODEL_DATA> SceneData;

    [[vk::push_constant]]
    cbuffer MESH_INDEX
    {
	    matrix wMatrix;
        uint ID;
    };

    struct OBJ_PIX_OUT
    {
        float4 pos : SV_POSITION;
        float4 nrm : NORMAL;
	    float4 wPos : POSITION;
    };

    float4 main(OBJ_PIX_OUT inputVertex) : SV_TARGET
    {
	
        float3 lightDirection = normalize(SceneData[0].ligthDirection.xyz);
        float4 lightColor = SceneData[0].sunColor;
        float3 surfacePos = inputVertex.pos.xyz;
        float3 surfaceNormal = normalize(inputVertex.nrm.xyz);
        float4 surfaceColor = float4(SceneData[0].materials[ID].Kd, 1);
    
        float4 finalColor;
        float lightRatio = clamp(dot( -lightDirection, surfaceNormal), 0, 1);
        float4 Ambient =  SceneData[0].sunAmbient * surfaceColor;
    
	    finalColor = lightRatio * lightColor * surfaceColor;
	    finalColor += Ambient; 

        float3 reflect = reflect(lightDirection, surfaceNormal);
        float3 toCam = normalize(SceneData[0].camPos.xyz - inputVertex.wPos.xyz);
        float specDot = dot(reflect, toCam);
        specDot = pow(specDot, SceneData[0].materials[ID].Ns * 0.5f);
        float4 specFinalColor = float4(1.0f, 1.0f, 1.0f, 0) * lightColor * saturate(specDot);
    
	    finalColor += specFinalColor;
	    return finalColor;
    }
)";
}