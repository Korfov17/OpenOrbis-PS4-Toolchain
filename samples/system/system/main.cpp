#include <sstream>
#include <iostream>
#include <orbis/libkernel.h>
#include <orbis/UserService.h>
#include <orbis/Pad.h>

#include "../../_common/graphics.h"
#include "../../_common/log.h"

// Dimensions
#define FRAME_WIDTH     1920
#define FRAME_HEIGHT    1080
#define FRAME_DEPTH        4
#define FONT_SIZE   	   42

// Logging
std::stringstream debugLogStream;

// Colors and font
Color bgColor = { 0, 0, 0 };
Color fgColor = { 255, 255, 255 };
FT_Face fontTxt;

int frameID = 0;

int main()
{
    int rc;
    int video;
    int curFrame = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    
    auto scene = new Scene2D(FRAME_WIDTH, FRAME_HEIGHT, FRAME_DEPTH);
    if (!scene->Init(0xC000000, 2)) {
        DEBUGLOG << "Failed to initialize 2D scene";
        for (;;);
    }

    const char *font = "/app0/assets/fonts/Gontserrat-Regular.ttf";
    if (!scene->InitFont(&fontTxt, font, FONT_SIZE)) {
        DEBUGLOG << "Failed to initialize font '" << font << "'";
        for (;;);
    }

    // Inicializa el pad
    scePadInit();

    bool showUserInfo = false;
    char username[32] = {0};
    int userID = -1;

    DEBUGLOG << "Esperando entrada para mostrar info de usuario...";

    for (;;)
    {
        scene->Clear(bgColor);

        if (!showUserInfo)
        {
            scene->DrawText((char *)"Presiona X para mostrar usuario logueado", fontTxt, 150, 150, bgColor, fgColor);
            
            // Leer el estado del pad
            OrbisPadData pad;
            scePadReadState(0, &pad);

            if (pad.buttons & ORBIS_PAD_BUTTON_CROSS)
            {
                // Obtener nombre de usuario
                OrbisUserServiceInitializeParams param;
                param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
                sceUserServiceInitialize(&param);
                sceUserServiceGetInitialUser(&userID);

                if (sceUserServiceGetUserName(userID, username, sizeof(username) - 1) < 0)
                {
                    snprintf(username, sizeof(username), "Desconocido");
                }

                showUserInfo = true;
            }
        }
        else
        {
            std::stringstream ss;
            ss << "Iniciado sesión como: " << username << " (ID: 0x" << std::hex << userID << ")";
            scene->DrawText((char *)ss.str().c_str(), fontTxt, 150, 150, bgColor, fgColor);
        }

        scene->SubmitFlip(frameID);
        scene->FrameWait(frameID);
        scene->FrameBufferSwap();
        frameID++;
    }

    return 0;
}
