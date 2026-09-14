using UnrealBuildTool;

public class HairEyeThrough : ModuleRules
{
    public HairEyeThrough(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Projects",
            "RenderCore",
            "RHI",
            "Renderer"
        });
    }
}
