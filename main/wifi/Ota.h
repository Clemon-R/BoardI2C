#ifndef OTA_H_
#define OTA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_https_ota.h>
#include <esp_ota_ops.h>
#include <esp_log.h>

#define OTA_BUF_SIZE 256
#define BIN_URL	"http://testbin.alwaysdata.net/board-%s.bin" //%s needed and replaced by current version X.X


esp_err_t	launchUpdate(char *version);

#endif