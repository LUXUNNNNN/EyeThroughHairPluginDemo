#include "EyeThroughHairViewExtension.h"

#include "Engine/TextureRenderTarget2D.h"
#include "EyeThroughHairShaders.h"
#include "NetworkMessage.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessInputs.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "SceneRenderTargetParameters.h"
#include "ScreenPass.h"
#include "Runtime/Renderer/Private/SceneRendering.h"

FEyeThroughHairViewExtension::FEyeThroughHairViewExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{
    //可在创建指针时先进行一次参数传入更新
    const FIntPoint InitialSize(1920, 1080);
    /*
    EyeColorDepthPass =
        MakeUnique<FEyeThroughHairCustomRenderPass>(
            TEXT("EyeThroughHair.EyeColor"),
            FCustomRenderPassBase::ERenderMode::DepthAndBasePass,
            FCustomRenderPassBase::ERenderOutput::SceneColorAndDepth, InitialSize,PF_FloatRGBA);

    HairDepthPass = MakeUnique<FEyeThroughHairCustomRenderPass>(
        TEXT("EyeThroughHair.HairDepth"),
        FCustomRenderPassBase::ERenderMode::DepthPass,
        FCustomRenderPassBase::ERenderOutput::SceneDepth,InitialSize,PF_R32_FLOAT);
    */
    //EyeDepthPass =MakeUnique<FEyeThroughHairCustomRenderPass>(TEXT("EyeThroughHair.EyeDepth")FCustomRenderPassBase::ERenderMode::DepthPass,FCustomRenderPassBase::ERenderOutput::SceneDepth,InitialSize, PF_R32_FLOAT);
}

void FEyeThroughHairViewExtension::SetOutputTargets_GameThread(UTextureRenderTarget2D* InEyeColorDepthTarget,
    UTextureRenderTarget2D* InHairDepthTarget, UTextureRenderTarget2D* InFaceDepthTarget)
{
    check(IsInGameThread());

    // GT 后面 BeginRenderViewFamily() 要用
    EyeColorDepthTarget_GT = InEyeColorDepthTarget;
    HairDepthTarget_GT = InHairDepthTarget;
    FaceDepthTarget_GT = InFaceDepthTarget;
    
    // 找到 RT 对应的渲染资源
    FRenderTarget* EyeRT = InEyeColorDepthTarget? InEyeColorDepthTarget->GameThread_GetRenderTargetResource(): nullptr;
    FRenderTarget* HairRT = InHairDepthTarget? InHairDepthTarget->GameThread_GetRenderTargetResource(): nullptr;
    FRenderTarget* FaceRT = InFaceDepthTarget? InFaceDepthTarget->GameThread_GetRenderTargetResource(): nullptr;
    
    const TWeakPtr<FEyeThroughHairViewExtension,ESPMode::ThreadSafe> WeakPtr = StaticCastSharedRef<FEyeThroughHairViewExtension>(AsShared());
    ENQUEUE_RENDER_COMMAND(EyeThroughHair_SetOutputTargets)(
        [WeakPtr, EyeRT, HairRT, FaceRT]
        (FRHICommandListImmediate& RHICmdList)
        {
            if (const auto Self =WeakPtr.Pin())
            {
                Self->SetOutputTargets_RenderThread(EyeRT,HairRT, FaceRT);
            }
        });
}

void FEyeThroughHairViewExtension::ClearOutputTargets_GameThread()
{
    check(IsInGameThread());
    //分别清理游戏线程和渲染线程资源
    //游戏线程
    EyeColorDepthTarget_GT = nullptr;
    HairDepthTarget_GT = nullptr;
    FaceDepthTarget_GT = nullptr;
    //渲染线程
    const TWeakPtr<FEyeThroughHairViewExtension,ESPMode::ThreadSafe> WeakPtr =StaticCastSharedRef<FEyeThroughHairViewExtension>(AsShared());
    ENQUEUE_RENDER_COMMAND(EyeThroughHair_ClearOutputTargets)(
        [WeakPtr]
        (FRHICommandListImmediate& RHICmdList)
        {
            if (const auto Self = WeakPtr.Pin())
            {
                Self->SetOutputTargets_RenderThread(nullptr,nullptr, nullptr);
            }
        });
}

