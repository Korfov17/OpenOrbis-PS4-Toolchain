#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <orbis/CommonDialog.h>
#include <orbis/MsgDialog.h>
#include <orbis/Sysmodule.h>
#include <orbis/Http.h>
#include <orbis/Ssl.h>
#include <orbis/Net.h>

// Dialog types
#define MDIALOG_OK       0
#define MDIALOG_YESNO    1

// HTTP constants
#define HTTP_SUCCESS 	1
#define HTTP_FAILED	 	0
#define HTTP_USER_AGENT "Mozilla/5.0 (PLAYSTATION 4; 1.00)"
#define NET_POOLSIZE 	(4 * 1024)

static int libnetMemId = 0, libhttpCtxId = 0, libsslCtxId = 0;

// Función para inicializar diálogos
static inline void _orbisCommonDialogSetMagicNumber(uint32_t* magic, const OrbisCommonDialogBaseParam* param)
{
    *magic = (uint32_t)(ORBIS_COMMON_DIALOG_MAGIC_NUMBER + (uint64_t)param);
}

static inline void _orbisCommonDialogBaseParamInit(OrbisCommonDialogBaseParam *param)
{
    memset(param, 0x0, sizeof(OrbisCommonDialogBaseParam));
    param->size = (uint32_t)sizeof(OrbisCommonDialogBaseParam);
    _orbisCommonDialogSetMagicNumber(&(param->magic), param);
}

static inline void orbisMsgDialogParamInitialize(OrbisMsgDialogParam *param)
{
    memset(param, 0x0, sizeof(OrbisMsgDialogParam));
    _orbisCommonDialogBaseParamInit(&param->baseParam);
    param->size = sizeof(OrbisMsgDialogParam);
}

int show_dialog(int dialog_type, const char * format, ...)
{
    OrbisMsgDialogParam param;
    OrbisMsgDialogUserMessageParam userMsgParam;
    OrbisMsgDialogResult result;

    char str[0x800];
    memset(str, 0, sizeof(str));

    va_list opt;
    va_start(opt, format);
    vsprintf(str, format, opt);
    va_end(opt);

    sceMsgDialogInitialize();
    orbisMsgDialogParamInitialize(&param);
    param.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

    memset(&userMsgParam, 0, sizeof(userMsgParam));
    userMsgParam.msg = str;
    userMsgParam.buttonType = (dialog_type ? ORBIS_MSG_DIALOG_BUTTON_TYPE_YESNO_FOCUS_NO : ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
    param.userMsgParam = &userMsgParam;

    if (sceMsgDialogOpen(&param) < 0)
        return 0;

    do { } while (sceMsgDialogUpdateStatus() != ORBIS_COMMON_DIALOG_STATUS_FINISHED);
    sceMsgDialogClose();

    memset(&result, 0, sizeof(result));
    sceMsgDialogGetResult(&result);
    sceMsgDialogTerminate();

    return (result.buttonId == ORBIS_MSG_DIALOG_BUTTON_ID_YES);
}

// Función para inicializar HTTP
static int skipSslCallback(int libsslId, unsigned int verifyErr, void * const sslCert[], int certNum, void *userArg)
{
    printf("sslCtx=%x (%X)", libsslId, verifyErr);
    return HTTP_SUCCESS;
}

int http_init()
{
    int ret;

    if(sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) < 0)
        return HTTP_FAILED;

    if(sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP) < 0)
        return HTTP_FAILED;

    if(sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL) < 0)
        return HTTP_FAILED;

    if (libnetMemId == 0 || libhttpCtxId == 0)
    {
        printf("sceNet init");
        ret = sceNetInit();
        ret = sceNetPoolCreate("netPool", NET_POOLSIZE, 0);
        if (ret < 0) {
            printf("sceNetPoolCreate() error: 0x%08X\n", ret);
            return HTTP_FAILED;
        }
        libnetMemId = ret;

        printf("sceSsl init");
        ret = sceSslInit(SSL_POOLSIZE);
        if (ret < 0) {
            printf("sceSslInit() error: 0x%08X\n", ret);
            return HTTP_FAILED;
        }
        libsslCtxId = ret;

        printf("sceHttp init");
        ret = sceHttpInit(libnetMemId, libsslCtxId, LIBHTTP_POOLSIZE);
        if (ret < 0) {
            printf("sceHttpInit() error: 0x%08X\n", ret);
            return HTTP_FAILED;
        }
        libhttpCtxId = ret;
    }

    return HTTP_SUCCESS;
}

