# HairEyeThrough — UE 5.4 prototype

This is a **source skeleton**, not a binary plugin. It implements:

1. `SceneCaptureComponent2D` Eye BaseColor
2. `SceneCaptureComponent2D` Eye SceneDepth
3. `SceneCaptureComponent2D` Front Hair SceneDepth
4. A `SceneViewExtension` callback at `EPostProcessingPass::BeforeDOF`
5. External RHI texture registration into RDG
6. A full-screen global pixel shader that composites the eye only when:
   - hair is in front of the eye;
   - eye/hair depth gap is below a thickness limit;
   - main SceneDepth matches the isolated hair depth.

## Install

Copy the `HairEyeThrough_UE54` directory into:

```
YourProject/Plugins/HairEyeThrough
```

The folder name must be `HairEyeThrough`, matching the plugin lookup in `HairEyeThroughModule.cpp`.
Regenerate project files and compile the Editor target.

## Character setup

The character should have separate primitive components:

- `BodyMesh`: normal character
- `FrontHairMesh`: only the fringe, or a masked depth proxy
- `EyeOverlayMesh`: only iris/pupil/lashes that may penetrate the fringe

The overlay mesh should follow the main skeletal pose. For a skeletal proxy, use the same skeleton and set its leader pose component in your character code or Blueprint.

Add `EyeThroughHairCaptureComponent` to the character and assign:

- `EyeProxyComponent`
- `FrontHairComponent`

The component automatically marks the eye proxy as `Visible In Scene Capture Only`.

## Materials

### Eye overlay material

For the UE 5.4 conservative path, EyeColor uses `SCS_BaseColor`:

- Opaque or Masked
- Put the desired overlay RGB in Base Color
- Use Opacity Mask for iris/lash cutouts
- Avoid depending on lighting in the overlay pass

### Front hair

Masked hair works directly. If the visible hair is Translucent, create a separate Masked fringe depth proxy and assign that proxy as `FrontHairComponent`.

## First-run checks

1. Set `r.CompositionGraphDebug 1` and confirm `EyeThroughHair Composite` appears.
2. Use RenderDoc / PIX and verify the three capture textures are populated before the composite pass.
3. Temporarily return the masks from the shader to inspect:
   - `EyeValid`
   - `HairValid`
   - `HairIsMainFrontSurface`
   - final `Mask`
4. Start with:
   - MinDepthGapCm = 0.05
   - MaxHairThicknessCm = 6
   - FrontDepthToleranceCm = 0.5
   - EdgeFeatherCm = 0.25

## Important limitations of this starter

- It supports one active `EyeThroughHairCaptureComponent`; the last component to push resources wins. For multiple characters, build one manager that renders all eligible eye/hair proxies into shared targets, or maintain per-view/per-character layers.
- It assumes one principal game view. Split-screen, stereo and multi-window need per-view resources.
- It assumes auxiliary RT UVs line up 1:1 with the current view. Test editor viewports, TSR and dynamic resolution carefully.
- The code has not been compiled in this environment because Unreal Engine 5.4 headers and libraries are not installed here. Minor include/signature corrections may be necessary for your exact 5.4 patch.
- If `FSceneView::bIsSceneCapture` is unavailable in your exact patch, replace that guard with your project's scene-capture/view-family filter.
- If `Inputs.GetInput(...)` returns `FScreenPassTextureSlice` in your branch, use the 2D texture conversion/helper available in that branch, or `Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder)`.

## Better production architecture

After the prototype works, replace three persistent `UTextureRenderTarget2D`s with custom render pass outputs owned/extracted by RDG, or pack EyeColor and EyeDepth into one render pass/output. Also add:

- per-view resource keys;
- side-view/head-facing rejection;
- stable resolution/view-rect transforms;
- selective activation only while a relevant character is visible;
- temporal stabilization if the fringe edge flickers.