void FEyeThroughHairViewExtension::SetOutputTargets_RenderThread(FRenderTarget* InEyeColorDepthTarget, FRenderTarget* InHairDepthTarget,FRenderTarget* InFaceDepthTarget)
{
    check(IsInRenderingThread());
    //将RT弱指针指向对应的RT资源
    EyeColorDepthTarget_RT = InEyeColorDepthTarget;
    HairDepthTarget_RT = InHairDepthTarget;
    FaceDepthTarget_RT = InFaceDepthTarget;
}

void FEyeThroughHairViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
    check(IsInGameThread());

    if (!bIsActive ||!InViewFamily.Scene ||InViewFamily.Views.IsEmpty())
        return;

    //获取要绘制的目标
    UPrimitiveComponent* EyeProxy = EyeProxyComponent.Get();
    UPrimitiveComponent* FrontHairProxy = FrontHairComponent.Get();
    UPrimitiveComponent* FaceProxy = FaceComponent.Get();

        
    //注册和保护
    if (!EyeProxy || !FrontHairProxy || !FaceProxy)
        return;
    if (!EyeProxy->IsRegistered() || !FrontHairProxy->IsRegistered() || !FaceProxy->IsRegistered())
        return;

    //游戏线程的Target资源，准备扔给临时CRP
    UTextureRenderTarget2D* EyeTarget = EyeColorDepthTarget_GT.Get();
    UTextureRenderTarget2D* HairTarget = HairDepthTarget_GT.Get();
    UTextureRenderTarget2D*  FaceTarget = FaceDepthTarget_GT.Get();
    if (!EyeTarget || !HairTarget || !FaceTarget)
        return;
    
    //UE_LOG(LogTemp,Warning,TEXT("获取到游戏线程Target"))
    // 非常重要：只提交给目标组件所属的 Scene
    UWorld* TargetWorld = EyeProxy->GetWorld();
    if (!TargetWorld || TargetWorld->Scene != InViewFamily.Scene)
        return;
    
    const FSceneView* MainView = InViewFamily.Views[0];
    if (!MainView)
        return;
    if (MainView->bIsSceneCapture)//不要把CRP提交给场景捕获
    {
        return;
    }
    //调试输出Aspect
    const FMatrix& P =MainView->ViewMatrices.GetProjectionMatrix();
    const float ProjectionAspect =P.M[1][1] / P.M[0][0];
    const float RTAspect =1920.0f / 1080.0f;
    UE_LOG(LogTemp,Warning,TEXT("Projection: M00=%.6f M11=%.6f ""ProjAspect=%.6f RTAspect=%.6f"),
        P.M[0][0],P.M[1][1],
        ProjectionAspect,RTAspect);

    
   //UE_LOG(LogTemp,Warning,TEXT("开始添加Pass"))
    auto AddPass =[&InViewFamily, MainView](UPrimitiveComponent* ShowOnlyComponent,const FString& DebugName,
        FCustomRenderPassBase::ERenderMode RenderMode,FCustomRenderPassBase::ERenderOutput RenderOutput,
        UTextureRenderTarget2D* RenderTarget)
        {
        if (!ShowOnlyComponent || !RenderTarget)
            return;
        FSceneInterface::FCustomRenderPassRendererInput PassInput{};
        FEyeThroughHairCustomRenderPass* CustomPass =new FEyeThroughHairCustomRenderPass(DebugName,RenderMode,RenderOutput,RenderTarget);
        PassInput.CustomRenderPass = CustomPass;
        PassInput.ViewLocation = MainView->ViewLocation;
        PassInput.ViewRotationMatrix = MainView->SceneViewInitOptions.ViewRotationMatrix;
        PassInput.ProjectionMatrix = MainView->ViewMatrices.GetProjectionMatrix();
        PassInput.ViewActor = MainView->ViewActor;
        const FPrimitiveComponentId PrimitiveId = ShowOnlyComponent->GetPrimitiveSceneId();
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "CRP %s Component=%s Owner=%s "
                "PrimId=%u Registered=%d RenderState=%d"),
            *DebugName,
            *GetNameSafe(ShowOnlyComponent),
            *GetNameSafe(ShowOnlyComponent->GetOwner()),
            PrimitiveId.PrimIDValue,
            ShowOnlyComponent->IsRegistered(),
            ShowOnlyComponent->IsRenderStateCreated());

        PassInput.ShowOnlyPrimitives.Emplace();
        PassInput.ShowOnlyPrimitives->Add(PrimitiveId);
        InViewFamily.Scene->AddCustomRenderPass(&InViewFamily,PassInput);
        //UE_LOG(LogTemp, Warning, TEXT("成功添加Pass"));
        };

    AddPass(
        EyeProxy,
        TEXT("EyeThroughHair.EyeColorDepth"),
        FCustomRenderPassBase::ERenderMode::DepthAndBasePass,
        FCustomRenderPassBase::ERenderOutput::SceneColorAndDepth,
        EyeTarget);

    AddPass(
        FrontHairProxy,
        TEXT("EyeThroughHair.HairDepth"),
        FCustomRenderPassBase::ERenderMode::DepthPass,
        FCustomRenderPassBase::ERenderOutput::SceneDepth,
        HairTarget);

    AddPass(
        FaceProxy,
        TEXT("EyeThroughHair.FaceDepth"),
        FCustomRenderPassBase::ERenderMode::DepthPass,
        FCustomRenderPassBase::ERenderOutput::SceneDepth,
        FaceTarget);
}

