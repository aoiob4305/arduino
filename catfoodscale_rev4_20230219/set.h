#ifndef STASSID
  #define STASSID "dslee"
  #define STAPSK  "0195495416"
#endif

#define AIO_SERVER      "kimyoungwook.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    ""
#define AIO_KEY         ""

#define SCALE_1ST_TOPIC "/catfoodscale/weight1"
#define SCALE_1ST_DATAPIN D1
#define SCALE_1ST_CLKPIN D2
#define SCALE_1ST_GAIN    128
#define SCALE_1ST_SCALE   2276.38
#define SCALE_1ST_OFFSET  -62585
#define SCALE_1ST_WEIGHT_TOL  1.0   //측정한 무게를 반영할지 오차 범위

#define SCALE_2ND_TOPIC "/catfoodscale/weight2"
#define SCALE_2ND_DATAPIN D5
#define SCALE_2ND_CLKPIN D6
#define SCALE_2ND_GAIN    1
#define SCALE_2ND_SCALE   -2276.38
#define SCALE_2ND_OFFSET  0
#define SCALE_2ND_WEIGHT_TOL  1.0   //측정한 무게를 반영할지 오차 범위

#define WATCHDOG_TOPIC "/catfoodscale/watchdog"

#define INTERVAL    60000
