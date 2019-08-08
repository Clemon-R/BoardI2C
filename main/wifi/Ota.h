#ifndef OTA_H_
#define OTA_H_

#include "../../demo_conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_https_ota.h>
#include <esp_ota_ops.h>
#include <esp_log.h>


esp_err_t	launchUpdate(char *version);

#endif