bool FEyeThroughHairViewExtension::IsActiveThisFrame_Internal(
    const FSceneViewExtensionContext& Context) const
{
    //保存场景
    return true;
    /*
    return bIsActive
        && TargetScene_GT != nullptr
        && Context.Scene == TargetScene_GT;
    */
}

void FEyeThroughHairViewExtension::SubscribeToPostProcessingPass(
    const EPostProcessingPass Pass,
    FAfterPassCallbackDelegateArray& InOutPassCallbacks,
    const bool bIsPassEnabled)
{
}


void FEyeThroughHairViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
    const FPostProcessingInputs& Inputs)
{
    check(IsInRenderingThread());

    FSceneViewExtensionBase::PrePostProcessPass_RenderThread(GraphBuilder, View, Inputs);
    if (!bIsActive)
    {
        return;
    }
    //保存场景
    /*
    if (!View.Family ||
    View.Family->Scene != TargetScene_RT)
    {
        return;
    }
    */
    // SceneCapture 本身也会创建 View，会运行到这段，因此需要截断。
    // 不要把合成效果再次写进辅助捕获。
    if (View.bIsSceneCapture)
    {
        return ;
    }
    
    //SceneCapture做法
    /*
    if (!EyeColorRHI || !EyeDepthRHI || !HairDepthRHI)
    {
        return;
    }
    */


    /*
    * 这个公共接口版本先假定：
     *
     * r.ScreenPercentage = 100
     * 单游戏视图
     * 非分屏
     * 非 VR
     *
     * 动态分辨率部分后面单独处理。
     */
    FRDGTextureRef SceneColorTexture = (*Inputs.SceneTextures)->SceneColorTexture;//从Inputs中拿到SceneTexture中的SceneColorTexture
    if (!SceneColorTexture)
    {
        return;
    }
    const FIntPoint SceneExtent = SceneColorTexture->Desc.Extent;
    const FIntRect ViewRect = static_cast<const FViewInfo&>(View).ViewRect;
    //const FIntRect ViewRect = View.UnscaledViewRect;
    /*输出ViewRect和FOV信息
    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "SceneColor Extent = %d x %d, "
            "ViewRect = (%d,%d)-(%d,%d)"),
        SceneExtent.X,
        SceneExtent.Y,
        ViewRect.Min.X,
        ViewRect.Min.Y,
        ViewRect.Max.X,
        ViewRect.Max.Y);

    const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);
    const FMatrix& ProjectionMatrix =ViewInfo.ViewMatrices.GetProjectionMatrix();
    
    const float MainHFovRadians =2.0f * FMath::Atan(1.0f / ProjectionMatrix.M[0][0]);
    const float MainVFovRadians =2.0f * FMath::Atan(1.0f / ProjectionMatrix.M[1][1]);
    const float MainHFovDegrees =FMath::RadiansToDegrees(MainHFovRadians);
    const float MainVFovDegrees =FMath::RadiansToDegrees(MainVFovRadians);
    UE_LOG(LogTemp,Warning,TEXT("MainView: HFOV=%.3f VFOV=%.3f ""P00=%.6f P11=%.6f"),MainHFovDegrees,MainVFovDegrees,ProjectionMatrix.M[0][0],ProjectionMatrix.M[1][1]);
    */
    

    
    if (ViewRect.IsEmpty())
    {return;}
    //UE_LOG(LogTemp, Warning, TEXT("EyeColorDepthTexture已创建"));
    const FScreenPassTexture SceneColor(SceneColorTexture, ViewRect);
    //CRP已在之前的PreRender中完成对两张Target渲染资源的写入，此处拿到写入结果的RDG资源准备传给Shader
    FRDGTextureRef EyeColorDepthTexture = EyeColorDepthTarget_RT ? EyeColorDepthTarget_RT->GetRenderTargetTexture(GraphBuilder) : nullptr;
    FRDGTextureRef HairDepthTexture = HairDepthTarget_RT ? HairDepthTarget_RT->GetRenderTargetTexture(GraphBuilder) : nullptr;
    FRDGTextureRef FaceDepthTexture = FaceDepthTarget_RT ? FaceDepthTarget_RT->GetRenderTargetTexture(GraphBuilder) : nullptr;
    if (!EyeColorDepthTexture || !HairDepthTexture || !FaceDepthTexture)
        return;

    //输出SceneColorTexture、EyeColorTexture以及ViewRect的尺寸，查看是否对齐
    UE_LOG(LogTemp,Warning,TEXT("SceneExtent=%dx%d ""ViewRect=(%d,%d)-(%d,%d) Size=%dx%d ""EyeRT=%dx%d"),
    SceneColorTexture->Desc.Extent.X,SceneColorTexture->Desc.Extent.Y,
    ViewRect.Min.X,ViewRect.Min.Y,ViewRect.Max.X,ViewRect.Max.Y,
    ViewRect.Width(),ViewRect.Height(),
    EyeColorDepthTexture->Desc.Extent.X,EyeColorDepthTexture->Desc.Extent.Y);
    /*
    FRDGTextureRef EyeDepthTexture = EyeDepthPass ? EyeDepthPass->ConsumeOutput_RenderThread() : nullptr;
    if (!EyeColorTexture ||!EyeDepthTexture ||!HairDepthTexture)
        return;
    */
    /*
    FRDGTextureRef EyeColorTexture = GraphBuilder.RegisterExternalTexture(
        CreateRenderTarget(EyeColorRHI, TEXT("EyeThroughHair.EyeColor")));
    FRDGTextureRef EyeDepthTexture = GraphBuilder.RegisterExternalTexture(
        CreateRenderTarget(EyeDepthRHI, TEXT("EyeThroughHair.EyeDepth")));
    FRDGTextureRef HairDepthTexture = GraphBuilder.RegisterExternalTexture(
        CreateRenderTarget(HairDepthRHI, TEXT("EyeThroughHair.HairDepth")));
    */
    
    //创建临时输出
    FRDGTextureDesc CompositeDesc = SceneColorTexture->Desc;
    EnumAddFlags(CompositeDesc.Flags, ETextureCreateFlags::RenderTargetable);
    EnumAddFlags(CompositeDesc.Flags, ETextureCreateFlags::ShaderResource);
    //Presentable表示需要进入交换链并最终显示到屏幕上的Buffer，中间各种rt都不需要，应该禁用
    EnumRemoveFlags(CompositeDesc.Flags, ETextureCreateFlags::Presentable);
    CompositeDesc.ClearValue = FClearValueBinding::None;

    FRDGTextureRef CompositeTexture = GraphBuilder.CreateTexture(CompositeDesc, TEXT("EyeThroughHair.Composite"));
    //利用合成纹理创建一张ScreenPassRT，作为往屏幕空间上绘制的容器
    const FScreenPassRenderTarget Output(CompositeTexture,ViewRect, ERenderTargetLoadAction::ENoAction);
    //获取SceneColor的extent，Rect值,决定屏幕空间RT的尺寸。 FScreenPassTextureViewport包含Extent和Rect两个属性
    const FScreenPassTextureViewport InputViewport(SceneColor);
    const FScreenPassTextureViewport OutputViewport(Output);


    FEyeThroughHairPS::FParameters* Parameters = GraphBuilder.AllocParameters<FEyeThroughHairPS::FParameters>();
    //获取当前View信息以及在后处理阶段开始前的Scenetexture（这里是Depth）
    Parameters->View = View.ViewUniformBuffer;
    Parameters->SceneTextures = CreateSceneTextureShaderParameters(GraphBuilder, View, ESceneTextureSetupMode::SceneDepth);
    //纹理和sampler
    Parameters->SceneColorTexture = SceneColor.Texture;
    Parameters->SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Parameters->EyeColorDepthTexture = EyeColorDepthTexture;
    Parameters->EyeColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Parameters->EyeDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Parameters->HairDepthTexture = HairDepthTexture;
    Parameters->HairDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Parameters->FaceDepthTexture = FaceDepthTexture;
    Parameters->FaceDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    //Parameters->EyeDepthTexture = EyeDepthTexture;
    //Parameters->EyeDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

    //用于计算基于ViewRect的UV
    Parameters->ViewRectMin = FVector2f(static_cast<float>(ViewRect.Min.X),static_cast<float>(ViewRect.Min.Y));
    Parameters->InvViewRectSize = FVector2f(1/static_cast<float>(ViewRect.Width()),1/static_cast<float>(ViewRect.Height()));
    //参数
    Parameters->MinDepthGapCm = RenderSettings.MinDepthGapCm;
    Parameters->MaxHairThicknessCm = RenderSettings.MaxHairThicknessCm;
    Parameters->FrontDepthToleranceCm = RenderSettings.FrontDepthToleranceCm;
    Parameters->EdgeFeatherCm = FMath::Max(RenderSettings.EdgeFeatherCm,0.0001f);
    Parameters->OverlayOpacity = RenderSettings.OverlayOpacity;
    Parameters->InvalidDepthCm = 100000000.0f;
    Parameters->FaceDepthToleranceCm = RenderSettings.FaceDepthToleranceCm;
    
    //ForTest
    Parameters->EyeHairLerp = RenderSettings.EyeHairLerp;
    
    //把SceneColor作为SRV读取到PS，计算完成后将结果写入CompositeTexture RTV，但不写入SceneColor，后续再将结果复制给SceneColor RTV，避免同一个pass中的读写冲突
    Parameters->RenderTargets[0] = Output.GetRenderTargetBinding();
    
    const FGlobalShaderMap* ViewShaderMap = static_cast<const FViewInfo&>(View).ShaderMap;
    TShaderMapRef<FScreenPassVS> VertexShader(ViewShaderMap);
    TShaderMapRef<FEyeThroughHairPS> PixelShader(ViewShaderMap);
    //修改 Parameters 中未使用资源的引用，清理掉未使用的参数，使RDG不追踪它
    ClearUnusedGraphResources(PixelShader,Parameters);
    //AddDrawScreenPass能使用ScreenPassVS自动生成UV，处理缩放等
    AddDrawScreenPass(GraphBuilder,
        RDG_EVENT_NAME(""
        "EyeThroughHair Composite Before PostProcess"),
        View,
        OutputViewport,
        InputViewport,
        VertexShader,
        PixelShader,
        Parameters
        );

    //把当前
    FRHICopyTextureInfo CopyInfo;
    CopyInfo.SourcePosition = FIntVector(ViewRect.Min.X, ViewRect.Min.Y, 0);
    CopyInfo.DestPosition = FIntVector(ViewRect.Min.X, ViewRect.Min.Y, 0);
    CopyInfo.Size = FIntVector(ViewRect.Width(), ViewRect.Height(), 1);
    AddCopyTexturePass(
        GraphBuilder,
        CompositeTexture,
        SceneColorTexture,
        CopyInfo);
    
}

