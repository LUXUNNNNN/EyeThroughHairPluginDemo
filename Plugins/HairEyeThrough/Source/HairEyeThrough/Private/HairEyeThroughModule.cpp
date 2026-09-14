#include "HairEyeThroughModule.h"

#include "EyeThroughHairViewExtension.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "SceneViewExtension.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FHairEyeThroughModule"

void FHairEyeThroughModule::StartupModule()
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("HairEyeThrough"));
    checkf(Plugin.IsValid(), TEXT("HairEyeThrough plugin descriptor was not found."));

    const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/HairEyeThrough"), ShaderDirectory);

    if (GEngine)
    {
        CreateViewExtension();
    }
    else
    {
        PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(
            this,
            &FHairEyeThroughModule::CreateViewExtension);
    }
}

void FHairEyeThroughModule::CreateViewExtension()
{
    if (!ViewExtension.IsValid() && !IsRunningCommandlet())
    {
        ViewExtension = FSceneViewExtensions::NewExtension<FEyeThroughHairViewExtension>();
    }
}

void FHairEyeThroughModule::ShutdownModule()
{
    if (PostEngineInitHandle.IsValid())
    {
        FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
        PostEngineInitHandle.Reset();
    }

    ViewExtension.Reset();
}

void FHairEyeThroughModule::SetSveActive(bool bIsActive)
{
    if (ViewExtension.IsValid())
    {
        ViewExtension->bIsActive = bIsActive;
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHairEyeThroughModule, HairEyeThrough)
