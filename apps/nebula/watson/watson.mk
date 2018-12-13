
NAME := Nebuwatson_V1

$(NAME)_SOURCES :=  mqtt.c \
					bme280_wiced_wrapper.c \
					watson.c

$(NAME)_COMPONENTS := drivers/sensors/BME280 \
				protocols/MQTT

WIFI_CONFIG_DCT_H := wifi_config_dct.h

$(NAME)_RESOURCES  := apps/secure_mqtt/secure_mqtt_root_cacert.cer
