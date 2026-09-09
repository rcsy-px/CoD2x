#include "downloading.h"

#include "drawing.h"
#include "shared.h"
#include "../shared/cod2_client.h"


namespace {

constexpr uintptr_t COD2_UI_DRAW_MAP_LEVELSHOT_CALL = 0x004D34C6;
constexpr uintptr_t COD2_LOADING_MAP_NAME_ADDRESS = 0x0196FFA0;


void UI_DrawFullscreenMaterial(const char * materialName) {
    materialHandle_t * material = CG_RegisterMaterialNoMip(materialName, MATERIAL_TYPE_UI2);

    if (material != NULL) {
        UI_DrawHandlePic(
            0.0f,
            0.0f,
            640.0f,
            480.0f,
            HORIZONTAL_ALIGN_FULLSCREEN,
            VERTICAL_ALIGN_FULLSCREEN,
            colWhite,
            material
        );
        return;
    }

    UI_DrawHandlePic(
        0.0f,
        0.0f,
        640.0f,
        480.0f,
        HORIZONTAL_ALIGN_FULLSCREEN,
        VERTICAL_ALIGN_FULLSCREEN,
        colBlack,
        shaderWhite
    );
}


/**
 * Draw Reforged art only while connecting/downloading. Once CoD2 publishes a
 * map name, cache it and draw that map's native splash directly. The engine's
 * map-name buffer is transient during cgame startup, so relying on the stock
 * connect menu here causes a missing-shader frame followed by the fallback art.
 */
void UI_DrawConnectionOrMapLoadingScreen() {
    static char loadingMapName[64] = { 0 };

    if (clientState <= CLIENT_STATE_CHALLENGING) {
        loadingMapName[0] = '\0';
    }

    const char * engineMapName = reinterpret_cast<const char *>(COD2_LOADING_MAP_NAME_ADDRESS);
    if (engineMapName[0] != '\0') {
        strncpy(loadingMapName, engineMapName, sizeof(loadingMapName) - 1);
        loadingMapName[sizeof(loadingMapName) - 1] = '\0';
    }

    if (loadingMapName[0] != '\0' && clientState >= CLIENT_STATE_CONNECTED) {
        char mapMaterialName[96];
        snprintf(mapMaterialName, sizeof(mapMaterialName), "loadscreen_%s", loadingMapName);
        UI_DrawFullscreenMaterial(mapMaterialName);
        return;
    }

    materialHandle_t * connectionBackground = CG_RegisterMaterialNoMip("rfg_conn", MATERIAL_TYPE_UI2);

    if (connectionBackground != NULL) {
        UI_DrawHandlePic(
            0.0f,
            0.0f,
            640.0f,
            480.0f,
            HORIZONTAL_ALIGN_FULLSCREEN,
            VERTICAL_ALIGN_FULLSCREEN,
            colWhite,
            connectionBackground
        );
        return;
    }

    // Match the stock black fallback if the optional Reforged asset is absent.
    UI_DrawHandlePic(
        0.0f,
        0.0f,
        640.0f,
        480.0f,
        HORIZONTAL_ALIGN_FULLSCREEN,
        VERTICAL_ALIGN_FULLSCREEN,
        colBlack,
        shaderWhite
    );
}

} // namespace


int EventListTimerHandler (void * timer, void * param) {

    // CoD2x: Fix crash at 0x0054e658 when trying to access param->timeouts
    if (param == NULL || timer == NULL || IsBadReadPtr(param, 0x20)) {
        Com_Printf("EventListTimerHandler: preventing crash\n");
        return -1; // HT_ERROR
    }

    int ret;
    ASM_CALL(RETURN(ret), 0x0054e650, 2, PUSH(timer), PUSH(param));

    return ret;
}


/** Called before the entry point to patch memory for downloading features. */
void downloading_patch() {
    patch_int32(0x0054e90c + 1, (unsigned int)EventListTimerHandler); // push EventListTimerHandler

    // Own the background selection so connection art can never replace a map's
    // native splash during cgame initialization.
    patch_call(COD2_UI_DRAW_MAP_LEVELSHOT_CALL, (unsigned int)UI_DrawConnectionOrMapLoadingScreen);

    // Downloading screen
    patch_float(0x00539351 + 1, 0.35f); // font size (0.5f original)
    patch_float(0x005c4390, 210.0f);    // Y1 (210.0f original)
    patch_float(0x005c438c, 230.0f);    // Y2 (235.0f original)
    patch_float(0x005c4388, 250.0f);    // Y3 (260.0f original)
}
