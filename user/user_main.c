#include "ets_sys.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "config.h"

os_event_t    user_procTaskQueue[user_procTaskQueueLen];

static void user_procTask(os_event_t *events);

static volatile os_timer_t sensor_timer;

void sensor_timerfunc(void *arg)
{
    uint8 ack;
    uint8 i;    
    uint8 pData[4];
    uint8 address;
    uint8 real_address;

    //Read data
    uart0_sendStr("sensor pull: ");
    i2c_master_stop(); //Stop i2c
    i2c_master_start(); //Start i2c
    i2c_master_writeByte(0x40 << 1); //write address 0x40
    ack = i2c_master_getAck(); // Get ack from slave
    if (ack) {
        os_printf("addr not ack when tx write cmd \n");
        i2c_master_stop();
        return;
    } else uart0_sendStr("ack ");
    os_delay_us(10);
    i2c_master_writeByte(0xf3);
    ack = i2c_master_getAck();
    if (!ack) uart0_sendStr("ack ");

    os_delay_us(100);
    ack = 1;
    while (ack) {
        i2c_master_start();
        address = 0x40 << 1;
        address |= 1;
        i2c_master_writeByte(address);
        ack = i2c_master_getAck();
    }
    uart0_sendStr("ack ");

    uint16_t t = i2c_master_readByte();
    i2c_master_setAck(0); 
    t <<= 8;
    t |= i2c_master_readByte();
    i2c_master_setAck(0);

    uint8_t crc = i2c_master_readByte();
    i2c_master_setAck(1);;
    i2c_master_stop();

    float temp = t;
    temp *= 175.72;
    temp /= 65536;
    temp -= 46.85;

    os_printf("%d",(uint16) temp);
    uart0_sendStr(" Done\r\n");
 
}

static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
    os_delay_us(5000);
}

void user_init(void)
{
    char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;

    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 32);

    //Init uart
    uart_init(BIT_RATE_115200, BIT_RATE_115200);

    //Setup wifi
    wifi_set_opmode( 0x1 );
    wifi_station_set_config(&stationConf);

    i2c_master_gpio_init();

    uart0_sendStr("Booting\r\n");

    //Disarm timer
    os_timer_disarm(&sensor_timer);

    //Setup timer
    os_timer_setfn(&sensor_timer, (os_timer_func_t *)sensor_timerfunc, NULL);

    //Arm timer for every 10 sec.
    os_timer_arm(&sensor_timer, 5000, 1);

    system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
}