//Capture做法，目前不会被调用
void FEyeThroughHairViewExtension::SetCaptureResources_GameThread(
    UTextureRenderTarget2D* EyeColor,
    UTextureRenderTarget2D* EyeDepth,
    UTextureRenderTarget2D* HairDepth,
    const FEyeThroughHairSettings& Settings)
{
    check(IsInGameThread());

    FTextureRenderTargetResource* EyeColorResource =
        EyeColor ? EyeColor->GameThread_GetRenderTargetResource() : nullptr;
    FTextureRenderTargetResource* EyeDepthResource =
        EyeDepth ? EyeDepth->GameThread_GetRenderTargetResource() : nullptr;
    FTextureRenderTargetResource* HairDepthResource =
        HairDepth ? HairDepth->GameThread_GetRenderTargetResource() : nullptr;

    const TSharedRef<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> Self =
        StaticCastSharedRef<FEyeThroughHairViewExtension>(AsShared());

    UE_LOG(LogTemp, Log, TEXT("EyeThroughHairViewExtension SetCaptureRenderThread"));
    
    ENQUEUE_RENDER_COMMAND(EyeThroughHair_SetCaptureResources)(
        [Self, EyeColorResource, EyeDepthResource, HairDepthResource, Settings]
        (FRHICommandListImmediate& RHICmdList)
        {
            
            Self->SetCaptureResources_RenderThread(
                EyeColorResource ? EyeColorResource->GetRenderTargetTexture() : nullptr,
                EyeDepthResource ? EyeDepthResource->GetRenderTargetTexture() : nullptr,
                HairDepthResource ? HairDepthResource->GetRenderTargetTexture() : nullptr,
                Settings);
                
        });
}

