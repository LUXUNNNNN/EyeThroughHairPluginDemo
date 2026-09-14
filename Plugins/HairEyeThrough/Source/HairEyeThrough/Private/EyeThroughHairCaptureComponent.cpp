#include "EyeThroughHairCaptureComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HairEyeThroughModule.h"
#include "Kismet/GameplayStatics.h"
#include "Modules/ModuleManager.h"

namespace
{
    UTextureRenderTarget2D* MakeRenderTarget(
        UObject* Outer,
        const TCHAR* Name,
        const FIntPoint Size,
        ETextureRenderTargetFormat Format,
        const FLinearColor ClearColor)
    {
        UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(Outer, Name, RF_Transient);
        Target->RenderTargetFormat = Format;
        Target->ClearColor = ClearColor;
        Target->bAutoGenerateMips = false;
        Target->bForceLinearGamma = true;
        Target->InitAutoFormat(FMath::Max(1, Size.X), FMath::Max(1, Size.Y));
        Target->UpdateResourceImmediate(true);
        return Target;
    }
}

UEyeThroughHairCaptureComponent::UEyeThroughHairCaptureComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    //PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

}

void UEyeThroughHairCaptureComponent::OnRegister()
{
    Super::OnRegister();

    if (GetWorld() && GetWorld()->IsGameWorld())
    {
        //InitializeCaptures();
        //InitializeCRP();
    }
}

void UEyeThroughHairCaptureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    //UpdateRenderSettings();

    APlayerCameraManager* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!PlayerCameraManager)
        return;
    const FMinimalViewInfo& POV = PlayerCameraManager->GetCameraCacheView();
    //SceneCapture做法
    /*
    SyncCaptureToCamera(EyeColorCapture, POV);
    SyncCaptureToCamera(EyeDepthCapture, POV);
    SyncCaptureToCamera(HairDepthCapture, POV);

    ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    if (!LocalPlayer)
        return;
    UGameViewportClient* GameViewport =GetWorld()->GetGameViewport();
    if (!GameViewport || !GameViewport->Viewport)
        return;
    FSceneViewProjectionData ProjectionData;
    if (!LocalPlayer->GetProjectionData(GameViewport->Viewport,ProjectionData, INDEX_NONE))//从这里获取投影矩阵
        return;
    //获取投影矩阵
    const FMatrix ProjectionMatrix = ProjectionData.ProjectionMatrix;
    auto ApplyProjection = [&ProjectionMatrix](USceneCaptureComponent2D* Capture)
    {
        if (!Capture)
        {
            return;
        }
        Capture->bUseCustomProjectionMatrix = true;
        Capture->CustomProjectionMatrix = ProjectionMatrix;
    };

    ApplyProjection(EyeColorCapture);
    ApplyProjection(EyeDepthCapture);
    ApplyProjection(HairDepthCapture);

    FVector Location;
    FRotator Rotation;
    PlayerCameraManager->GetCameraViewPoint(Location, Rotation);
    const float FOV = PlayerCameraManager->GetFOVAngle();
    SyncCaptureToCamera(EyeColorCapture, Location, Rotation, FOV);
    SyncCaptureToCamera(EyeDepthCapture, Location, Rotation, FOV);
    SyncCaptureToCamera(HairDepthCapture, Location, Rotation, FOV);

    //输出摄像机信息
    const float CaptureHFOV =EyeColorCapture->FOVAngle;
    const float CaptureAspect =static_cast<float>(EyeColorTarget->SizeX) /static_cast<float>(EyeColorTarget->SizeY);
    const float CaptureVFOV =FMath::RadiansToDegrees(2.0f *FMath::Atan(FMath::Tan(FMath::DegreesToRadians(CaptureHFOV * 0.5f))/ CaptureAspect));
    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "Capture: HFOV=%.3f VFOV=%.3f Aspect=%.6f"),
        CaptureHFOV,
        CaptureVFOV,
        CaptureAspect);
    */

}

void UEyeThroughHairCaptureComponent::BeginPlay()
{
    Super::BeginPlay();
    //InitializeCaptures();
    InitializeCRP();
}

void UEyeThroughHairCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DestroyTargetComponent();
    Super::EndPlay(EndPlayReason);
}

void UEyeThroughHairCaptureComponent::ReinitializeCaptures()
{
    DestroyTargetComponent();
    //InitializeCaptures();
    InitializeCRP();
}

