#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ScreenPass.h"
#include "SceneRenderTargetParameters.h"
#include "ShaderParameterStruct.h"
#include "DataDrivenShaderPlatformInfo.h"

class FEyeThroughHairPS final : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FEyeThroughHairPS);
    SHADER_USE_PARAMETER_STRUCT(FEyeThroughHairPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
        SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)

        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
        //EyeColorDepthTexture需要两个采样器（双线性和邻近）过滤，分别采样颜色和深度
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, EyeColorDepthTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, EyeColorSampler)
        SHADER_PARAMETER_SAMPLER(SamplerState, EyeDepthSampler)
        /*
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, EyeDepthTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, EyeDepthSampler)
        */
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HairDepthTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, HairDepthSampler)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FaceDepthTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, FaceDepthSampler)
    
        //用来算基于ViewRect范围的UV
        SHADER_PARAMETER(FVector2f, ViewRectMin)
        SHADER_PARAMETER(FVector2f, InvViewRectSize)
    
        SHADER_PARAMETER(float, MinDepthGapCm)
        SHADER_PARAMETER(float, MaxHairThicknessCm)
        SHADER_PARAMETER(float, FrontDepthToleranceCm)
        SHADER_PARAMETER(float, EdgeFeatherCm)
        SHADER_PARAMETER(float, OverlayOpacity)
        SHADER_PARAMETER(float, InvalidDepthCm)
        SHADER_PARAMETER(float, FaceDepthToleranceCm)
        SHADER_PARAMETER(float, EyeHairLerp)
    
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