void FEyeThroughHairViewExtension::ClearCaptureResources_GameThread()
{
    check(IsInGameThread());

    const TSharedRef<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> Self =
        StaticCastSharedRef<FEyeThroughHairViewExtension>(AsShared());

    ENQUEUE_RENDER_COMMAND(EyeThroughHair_ClearCaptureResources)(
        [Self](FRHICommandListImmediate& RHICmdList)
        {
            Self->SetCaptureResources_RenderThread(nullptr, nullptr, nullptr, FEyeThroughHairSettings{});
        });
}

void FEyeThroughHairViewExtension::SetCaptureResources_RenderThread(
    FTextureRHIRef InEyeColor,
    FTextureRHIRef InEyeDepth,
    FTextureRHIRef InHairDepth,
    const FEyeThroughHairSettings& InSettings)
{
    check(IsInRenderingThread());
    //SceneCapture做法
    /*
    EyeColorRHI = MoveTemp(InEyeColor);
    EyeDepthRHI = MoveTemp(InEyeDepth);
    HairDepthRHI = MoveTemp(InHairDepth);
    */
    RenderSettings = InSettings;
    
}

void FEyeThroughHairViewExtension::SetRenderSettings_GameThread(const FEyeThroughHairSettings& NewSettings)
{
    check(IsInGameThread());

    const TWeakPtr<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> WeakPtr =
        StaticCastSharedRef<FEyeThroughHairViewExtension>(AsShared());
    
    ENQUEUE_RENDER_COMMAND(EyeThroughHair_UpdateSettings)(
        [WeakPtr, NewSettings]
        (FRHICommandListImmediate& RHICmdList)
        {
            if (const TSharedPtr<
                    FEyeThroughHairViewExtension,
                    ESPMode::ThreadSafe> Pinned =
                    WeakPtr.Pin())//Pin将WeakPtr转化成SharedPtr，用于访问对象。如果转换失败，说明这个对象已被销毁，可以防止空指针
            {
                Pinned->SetRenderSettings_RenderThread(
                    NewSettings);
            }
        });
}

