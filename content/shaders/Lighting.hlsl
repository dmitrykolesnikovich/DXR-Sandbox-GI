#define PI 3.14159265359f

SamplerState BilinearSampler : register(s0);
SamplerComparisonState PcfShadowMapSampler : register(s1);

cbuffer LightingConstantBuffer : register(b0)
{
	float4x4 InvViewProjection;
    float4x4 ShadowViewProjection;
	float4 CameraPos;
	float4 ScreenSize;
    float2 ShadowTexelSize;
    float ShadowIntensity;
    float pad0;
};

cbuffer LightsConstantBuffer : register(b1)
{
    float4 LightDirection;
    float4 LightColor;
    float LightIntensity;
    float3 pad1;
};

struct VSInput
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float2 uv : TEXCOORD;
	float4 position : SV_POSITION;
};

struct PSOutput
{
	float4 diffuse : SV_Target0;
};

PSInput VSMain(VSInput input)
{
	PSInput result;

	result.position = float4(input.position.xyz, 1);
	result.uv = input.uv;
    
	return result;
}

Texture2D<float4> albedoBuffer : register(t0);
Texture2D<float4> normalBuffer : register(t1);
Texture2D<float4> worldPosBuffer : register(t2);
Texture2D<float4> rsmBuffer : register(t3);
Texture2D<float> depthBuffer : register(t4);
Texture2D<float> shadowBuffer : register(t5);

float CalculateShadow(float3 ShadowCoord)
{
    const float Dilation = 2.0;
    float d1 = Dilation * ShadowTexelSize.x * 0.125;
    float d2 = Dilation * ShadowTexelSize.x * 0.875;
    float d3 = Dilation * ShadowTexelSize.x * 0.625;
    float d4 = Dilation * ShadowTexelSize.x * 0.375;
    float result = (
        2.0 *
            shadowBuffer.SampleCmpLevelZero(PcfShadowMapSampler, ShadowCoord.xy, ShadowCoord.z) +
            shadowBuffer.SampleCmpLevelZero(PcfShadowMapSampler, ShadowCoord.xy + float2(-d2, d1), ShadowCoord.z) +
            shadowBuffer.SampleCmpLevelZero(PcfShadowMapSampler, ShadowCoord.xy + float2(-d1, -d2), ShadowCoord.z) +
            shadowBuffer.SampleCmpLevelZero(PcfShadowMapSampler, ShadowCoord.xy + float2(d2, -d1), ShadowCoord.z) +
            shadowBuffer.SampleCmpLevelZero(PcfShadowMapSampler, ShadowCoord.xy + float2(d1, d2), ShadowCoord.z) +
            shadowBuffer.SampleCmpLevelZero(PcfShadowMapSampler, ShadowCoord.xy + float2(-d4, d3), ShadowCoord.z) +
            shadowBuffer.SampleCmpLevelZero(PcfShadowMapSampler, ShadowCoord.xy + float2(-d3, -d4), ShadowCoord.z) +
            shadowBuffer.SampleCmpLevelZero(PcfShadowMapSampler, ShadowCoord.xy + float2(d4, -d3), ShadowCoord.z) +
            shadowBuffer.SampleCmpLevelZero(PcfShadowMapSampler, ShadowCoord.xy + float2(d3, d4), ShadowCoord.z)
        ) / 10.0;
    
    return result * result;
}

PSOutput PSMain(PSInput input)
{
	PSOutput output = (PSOutput)0;
    float2 inPos = input.position.xy;    
    
    float depth = depthBuffer[inPos].x;
    float4 normal = normalBuffer[inPos];
    float4 albedo = albedoBuffer[inPos];
    float4 worldPos = worldPosBuffer[inPos];
    
    uint gWidth = 0;
    uint gHeight = 0;
    albedoBuffer.GetDimensions(gWidth, gHeight);
    float3 rsm = rsmBuffer.Sample(BilinearSampler, inPos * float2(1.0f / gWidth, 1.0f / gHeight)).rgb;
    
    float4 lightSpacePos = mul(ShadowViewProjection, worldPos);
    float4 shadowcoord = lightSpacePos / lightSpacePos.w;
    shadowcoord.rg = shadowcoord.rg * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    float shadow = CalculateShadow(shadowcoord.rgb);
    
	float3 viewDir = normalize(CameraPos.xyz - worldPos.xyz);

	float3 lightDir = LightDirection.xyz;
	float3 lightColor = LightColor.xyz;
    float lightIntensity = LightIntensity;
	float NdotL = saturate(dot(normal.xyz, lightDir));
    

    output.diffuse.rgb = rsm.rgb + (lightIntensity * NdotL * shadow) * lightColor * albedo.rgb;
    return output;
}
