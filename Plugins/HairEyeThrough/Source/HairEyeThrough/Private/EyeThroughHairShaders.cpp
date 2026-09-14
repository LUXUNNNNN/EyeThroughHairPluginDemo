#include "EyeThroughHairShaders.h"

IMPLEMENT_GLOBAL_SHADER(
    FEyeThroughHairPS,
    "/HairEyeThrough/Private/EyeThroughHairComposite.usf",
    "MainPS",
    SF_Pixel);