void FEyeThroughHairViewExtension::SetTargetComponents_GameThread(UPrimitiveComponent* EyeProxy,
    UPrimitiveComponent* FrontHair, UPrimitiveComponent* Face)
{
    check(IsInGameThread());
    EyeProxyComponent = EyeProxy;
    FrontHairComponent = FrontHair;
    FaceComponent = Face;

    //保存场景
    /*
    TargetScene_GT =
        (EyeProxy && EyeProxy->GetWorld())
        ? EyeProxy->GetWorld()->Scene
        : nullptr;

    FSceneInterface* Scene = TargetScene_GT;

    const TWeakPtr<
        FEyeThroughHairViewExtension,
        ESPMode::ThreadSafe> WeakPtr =
        StaticCastSharedRef<
            FEyeThroughHairViewExtension>(AsShared());

    ENQUEUE_RENDER_COMMAND(EyeThroughHair_SetTargetScene)(
        [WeakPtr, Scene](FRHICommandListImmediate&)
        {
            if (const auto Self = WeakPtr.Pin())
            {
                Self->TargetScene_RT = Scene;
            }
        });
        */
    //保存场景
}

void FEyeThroughHairViewExtension::ClearTargetComponents_GameThread()
{
    EyeProxyComponent = nullptr;
    FrontHairComponent = nullptr;
    FaceComponent = nullptr;

    //保存场景
    /*
    TargetScene_GT = nullptr;
    const TWeakPtr<
        FEyeThroughHairViewExtension,
        ESPMode::ThreadSafe> WeakPtr =
        StaticCastSharedRef<
            FEyeThroughHairViewExtension>(AsShared());

    ENQUEUE_RENDER_COMMAND(EyeThroughHair_ClearTargetScene)(
        [WeakPtr](FRHICommandListImmediate&)
        {
            if (const auto Self = WeakPtr.Pin())
            {
                Self->TargetScene_RT = nullptr;
            }
        });
        */
    //保存场景
}



