/**
 * The following code is a sample demonstrating nebula 2.0 functionality with www.scriptr.io as a cloud backend, going through watson bluemix.
 * The full application is described in details in our [blog](https://blog.scriptr.io/<KEY_TO_BLOG>)
 * In summary, this sample will demonstrate
 * 1. connectivity to scriptr.io, over mqtt, through bluemix quickstart
 * a. on-demand and periodic publish of sensor readings
 * b. receiving messages from the cloud (over an mqtt subscription): this is not implemented since bluemix quickstart doesn't allow talking back to a device
 * The code is based on
 * 1. wiced sample code secrure_mqtt
 * 2. bme280 drivers from [cypress community](https://community.cypress.com/docs/DOC-14605)
 */
#include "../bluemix/mqtt.h"
#include "../bme280_test/bme280_test.h"
#include "../bme280_test/bme280_wiced_wrapper.h"
#include "bluemix.h"
#include "wiced.h"
#include "wiced_management.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define MQTT_ALL_EVENTS                     (-1)
/******************************************************
 *                    Constants
 ******************************************************/
#define WICED_MQTT_TIMEOUT                  (5000)

#define WICED_MQTT_DELAY_IN_MILLISECONDS    (1000)

#define MQTT_MAX_RESOURCE_SIZE              (0x7fffffff)
/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum
{
    BUTTON_ALL_EVENTS       = -1,
    BUTTON1_EVENT           = (1 << 0),
    BUTTON2_EVENT           = (1 << 1),
} BUTTON_EVENTS_T;
/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static wiced_event_flags_t button_events;
/**
 * Print the sensor data read from the BME280.
 *
 * @param[in] comp_data : Pointer to the compensated BME280 data to print out
 *
 * @return void
 */
static void print_sensor_data(struct bme280_data *comp_data);
/**
 * format sensor data, returns the sensor readings in a json format
 */
static char * format_sensor_data(struct bme280_data *comp_data);

/******************************************************
 *               Variable Definitions
 ******************************************************/
struct bme280_dev dev_bme280;
struct bme280_data sensor_data;
static wiced_event_flags_t button_events;
static wiced_ip_address_t    broker_address;
static wiced_mqtt_callback_t callbacks = mqtt_connection_event_cb;
static wiced_mqtt_security_t security;
static wiced_mqtt_object_t mqtt_object;
/**
 * event handler for button 1 clicks
 * When the button is clicked, the system will get a reading from the sensor, format it (json) and publish it to bluemix.
 * Led1 will be on while publishing
 * if we're publishing already, nothing will happen
 */
static wiced_bool_t publishing = WICED_FALSE;
static void button_isr_event(void* arg)
{
   int8_t bme_rslt;
   if(publishing == WICED_FALSE)
   {
       wiced_gpio_output_high( WICED_LED1 );
       publishing = WICED_TRUE;
       if((bme_rslt = bme280_get_sensor_data(BME280_ALL, &sensor_data, &dev_bme280)) == BME280_OK){
           WPRINT_APP_INFO(("Normal Mode Measurement: "));
           print_sensor_data(&sensor_data);
           char * formattedMessage;
           formattedMessage = format_sensor_data(&sensor_data);
           WPRINT_APP_INFO(("Topic :%s\n", PUB_TOPIC));
           wiced_result_t ret = mqtt_app_publish( mqtt_object, WICED_MQTT_QOS_DELIVER_AT_MOST_ONCE, PUB_TOPIC, (uint8_t*)formattedMessage ,strlen(formattedMessage));
       }
       else{
           WPRINT_APP_INFO(("Error %d reading BME280 sensor data!\n", bme_rslt));
       }
       wiced_gpio_output_low( WICED_LED1 );
       publishing = WICED_FALSE;
   }
   else
   {
       WPRINT_APP_INFO(("we're here now, and we shouldn't\n"));
   }
}

/**
 * bring network up
 */
void netword_setup()
{
    wiced_result_t        ret = WICED_SUCCESS;
    /* Bring up the network interface */
    ret = wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    if ( ret != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ( "\nNot able to join the requested AP\n\n" ) );
        return;
    }
}

/**
 * generate a random string, terminated by \0
 */
static char *random_string(char *random, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    --size;
    random[0] = '0';
    for (size_t n = 1; n < size; n++) {
        int r = rand();
        int key = r % (int) (sizeof charset - 1);
        random[n] = charset[key];
    }
    random[size] = '\0';
    return random;
}

/**
 * mqtt setup
 * this method will
 * 1. setup the wiced_mqtt_object_t
 * 2. resolve ip of mqtt broker defined in scriptr.h
 * 3. init mqtt client
 * 4. open connection to resolved ip
 * 5. subscribe to topic specified in scriptr.h
 */
void mqtt_setup()
{
    mqtt_object = (wiced_mqtt_object_t) malloc( WICED_MQTT_OBJECT_MEMORY_SIZE_REQUIREMENT );
    wiced_result_t ret = WICED_SUCCESS;
    uint32_t size_out;
    if ( mqtt_object == NULL )
    {
        WPRINT_APP_ERROR(("Dont have memory to allocate for mqtt object...\n"));
        return;
    }

    WPRINT_APP_INFO( ( "Resolving IP address of MQTT broker?!...\n" ) );
    ret = wiced_hostname_lookup( MQTT_BROKER_ADDRESS, &broker_address, 10000, WICED_STA_INTERFACE );
    WPRINT_APP_INFO(("Resolved Broker IP: %u.%u.%u.%u\n\n", (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 24),
            (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 16),
            (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 8),
            (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 0)));
    if ( ret == WICED_ERROR || broker_address.ip.v4 == 0 )
    {
        WPRINT_APP_INFO(("Error in resolving DNS\n"));
        return;
    }

    wiced_mqtt_init( mqtt_object );
    ret = mqtt_conn_open( mqtt_object,&broker_address, WICED_STA_INTERFACE, callbacks, &security, CLIENT_ID);
    if ( ret == WICED_SUCCESS )
    {
            WPRINT_APP_INFO(( "OK.\n\n" ));
    }
    else
    {
            WPRINT_APP_INFO(( "ERROR.\n\n" ));
    }
}

