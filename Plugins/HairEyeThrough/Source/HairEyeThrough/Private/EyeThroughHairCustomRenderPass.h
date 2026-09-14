#pragma once
#include"CoreMinimal.h"
#include "RenderGraphBuilder.h"
#include "Engine/TextureRenderTarget2D.h"
#include"Rendering/CustomRenderPass.h"


class FEyeThroughHairCustomRenderPass final : public FCustomRenderPassBase
{
private:
	//bool bProducedThisFrame = false;
	//EPixelFormat OutputFormat = PF_Unknown;
	FRenderTarget* OutRenderTarget = nullptr;
public:
	IMPLEMENT_CUSTOM_RENDER_PASS(FEyeThroughHairCustomRenderPass)
	FEyeThroughHairCustomRenderPass(const FString& InDebugName,ERenderMode InRenderMode,
		ERenderOutput InRenderOutput,UTextureRenderTarget2D* InRenderTarget)
		: FCustomRenderPassBase(
			InDebugName,
			InRenderMode,
			InRenderOutput,
			FIntPoint(InRenderTarget->SizeX,InRenderTarget->SizeY))
	{
		OutRenderTarget = InRenderTarget->GameThread_GetRenderTargetResource();
		check(OutRenderTarget);
	}
	
protected:
	virtual void OnPreRender(FRDGBuilder& GraphBuilder) override
	{
		// 当前这一轮 pass 重新生成资源。
		//bProducedThisFrame = false;

		//FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(GetRenderTargetSize(),OutputFormat, FClearValueBinding::BlackETextureCreateFlags::RenderTargetable|ETextureCreateFlags::ShaderResource);
		//RenderTargetTexture = GraphBuilder.CreateTexture(Desc, *GetDebugName());
		RenderTargetTexture = OutRenderTarget->GetRenderTargetTexture(GraphBuilder);
	}
	
	virtual void OnBeginPass(FRDGBuilder& GraphBuilder) override
	{
		UE_LOG(LogTemp,Warning,TEXT("CRP OnBeginPass: this=%p Name=%s"),this,*GetDebugName());
	}

	/*
	virtual void OnPostRender(FRDGBuilder& GraphBuilder) override
	{
		if (RenderTargetTexture!=nullptr)
		{
			bProducedThisFrame = true;
		}
	}
	*/
public:
	/*
	FRDGTextureRef ConsumeOutput_RenderThread()
	{
		check(IsInRenderingThread());
		if (!bProducedThisFrame)
		{
			return nullptr;
		}
		bProducedThisFrame = false;
		//获取由FCustomRenderPassBase本身提供的一张Rendertarget
		return GetRenderTargetTexture();
	}
	*/
	
};