void UEyeThroughHairCaptureComponent::SetSveEnable(bool enable)
{
    FHairEyeThroughModule& Module = FModuleManager::LoadModuleChecked<FHairEyeThroughModule>(TEXT("HairEyeThrough"));
    const TSharedPtr<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> Extension = Module.GetViewExtension();
    Extension->bIsActive = enable;
}

void UEyeThroughHairCaptureComponent::UpdateRenderSettings()
{
    UE_LOG(LogTemp, Warning, TEXT("EyeThroughHairViewExtension UpdateRenderSettings"));  
    FHairEyeThroughModule& Module = FModuleManager::LoadModuleChecked<FHairEyeThroughModule>(TEXT("HairEyeThrough"));
    const TSharedPtr<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> Extension = Module.GetViewExtension();
    if (!Extension.IsValid())
    {
        return;
    }
    
    FEyeThroughHairSettings Settings;
    Settings.MinDepthGapCm = MinDepthGapCm;
    Settings.MaxHairThicknessCm = MaxHairThicknessCm;
    Settings.FrontDepthToleranceCm = FrontDepthToleranceCm;
    Settings.EdgeFeatherCm = EdgeFeatherCm;
    Settings.OverlayOpacity = OverlayOpacity;
    Settings.FaceDepthToleranceCm = FaceDepthToleranceCm;
    
    Settings.EyeHairLerp = EyeHairLerp;
    Extension->SetRenderSettings_GameThread(Settings);
}

void UEyeThroughHairCaptureComponent::CreateRenderTargets(const FIntPoint Size)
{
    // UE 5.4's conservative in-main-renderer path: BaseColor for eye color, SceneDepth for depths.
    //SceneCapture做法
    /*
    EyeColorTarget = MakeRenderTarget(
        this,
        TEXT("RT_EyeThroughHair_EyeColor"),
        Size,
        RTF_RGBA16f,
        FLinearColor(0, 0, 0, 0));

    constexpr float InvalidDepth = 100000000.0f;

    EyeDepthTarget = MakeRenderTarget(
        this,
        TEXT("RT_EyeThroughHair_EyeDepth"),
        Size,
        RTF_R32f,
        FLinearColor(InvalidDepth, InvalidDepth, InvalidDepth, InvalidDepth));

    HairDepthTarget = MakeRenderTarget(
        this,
        TEXT("RT_EyeThroughHair_HairDepth"),
        Size,
        RTF_R32f,
        FLinearColor(InvalidDepth, InvalidDepth, InvalidDepth, InvalidDepth));
    */
}

void UEyeThroughHairCaptureComponent::ConfigureCapture(
    USceneCaptureComponent2D* Capture,
    UTextureRenderTarget2D* Target,
    UPrimitiveComponent* ShowOnly,
    const ESceneCaptureSource Source,
    const int32 SortPriority)
{
    check(Capture);

    Capture->TextureTarget = Target;
    Capture->CaptureSource = Source;
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->ClearShowOnlyComponents();

    if (ShowOnly)
    {
        Capture->ShowOnlyComponent(ShowOnly);
    }
    Capture->ProjectionType =
        ECameraProjectionMode::Perspective;

    Capture->bCaptureEveryFrame = true;
    Capture->bCaptureOnMovement = false;
    Capture->bAlwaysPersistRenderingState = true;
    Capture->CaptureSortPriority = SortPriority;

    // UE 5.4 custom render pass integration.
    Capture->bRenderInMainRenderer = false;
    //Capture->bRenderInMainRenderer = Source==SCS_SceneDepth || Source==SCS_DeviceDepth; 
    
    Capture->PostProcessBlendWeight = 0.0f;
}

void UEyeThroughHairCaptureComponent::SyncCaptureToCamera(USceneCaptureComponent2D* Capture, const FVector& Location,
    const FRotator& Rotation, float FOV)
{
    if (!Capture)
    {return ;}
    
    Capture->SetWorldLocation(Location);
    Capture->SetWorldRotation(Rotation);
    Capture->FOVAngle = FOV;
    
}

void UEyeThroughHairCaptureComponent::SyncCaptureToCamera(USceneCaptureComponent2D* Capture,
    const FMinimalViewInfo& ViewInfo)
{
    Capture->SetWorldLocation(ViewInfo.Location);
    Capture->SetWorldRotation(ViewInfo.Rotation);
    /*
    Capture->FOVAngle = ViewInfo.FOV;
    Capture->ProjectionType = ViewInfo.ProjectionMode;
    if (ViewInfo.ProjectionMode ==
    ECameraProjectionMode::Orthographic)
    {
        Capture->OrthoWidth =
            ViewInfo.OrthoWidth;
    }
    */
}

