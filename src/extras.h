#ifndef _extras_h_
#define _extras_h_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <math.h>

#define PI 3.1415926535897932384626433832795
#define RAD_TO_DEG 57.295779513082320876798154814105

#define min(a,b) ((a)<(b)?(a):(b))
#define VALUE_WITHIN(v,l,h) (((v)>=(l)) && ((v)<=(h)))

#endif