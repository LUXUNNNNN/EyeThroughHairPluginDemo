#pragma once

#include "CoreMinimal.h"
#include "EyeThroughHairCustomRenderPass.h"
#include "SceneViewExtension.h"

#include "EyeThroughHairViewExtension.generated.h"
class UTextureRenderTarget2D;

USTRUCT(BlueprintType)
struct FEyeThroughHairSettings
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair")
    float MinDepthGapCm = 0.05f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair")
    float MaxHairThicknessCm = 6.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair")
    float FrontDepthToleranceCm = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair")
    float FaceDepthToleranceCm = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair")
    float EdgeFeatherCm = 0.25f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair")
    float OverlayOpacity = 1.0f;

    //ForTest
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair")
    float EyeHairLerp = 0.5f;
};

class FEyeThroughHairViewExtension final : public FSceneViewExtensionBase
{
    
public:
    bool bIsActive = true;
    FMatrix ViewMatrix;//用于变换SceneCapture的矩阵
    
    explicit FEyeThroughHairViewExtension(const FAutoRegister& AutoRegister);
    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
    virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;
    virtual void SubscribeToPostProcessingPass(
        EPostProcessingPass Pass,
        FAfterPassCallbackDelegateArray& InOutPassCallbacks,
        bool bIsPassEnabled) override;
    /*
    FScreenPassTexture SubscribeToPostProcessingPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
                                                                  const FPostProcessMaterialInputs& InOutInputs);
    */
    virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
                                                 const FPostProcessingInputs& Inputs) override;

    //游戏线程调用此函数更新参数
    void SetRenderSettings_GameThread(const FEyeThroughHairSettings& NewSettings);

    //CRP 方法
    //游戏线程调用此函数设置目标眼睛与头发组件
    void SetTargetComponents_GameThread(
       UPrimitiveComponent* EyeProxy,
       UPrimitiveComponent* FrontHair,
       UPrimitiveComponent* Face);
    void ClearTargetComponents_GameThread();
    void SetOutputTargets_GameThread(UTextureRenderTarget2D* InEyeColorDepthTarget, UTextureRenderTarget2D* InHairDepthTarget, UTextureRenderTarget2D* InFaceDepthTarget);
    void ClearOutputTargets_GameThread();
    //CRP 方法
    
    //SceneCapture方法
    void SetCaptureResources_GameThread(
        UTextureRenderTarget2D* EyeColor,
        UTextureRenderTarget2D* EyeDepth,
        UTextureRenderTarget2D* HairDepth,
        const FEyeThroughHairSettings& Settings);
    void ClearCaptureResources_GameThread();
    //SceneCapture方法

    //保存场景
private:
    FSceneInterface* TargetScene_GT = nullptr;
    FSceneInterface* TargetScene_RT = nullptr;
    
private:
    //维护目标眼睛和头发组件,由CustomRenderPass解析渲染资源EyeProxyComponent->GetPrimitiveSceneId()
    //弱引用，会自动保存对象索引，不负责生命周期
    TWeakObjectPtr<UPrimitiveComponent> EyeProxyComponent;
    TWeakObjectPtr<UPrimitiveComponent> FrontHairComponent;
    TWeakObjectPtr<UPrimitiveComponent> FaceComponent;
    
    TWeakObjectPtr<UTextureRenderTarget2D> EyeColorDepthTarget_GT;
    TWeakObjectPtr<UTextureRenderTarget2D> HairDepthTarget_GT;
    TWeakObjectPtr<UTextureRenderTarget2D> FaceDepthTarget_GT;
    FEyeThroughHairSettings RenderSettings;
    // Render-thread-only state.
    FRenderTarget* EyeColorDepthTarget_RT = nullptr;
    FRenderTarget* HairDepthTarget_RT = nullptr;
    FRenderTarget* FaceDepthTarget_RT = nullptr;
    
    FScreenPassTexture Composite_RenderThread(FRDGBuilder& GraphBuild,const FSceneView& View,const FPostProcessMaterialInputs& Inputs);
    //CRP 方法
    void SetOutputTargets_RenderThread(FRenderTarget* InEyeColorDepthTarget,FRenderTarget* InHairDepthTarget, FRenderTarget* InFaceDepthTarget);
    //CRP 方法
    //渲染线程更新参数
    void SetRenderSettings_RenderThread(const FEyeThroughHairSettings& NewSettings);
    
    //SceneCapture做法
    void SetCaptureResources_RenderThread(
        FTextureRHIRef InEyeColor,
        FTextureRHIRef InEyeDepth,
        FTextureRHIRef InHairDepth,
        const FEyeThroughHairSettings& InSettings);
    /*
    FTextureRHIRef EyeColorRHI;
    FTextureRHIRef EyeDepthRHI;
    FTextureRHIRef HairDepthRHI;
    FEyeThroughHairSettings RenderSettings;
    */
    
    //维护三个CustomPass
    /*
    TUniquePtr<FEyeThroughHairCustomRenderPass> EyeColorDepthPass;
    TUniquePtr<FEyeThroughHairCustomRenderPass> HairDepthPass;
    */
    //TUniquePtr<FEyeThroughHairCustomRenderPass> EyeDepthPass;

};