void UEyeThroughHairCaptureComponent::InitializeCaptures()
{
    //SceneCapture做法
    /*
    if (bUseCapture)
    {
        if (EyeColorCapture || !GetOwner() || !GetWorld() || !GetWorld()->IsGameWorld())
        {
            return;
        }
    }
    */
    
    UE_LOG(LogTemp, Warning, TEXT("EyeThroughHairViewExtension Initialize"));

    //FHairEyeThroughModule* Module = FModuleManager::GetModulePtr<FHairEyeThroughModule>(TEXT("HairEyeThrough"));
    const FHairEyeThroughModule& Module = FModuleManager::LoadModuleChecked<FHairEyeThroughModule>(TEXT("HairEyeThrough"));
    //SceneCapture做法
    /*
    if (Module)
    {
        UE_LOG(LogTemp,Error,TEXT("HairEyeThrough module is not loaded."));
        return;
    }
    */
    const TSharedPtr<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> Extension = Module.GetViewExtension();

    if (!Extension.IsValid())
    {
        return;
    }
    UpdateRenderSettings();
    //使用SceneCapture的做法
    /*
    CreateRenderTargets(FallbackRenderTargetSize);
    EyeColorCapture = NewObject<USceneCaptureComponent2D>(GetOwner(), TEXT("SC_EyeThroughHair_EyeColor"));
    EyeDepthCapture = NewObject<USceneCaptureComponent2D>(GetOwner(), TEXT("SC_EyeThroughHair_EyeDepth"));
    HairDepthCapture = NewObject<USceneCaptureComponent2D>(GetOwner(), TEXT("SC_EyeThroughHair_HairDepth"));
    
    EyeColorCapture->RegisterComponent();
    EyeDepthCapture->RegisterComponent();
    HairDepthCapture->RegisterComponent();

    // The eye proxy should normally not be visible in the main scene.
    //EyeProxyComponent->SetVisibleInSceneCaptureOnly(true);
    ConfigureCapture(EyeColorCapture, EyeColorTarget, EyeProxyComponent, ESceneCaptureSource::SCS_BaseColor, 10);
    ConfigureCapture(EyeDepthCapture, EyeDepthTarget, EyeProxyComponent, ESceneCaptureSource::SCS_SceneDepth, 20);
    ConfigureCapture(HairDepthCapture, HairDepthTarget, FrontHairComponent, ESceneCaptureSource::SCS_SceneDepth, 30);
    
    PushResourcesAndSettingsToRenderThread();
    */
    
}

void UEyeThroughHairCaptureComponent::InitializeCRP()
{
    UE_LOG(LogTemp, Warning, TEXT("EyeThroughHairViewExtension Start Initialize"));
    /*
    ShaderSettings.EyeHairLerp = this->EyeHairLerp;
    ShaderSettings.EdgeFeatherCm = this->EdgeFeatherCm;
    ShaderSettings.OverlayOpacity = this->OverlayOpacity;
    ShaderSettings.FrontDepthToleranceCm = this->FrontDepthToleranceCm;
    ShaderSettings.MaxHairThicknessCm = this->MaxHairThicknessCm;
    ShaderSettings.MinDepthGapCm = this->MinDepthGapCm;
    */
    
    //将picker引用的对象类型转换为PrimitiveComponent
    UPrimitiveComponent* EyeProxyComponent = Cast<UPrimitiveComponent>(EyeProxyComponentRef.GetComponent(GetOwner()));
    UPrimitiveComponent* FrontHairComponent = Cast<UPrimitiveComponent>(FrontHairComponentRef.GetComponent(GetOwner()));
    UPrimitiveComponent* FaceComponent = Cast<UPrimitiveComponent>(FaceComponentRef.GetComponent(GetOwner()));
    
    if (!EyeProxyComponent || !FrontHairComponent)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("EyeThroughHair: assign EyeProxyComponent and FrontHairComponent on %s."),
            *GetNameSafe(GetOwner()));
        return;
    }
    
    // 当前调试固定 1920x1080
    FIntPoint RenderTargetSize = FIntPoint(1920* RenderTargetScale, 1080* RenderTargetScale) ;
    CreateRenderTargetsCRP(RenderTargetSize);
    //CreateRenderTargets(FIntPoint(1920, 1080));

    //FHairEyeThroughModule* Module = FModuleManager::GetModulePtr<FHairEyeThroughModule>(TEXT("HairEyeThrough"));
    const FHairEyeThroughModule& Module = FModuleManager::LoadModuleChecked<FHairEyeThroughModule>(TEXT("HairEyeThrough"));
    const TSharedPtr<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> Extension = Module.GetViewExtension();
    if (!Extension.IsValid())
    {
        return;
    }
    //指定画谁
    Extension->SetTargetComponents_GameThread(EyeProxyComponent, FrontHairComponent, FaceComponent);
    // 指定画到哪
    Extension->SetOutputTargets_GameThread(EyeColorDepthTarget,HairDepthTarget, FaceDepthTarget);
    UpdateRenderSettings();
}