int http_download(const char* full_url, const char* local_dst, int show_progress)
{
    int ret, tpl = 0, conn = 0, req = 0;
    int http_res = HTTP_FAILED;
    int contentLengthType;
    uint64_t contentLength;
    int32_t statusCode;

    tpl = sceHttpCreateTemplate(libhttpCtxId, HTTP_USER_AGENT, ORBIS_HTTP_VERSION_1_1, 1);
    if (tpl < 0) {
        printf("sceHttpCreateConnectionWithURL() error: 0x%08X\n", tpl);
        return HTTP_FAILED;
    }

    ret = sceHttpsSetSslCallback(tpl, skipSslCallback, NULL);
    if (ret < 0) {
        printf("sceHttpsSetSslCallback() error: 0x%08X\n", ret);
    }

    conn = sceHttpCreateConnectionWithURL(tpl, full_url, 1);
    if (conn < 0) {
        printf("sceHttpCreateConnectionWithURL() error: 0x%08X\n", conn);
        goto close_http;
    }

    req = sceHttpCreateRequestWithURL(conn, ORBIS_METHOD_GET, full_url, 0);
    if (req < 0) {
        printf("sceHttpCreateRequestWithURL() error: 0x%08X\n", req);
        goto close_http;
    }

    printf("Sending Request to '%s'\n", full_url);
    ret = sceHttpSendRequest(req, NULL, 0);
    if (ret < 0) {
        printf("sceHttpCreateRequestWithURL (%X)", ret);
        goto close_http;
    }

    ret = sceHttpGetStatusCode(req, &statusCode);
    if (ret < 0) {
        printf("sceHttpGetStatusCode (%X)", ret);
        goto close_http;
    }

    ret = sceHttpGetResponseContentLength(req, &contentLengthType, &contentLength);
    if (ret < 0) {
        printf("sceHttpGetContentLength() error: 0x%08X\n", ret);
        //goto close_http;
    }
    else if (contentLengthType == ORBIS_HTTP_CONTENTLEN_EXIST) {
        printf("Content-Length = %lu\n", contentLength);
    }

    if (statusCode != 200)
    {
        if (statusCode == 404)
            printf("404 Download Not Found");

        else if (statusCode == 408)
            printf("Request Timed Out - Check your Connection");

        goto close_http;
    }

    uint8_t dl_buf[64 * 1024];
    uint64_t total_read = 0;
    FILE* fd = fopen(local_dst, "wb");

    if (!fd) {
        printf("fopen Error: File path '%s'", local_dst);
        goto close_http;
    }

    if (show_progress)
        printf("Downloading...");

    while (1) {
        int read = sceHttpReadData(req, dl_buf, sizeof(dl_buf));
        if (read < 0)
            break;

        if (read == 0)
        {
            http_res = HTTP_SUCCESS;
            break;
        }

        ret = fwrite(dl_buf, 1, read, fd);
        if (ret < 0 || ret != read)
            break;

        total_read += read;

        if (show_progress)
            printf("Downloaded %ld/%ld\n", total_read, contentLength);
    }

    fclose(fd);

    if (show_progress)
        printf("Download completed.");

close_http:
    if (req > 0) {
        ret = sceHttpDeleteRequest(req);
        if (ret < 0) {
            printf("sceHttpDeleteRequest() error: 0x%08X\n", ret);
        }
    }

    if (conn > 0) {
        ret = sceHttpDeleteConnection(conn);
        if (ret < 0) {
            printf("sceHttpDeleteConnection() error: 0x%08X\n", ret);
        }
    }

    if (tpl > 0) {
        ret = sceHttpDeleteTemplate(tpl);
        if (ret < 0) {
            printf("sceHttpDeleteTemplate() error: 0x%08X\n", ret);
        }
    }

    return (http_res);
}

void http_end(void)
{
    sceHttpTerm(libhttpCtxId);
    sceSslTerm(libsslCtxId);
    sceNetPoolDestroy(libnetMemId);
}

int main()
{
    // Cargar módulos de diálogo
    if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG) < 0 ||
        sceCommonDialogInitialize() < 0)
    {
        printf("Failed to initialize CommonDialog\n");
        for(;;);
    }

    // Mostrar diálogo de confirmación
    if (show_dialog(MDIALOG_YESNO, "¿Desea descargar el archivo de ejemplo?"))
    {
        show_dialog(MDIALOG_OK, "Iniciando descarga...");
        
        // Inicializar y realizar la descarga HTTP
        if (http_init() == HTTP_SUCCESS) 
        {
            int result = http_download("https://www.google.com/robots.txt", "/data/net_http.txt", 1);
            
            if (result == HTTP_SUCCESS) 
            {
                show_dialog(MDIALOG_OK, "Descarga completada con éxito.");
            } 
            else 
            {
                show_dialog(MDIALOG_OK, "Error en la descarga.");
            }
            
            http_end();
        } 
        else 
        {
            show_dialog(MDIALOG_OK, "Error al inicializar HTTP.");
        }
    }
    else
    {
        show_dialog(MDIALOG_OK, "Descarga cancelada.");
    }

    for(;;);
}
