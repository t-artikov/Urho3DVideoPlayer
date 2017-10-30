#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"

void VS(float4 iPos : POSITION,
    #ifdef INSTANCED
        float4x3 iModelInstance : TEXCOORD4,
    #endif
    out float3 oTexCoord : TEXCOORD0,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);

    oPos.z = oPos.w;
    oTexCoord = iPos.xyz;
}

float2 GetPanoramicTexCoord(float3 dir)
{
	const float pi = 3.1415926;
	return float2(atan2(dir.x, -dir.z)/(pi * 2.0) + 0.5, asin(-dir.y)/pi + 0.5);
}

void PS(float3 iTexCoord : TEXCOORD0,
    out float4 oColor : OUTCOLOR0)
{
    oColor = cMatDiffColor * Sample2D(DiffMap, GetPanoramicTexCoord(normalize(iTexCoord)));
}