void UEyeThroughHairCaptureComponent::CreateRenderTargetsCRP(FIntPoint Size)
{
    EyeColorDepthTarget = MakeRenderTarget(this,TEXT("RT_EyeThroughHair_EyeColorDepth"),
        Size,RTF_RGBA16f,FLinearColor(0, 0, 0, 65504.0f));
    HairDepthTarget = MakeRenderTarget(this, TEXT("RT_EyeThroughHair_HairDepth"),
        Size,RTF_R32f, FLinearColor(100000000.0f,100000000.0f,100000000.0f,100000000.0f));
    FaceDepthTarget = MakeRenderTarget(this, TEXT("RT_EyeThroughHair_FaceDepth"),
        Size,RTF_R32f, FLinearColor(100000000.0f,100000000.0f,100000000.0f,100000000.0f));
    //UObject 在同一个 Outer 下依赖 Name 唯一标识对象，名字不能串
}

void UEyeThroughHairCaptureComponent::DestroyTargetComponent()
{
    const FHairEyeThroughModule* Module = FModuleManager::GetModulePtr<FHairEyeThroughModule>(TEXT("HairEyeThrough"));
    if (!Module)
        return;
    const TSharedPtr<FEyeThroughHairViewExtension> Extension = Module->GetViewExtension();
    if (!Extension.IsValid())
        return;
    
    Extension->ClearTargetComponents_GameThread();
    Extension->ClearOutputTargets_GameThread();
    
    EyeColorDepthTarget = nullptr;
    HairDepthTarget = nullptr;
    FaceDepthTarget = nullptr;
}

void UEyeThroughHairCaptureComponent::DestroyCaptures()
{
    //SceneCapture做法
    /*
    FHairEyeThroughModule* Module = FModuleManager::GetModulePtr<FHairEyeThroughModule>(TEXT("HairEyeThrough"));
    if (Module)
    {
        if (const TSharedPtr<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> Extension = Module->GetViewExtension())
        {

            Extension->ClearCaptureResources_GameThread();
        }
    }

    auto DestroyCapture = [](TObjectPtr<USceneCaptureComponent2D>& Capture)
    {
        if (Capture)
        {
            Capture->DestroyComponent();
            Capture = nullptr;
        }
    };


    DestroyCapture(EyeColorCapture);
    DestroyCapture(EyeDepthCapture);
    DestroyCapture(HairDepthCapture);

    EyeColorTarget = nullptr;
    EyeDepthTarget = nullptr;
    HairDepthTarget = nullptr;
    */
}

void UEyeThroughHairCaptureComponent::PushResourcesAndSettingsToRenderThread()
{
    //SceneCapture做法
    /*
    FHairEyeThroughModule& Module = FModuleManager::LoadModuleChecked<FHairEyeThroughModule>(TEXT("HairEyeThrough"));
    const TSharedPtr<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> Extension = Module.GetViewExtension();
    if (!Extension.IsValid())
    {
        return;
    }

    FEyeThroughHairSettings Settings;
    Settings.MinDepthGapCm = MinDepthGapCm;
    Settings.MaxHairThicknessCm = MaxHairThicknessCm;
    Settings.FrontDepthToleranceCm = FrontDepthToleranceCm;
    Settings.EdgeFeatherCm = EdgeFeatherCm;
    Settings.OverlayOpacity = OverlayOpacity;
    //ForTest
    Settings.EyeHairLerp = EyeHairLerp;
    Extension->SetCaptureResources_GameThread(
        EyeColorTarget,
        EyeDepthTarget,
        HairDepthTarget,
        Settings);
    */    
    
}