/**
 * struct to hold the randomly generated bluemix client id.
 * This id will be stored in the dct and will be used after each reset.
 */
typedef struct
{
    char     clientId[8];
} app_config_dct_t;

/**
 * main application thread.
 * This will setup all needed parts
 * 1. wiced
 * 2. network
 * 3. mqtt
 * 4. bme
 * 5. generate / fetch clientid
 * 6. register listeners for button clicks
 * 7. run main loop
 */
void application_start( )
{
    wiced_result_t wres;
    int8_t bme_rslt;
    wiced_result_t      result;

    uint8_t settings_sel;
    uint16_t typ_meas_time;
    uint32_t events;

    /* Initialise the WICED device */
    wiced_init();

    WPRINT_APP_INFO(("To view data, connect scriptr.io to the mqtt endpoint defined as:\n"));
    WPRINT_APP_INFO(("Protocol: MQTTS\n"));
    WPRINT_APP_INFO(("URL: %s\n", MQTT_BROKER_ADDRESS));
    WPRINT_APP_INFO(("Port: 8883\n"));
    WPRINT_APP_INFO(("Topic: %s\n", PUB_TOPIC));
    WPRINT_APP_INFO(("ClientId: %s\n", CLIENT_ID));

    /* Initialise network using wifi */
    netword_setup();
    mqtt_setup();
#ifndef BME280_USE_SPI
    wres = bme280_wiced_init_i2c(&dev_bme280, BME280_I2C, BME280_I2C_ADDR_PRIM);
#else
    wres = bme280_wiced_init_spi(&dev_bme280, BME280_SPI, BME280_SPI_CS);
#endif

    if(wres != WICED_SUCCESS){
        WPRINT_APP_INFO( ( "BME280 successfully initialized.\n") );
    }
    else{
        WPRINT_APP_INFO( ( "Error %u while initializing BME280!\n", (unsigned)wres ) );
    }

    /* BME280 datasheet recommended mode of operation: Indoor navigation */
    dev_bme280.settings.osr_h = BME280_OVERSAMPLING_1X;
    dev_bme280.settings.osr_p = BME280_OVERSAMPLING_16X;
    dev_bme280.settings.osr_t = BME280_OVERSAMPLING_2X;
    dev_bme280.settings.filter = BME280_FILTER_COEFF_16;

    settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    if((bme_rslt = bme280_set_sensor_settings(settings_sel, &dev_bme280)) != BME280_OK){
        WPRINT_APP_INFO(("Error %d while configuring BME280!\n", bme_rslt));
    }

    typ_meas_time = bme280_wiced_get_meas_time(&dev_bme280);
    WPRINT_APP_INFO(("Typical measurement time for current settings: %ums\n", (unsigned)typ_meas_time));

    /* One-shot read of temperature, humidity, and pressure */
    if((bme_rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &dev_bme280)) == BME280_OK){
        wiced_rtos_delay_milliseconds(typ_meas_time);
        bme280_get_sensor_data(BME280_ALL, &sensor_data, &dev_bme280);
        WPRINT_APP_INFO(("One-Shot Forced Measurement: "));
        print_sensor_data(&sensor_data);
    }
    else{
        WPRINT_APP_INFO(("Error %d setting BME280 sensor mode!\n", bme_rslt));
    }

    /* Start periodic measurements */
    dev_bme280.settings.standby_time = BME280_STANDBY_TIME_500_MS;
    settings_sel = BME280_STANDBY_SEL;
    if((bme_rslt = bme280_set_sensor_settings(settings_sel, &dev_bme280)) != BME280_OK){
        WPRINT_APP_INFO(("Error %d while configuring BME280!\n", bme_rslt));
    }
    bme280_set_sensor_mode(BME280_NORMAL_MODE, &dev_bme280);
    wiced_rtos_delay_milliseconds(typ_meas_time);
    wiced_gpio_input_irq_enable( WICED_BUTTON1, IRQ_TRIGGER_FALLING_EDGE, button_isr_event, NULL );
    wiced_rtos_init_event_flags(&button_events);
    WPRINT_APP_INFO(("Starting event loop\n"));
    while ( 1 )
    {
        events = 0;
        WPRINT_APP_INFO(("In event loop, waiting for event\n"));

        result = wiced_rtos_wait_for_event_flags(&button_events, BUTTON1_EVENT, &events,
                                                         WICED_TRUE, WAIT_FOR_ANY_EVENT, WICED_WAIT_FOREVER);
    }
}


static void print_sensor_data(struct bme280_data *comp_data)
{
    WPRINT_APP_INFO(("Temperature = %.2f\xf8""C, Humidity = %.2f%%, Pressure = %.2fPa\n", comp_data->temperature, comp_data->humidity, comp_data->pressure));
}
static char out[102];
static char* format_sensor_data(struct bme280_data *comp_data)
{
    sprintf(out, "{\"d\": {\"p\":%.2f,\"h_unit\":\"%%\",\"p_unit\":\"Pa\",\"t\":%.2f,\"h\":%.2f,\"t_unit\":\"C\", \"id\":\"%s\"}}",
            comp_data->pressure, comp_data->temperature, comp_data->humidity, DEVICE_ID);
    return out;
}
