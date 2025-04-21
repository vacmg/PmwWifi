#include <unity.h>
#include <PMW-Wifi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

TEST_CASE("getInstance", "[PMW-Wifi]")
{
    PMW_Wifi* wifi = PMW_Wifi::getInstance();
    TEST_ASSERT_NOT_NULL(wifi);
}
