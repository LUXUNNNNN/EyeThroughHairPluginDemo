#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "EyeThroughHairViewExtension.h"

#include "EyeThroughHairCaptureComponent.generated.h"

class UPrimitiveComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Single-view desktop prototype.
 *
 * EyeProxyComponent should contain only the iris / pupil / lashes that may show through hair.
 * FrontHairComponent should contain only the front fringe, or a masked depth proxy of it.
 */
UCLASS(ClassGroup=(Rendering), meta=(BlueprintSpawnableComponent))
class HAIREYETHROUGH_API UEyeThroughHairCaptureComponent final : public UActorComponent
{
    GENERATED_BODY()

public:
    UEyeThroughHairCaptureComponent();
    
    UPROPERTY(EditAnywhere,BlueprintReadWrite,
        Category="Eye Through Hair",
        meta=(UseComponentPicker, AllowedClasses="/Script/Engine.PrimitiveComponent"))
    FComponentReference EyeProxyComponentRef;
    
    UPROPERTY(EditAnywhere,BlueprintReadWrite,
        Category="Eye Through Hair",
        meta=(UseComponentPicker, AllowedClasses="/Script/Engine.PrimitiveComponent"))
    FComponentReference FrontHairComponentRef;

    UPROPERTY(EditAnywhere,BlueprintReadWrite,
        Category="Eye Through Hair",
        meta=(UseComponentPicker, AllowedClasses="/Script/Engine.PrimitiveComponent"))
    FComponentReference FaceComponentRef;
    
    //Component中维护两张RT
    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> EyeColorDepthTarget = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> HairDepthTarget = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> FaceDepthTarget = nullptr;
    /** Mesh rendered into EyeColor and EyeDepth. Prefer a capture-only duplicate. */
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair")
    //TObjectPtr<UPrimitiveComponent> EyeProxyComponent = nullptr;

    /** Front fringe mesh, or a capture-only masked depth proxy. */
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair")
    //TObjectPtr<UPrimitiveComponent> FrontHairComponent = nullptr;

    /** Eye must be at least this far behind the hair, in centimeters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair|Depth", meta=(ClampMin="0.0"))
    float MinDepthGapCm = 0.05f;

    /** Eye is not allowed to penetrate an occluder thicker than this. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair|Depth", meta=(ClampMin="0.0"))
    float MaxHairThicknessCm = 6.0f;

    /** Main scene depth must match the isolated hair depth within this tolerance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair|Depth", meta=(ClampMin="0.0"))
    float FrontDepthToleranceCm = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair|Depth", meta=(ClampMin="0.0"))
    float FaceDepthToleranceCm = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair|Depth", meta=(ClampMin="0.0001"))
    float EdgeFeatherCm = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair", meta=(ClampMin="0.0", ClampMax="1.0"))
    float OverlayOpacity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair", meta=(ClampMin="0.0", ClampMax="1.0"))
    float EyeHairLerp = 0.5f;

    /** Keep false if you want the capture size to follow the main internal render resolution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair|Capture")
    bool bIgnoreScreenPercentage = false;

    /** Fallback allocation; bMainViewResolution aligns the actual custom pass to the main view. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye Through Hair|Capture", meta=(ClampMin="64"))
    FIntPoint FallbackRenderTargetSize = FIntPoint(1920, 1080);
    
    UPROPERTY(EditAnywhere,meta=(ClampMin="0.25", ClampMax="1.0"))
    float RenderTargetScale = 1.0f;
    
    UFUNCTION(BlueprintCallable, Category="Eye Through Hair")
    void ReinitializeCaptures();

    UFUNCTION(BlueprintCallable, Category="Eye Through Hair")
    void SetSveEnable(bool enable);
    
    UFUNCTION(BlueprintCallable, Category="Eye Through Hair")
    void SetEyeHairLerp(float Value)
    {
        //EyeHairLerp = Value;
        //UpdateRenderSettings();
    };
    UFUNCTION(BlueprintCallable, Category="Eye Through Hair")
    void SetShaderSettings(const FEyeThroughHairSettings& Set)
    {
        MinDepthGapCm = Set.MinDepthGapCm;
        MaxHairThicknessCm = Set.MaxHairThicknessCm;
        FrontDepthToleranceCm = Set.FrontDepthToleranceCm;
        EdgeFeatherCm = Set.EdgeFeatherCm;
        OverlayOpacity = Set.OverlayOpacity;
        EyeHairLerp = Set.EyeHairLerp;
        FaceDepthToleranceCm = Set.FaceDepthToleranceCm;
        UpdateRenderSettings();
    };
private:
    //FEyeThroughHairSettings ShaderSettings;
    
    
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnRegister() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void InitializeCaptures();
    void InitializeCRP();
    void CreateRenderTargetsCRP(FIntPoint Size);
    void DestroyTargetComponent();
    void UpdateRenderSettings();
    //Capture做法
    /*
    UPROPERTY(Transient)
    TObjectPtr<USceneCaptureComponent2D> EyeColorCapture = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USceneCaptureComponent2D> EyeDepthCapture = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USceneCaptureComponent2D> HairDepthCapture = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> EyeColorTarget = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> EyeDepthTarget = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> HairDepthTarget = nullptr;
    */
    //ForTest
    //bool bUseCapture = false;

    //以下均为SceneCapture做法
    void DestroyCaptures();
    void ConfigureCapture(
        USceneCaptureComponent2D* Capture,
        UTextureRenderTarget2D* Target,
        UPrimitiveComponent* ShowOnly,
        ESceneCaptureSource Source,
        int32 SortPriority);
    void CreateRenderTargets(FIntPoint Size);
    void SyncCaptureToCamera(USceneCaptureComponent2D* Capture,const FVector& Location,const FRotator& Rotation,float FOV);
    void SyncCaptureToCamera(USceneCaptureComponent2D* Capture,const FMinimalViewInfo& ViewInfo);
    void PushResourcesAndSettingsToRenderThread();
};
