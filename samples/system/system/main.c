#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <orbis/libkernel.h>
#include <orbis/UserService.h>

#include "../../_common/graphics.h"
#include "../../_common/log.h"

#define FRAME_WIDTH     1920
#define FRAME_HEIGHT    1080
#define FRAME_DEPTH     4
#define FONT_SIZE       42

typedef struct {
    unsigned char r, g, b;
} Color;

Color bgColor;
Color fgColor;
FT_Face fontTxt;

int frameID = 0;

int main(void)
{
    int rc;
    int userID;
    char username[32];
    char userText[128];

    setvbuf(stdout, NULL, _IONBF, 0);

    DEBUGLOG("Creating a scene");

    Scene2D* scene = Scene2D_Create(FRAME_WIDTH, FRAME_HEIGHT, FRAME_DEPTH);
    if (!Scene2D_Init(scene, 0xC000000, 2)) {
        DEBUGLOG("Failed to initialize 2D scene");
        for(;;);
    }

    bgColor = (Color){0, 0, 0};
    fgColor = (Color){255, 255, 255};

    const char *font = "/app0/assets/fonts/Gontserrat-Regular.ttf";
    DEBUGLOG("Initializing font");

    if (!Scene2D_InitFont(scene, &fontTxt, font, FONT_SIZE)) {
        DEBUGLOG("Failed to initialize font");
        for(;;);
    }

    OrbisUserServiceInitializeParams param;
    param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;

    sceUserServiceInitialize(&param);
    sceUserServiceGetInitialUser(&userID);
    memset(username, 0, sizeof(username));

    if (sceUserServiceGetUserName(userID, username, sizeof(username) - 1) < 0) {
        DEBUGLOG("Failed to get username!");
        return -1;
    }

    snprintf(userText, sizeof(userText),
             "Tu cuenta es: %s (ID: 0x%X)", username, userID);

    DEBUGLOG("Entering draw loop...");

    while (1) {
        Scene2D_DrawText(scene, userText, fontTxt, 150, 150, bgColor, fgColor);
        Scene2D_SubmitFlip(scene, frameID);
        Scene2D_FrameWait(scene, frameID);
        Scene2D_FrameBufferSwap(scene);
        frameID++;
    }

    return 0;
}
