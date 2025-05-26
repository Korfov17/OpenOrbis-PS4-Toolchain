#include <sstream>
#include <iostream>
#include <orbis/libkernel.h>
#include <orbis/UserService.h>

#include "../../_common/graphics.h"
#include "../../_common/log.h"

#define FRAME_WIDTH   1920
#define FRAME_HEIGHT  1080
#define FRAME_DEPTH   4
#define FONT_SIZE     42

Color bgColor = { 0, 0, 0 };
Color fgColor = { 255, 255, 255 };
FT_Face fontTxt;
int frameID = 0;

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);

    auto scene = new Scene2D(FRAME_WIDTH, FRAME_HEIGHT, FRAME_DEPTH);
    if (!scene->Init(0xC000000, 2)) {
        for (;;);
    }

    const char* font = "/app0/assets/fonts/Gontserrat-Regular.ttf";
    if (!scene->InitFont(&fontTxt, font, FONT_SIZE)) {
        for (;;);
    }

    bool showUserInfo = false;
    int userID;
    char username[32] = {0};

    // Inicializar UserService una sola vez
    OrbisUserServiceInitializeParams param;
    param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    sceUserServiceInitialize(&param);
    sceUserServiceGetInitialUser(&userID);
    sceUserServiceGetUserName(userID, username, sizeof(username) - 1);

    for (;;) {
        // Dibujar menú
        if (!showUserInfo) {
            scene->DrawRect(0, 0, FRAME_WIDTH, FRAME_HEIGHT, bgColor);
            scene->DrawText((char*)"Pulsa cualquier botón para mostrar usuario", fontTxt, 150, 150, bgColor, fgColor);

            // Aquí puedes meter alguna condición para activar el botón real, por ahora lo activamos automáticamente tras unos frames
            if (frameID > 60) showUserInfo = true;
        } else {
            std::stringstream ss;
            ss << "Logged into: " << username << " (ID: 0x" << std::hex << userID << ")";
            scene->DrawRect(0, 0, FRAME_WIDTH, FRAME_HEIGHT, bgColor);
            scene->DrawText((char*)ss.str().c_str(), fontTxt, 150, 150, bgColor, fgColor);
        }

        scene->SubmitFlip(frameID);
        scene->FrameWait(frameID);
        scene->FrameBufferSwap();
        frameID++;
    }

    return 0;
}