void FEyeThroughHairViewExtension::SetRenderSettings_RenderThread(const FEyeThroughHairSettings& NewSettings)
{
    check(IsInRenderingThread());

    RenderSettings = NewSettings;
} 


/*
FScreenPassTexture FEyeThroughHairViewExtension::Composite_RenderThread(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessMaterialInputs& Inputs)
{

    check(IsInRenderingThread());

    const FScreenPassTexture SceneColor(Inputs.GetInput(EPostProcessMaterialInput::SceneColor));

    if (!SceneColor.IsValid() || !EyeColorRHI || !EyeDepthRHI || !HairDepthRHI)
    {
        return SceneColor;
    }

    FRDGTextureRef EyeColorTexture = GraphBuilder.RegisterExternalTexture(
        CreateRenderTarget(EyeColorRHI, TEXT("EyeThroughHair.EyeColor")));
    FRDGTextureRef EyeDepthTexture = GraphBuilder.RegisterExternalTexture(
        CreateRenderTarget(EyeDepthRHI, TEXT("EyeThroughHair.EyeDepth")));
    FRDGTextureRef HairDepthTexture = GraphBuilder.RegisterExternalTexture(
        CreateRenderTarget(HairDepthRHI, TEXT("EyeThroughHair.HairDepth")));

    FScreenPassRenderTarget Output = Inputs.OverrideOutput;
    if (!Output.IsValid())
    {
        Output = FScreenPassRenderTarget::CreateFromInput(
            GraphBuilder,
            SceneColor,
            View.GetOverwriteLoadAction(),
            TEXT("EyeThroughHair.Composite"));
    }

    const FScreenPassTextureViewport InputViewport(SceneColor);
    const FScreenPassTextureViewport OutputViewport(Output);

    FEyeThroughHairPS::FParameters* Parameters =
        GraphBuilder.AllocParameters<FEyeThroughHairPS::FParameters>();

    Parameters->View = View.ViewUniformBuffer;
    Parameters->SceneTextures = CreateSceneTextureShaderParameters(
        GraphBuilder,
        View,
        ESceneTextureSetupMode::SceneDepth);

    Parameters->SceneColorTexture = SceneColor.Texture;
    Parameters->SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

    Parameters->EyeColorTexture = EyeColorTexture;
    Parameters->EyeColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

    Parameters->EyeDepthTexture = EyeDepthTexture;
    Parameters->EyeDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

    Parameters->HairDepthTexture = HairDepthTexture;
    Parameters->HairDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

    Parameters->MinDepthGapCm = RenderSettings.MinDepthGapCm;
    Parameters->MaxHairThicknessCm = RenderSettings.MaxHairThicknessCm;
    Parameters->FrontDepthToleranceCm = RenderSettings.FrontDepthToleranceCm;
    Parameters->EdgeFeatherCm = FMath::Max(RenderSettings.EdgeFeatherCm, 0.0001f);
    Parameters->OverlayOpacity = RenderSettings.OverlayOpacity;
    Parameters->InvalidDepthCm = 100000000.0f;
    Parameters->RenderTargets[0] = Output.GetRenderTargetBinding();

    TShaderMapRef<FScreenPassVS> VertexShader(View.ShaderMap);
    TShaderMapRef<FEyeThroughHairPS> PixelShader(View.ShaderMap);

    ClearUnusedGraphResources(PixelShader, Parameters);

    AddDrawScreenPass(
        GraphBuilder,
        RDG_EVENT_NAME("EyeThroughHair Composite"),
        View,
        OutputViewport,
        InputViewport,
        VertexShader,
        PixelShader,
        Parameters);

    return MoveTemp(Output);
}
*/
