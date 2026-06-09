#pragma once
// =============================================================================
// Display Driver Abstraction
// Selects TFT_eSPI or LovyanGFX based on build flags.
// Both libraries share a compatible API for drawing operations.
// =============================================================================

#ifdef USE_LOVYANGFX
    #ifdef LGFX_8048S070
        #include "lgfx_config_8048S070.h"
    #else
        #include "lgfx_config_4827S043.h"
    #endif
    typedef LGFX DisplayDriver;
#else
    #include <TFT_eSPI.h>
    typedef TFT_eSPI DisplayDriver;
#endif

