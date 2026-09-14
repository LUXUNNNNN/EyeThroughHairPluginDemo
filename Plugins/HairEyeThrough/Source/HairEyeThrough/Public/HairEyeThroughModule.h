#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FEyeThroughHairViewExtension;

class HAIREYETHROUGH_API FHairEyeThroughModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    void SetSveActive(bool bIsActive);
    
    TSharedPtr<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> GetViewExtension() const
    {
        return ViewExtension;
    }

private:
    void CreateViewExtension();
    
    FDelegateHandle PostEngineInitHandle;
    TSharedPtr<FEyeThroughHairViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
