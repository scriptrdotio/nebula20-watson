# nebula20-bluemix
This module is a firmware for [Nebula 2.0](https://www.futureelectronics.com/p/development-tools--development-tool-hardware/neb1dx-02-future-electronics-dev-tools-5094171)
In order to run this you will need to 
1. Install [Wiced Studio](http://www.cypress.com/products/wiced-software) by [Cypress](http://www.cypress.com)
2. Clone this repository using
```bash
git clone https://github.com/scriptrdotio/nebula20-bluemix.io.git
```
3. Copy cloned project into your workspace
4. Configure your wifi connection under apps/nebula/bluemix/wifi_config_dct.h by setting CLIENT_AP_SSID and CLIENT_AP_PASSPHRASE
5. Generate a new build target by clicking on "new" under "Make Target" and naming it "nebula.scriptr-NEB1DX_02 download download_apps run"
6. Run the build

Note: In order to run this demo you will need to enable MQTT Bridges in scriptr.io, available as a free trial from inside the IDE.

The code is built using the following
. click board BME 280 from [click board samples](https://community.cypress.com/docs/DOC-14605) (drivers and wrappers)
. mqtt code from secure_mqtt, sample code provided by default under apps/snip

## Project Files
1. mqtt.c and mqtt.h, code for connecting over mqtt to the scriptr.io mqtt broker
2. wifi_config_dct.h for wifi configuration (SSID and PASSPHRASE)
3. bluemix.c, code for connecting to bme280 sensor, button events, connectivity to bluemix
4. bluemix.h, config file for bluemix connectivity (mqtt endpoint, topic,...)

# Configuration
## Scriptr.io configuration, part 1
1. If you don't have a scriptr.io account, you will need to [register](https://www.scriptr.io/register) for a free one
2. Create a default script, name it "echo" with content ``` return request ``` This script will simply return (and log) the request as it received it
3. Create a channel named "nebula-demo"
4. Create a device named "nebula20"
5. Go External Endpoints (Under account / Settings) and enable bridges feature.
Note, this configuration can be done before setting up the board. Continue with part 2 once the firmware is install and the board is booted

## WIFI configuration 
1. set CLIENT_AP_SSID to your wifi network name 
2. set CLIENT_AP_PASSPHRASE to your wifi password

For a more complete demo, please refer to our corresponding [blog post](https://blog.scriptr.io/to-be-set)

# Runtime
Once the firmware is installed, the main method (application_start) will be started.
The system will 
1. initialize the network
2. initialize mqtt for tls
3. connect to bluemix mqtt endpoint, using [quickstart](https://quickstart.internetofthings.ibmcloud.com) mode
4. initialize BME sensor module (and do one initial forced reading, as per sample code provided by FE)
5. register an event handler for button clicks
6. read dct for previous device configuration, generate new device id on first run.
7. button 1 click will publish the sensor data to bluemix using the generated device id
Note, sensor readings are formatted as JSON ```{"parameters": {"p": 94465.42,"h_unit": "%","p_unit": "Pa","t": 28.41,"h": 52.71,"t_unit": "°C"}}```
For more details about bridges module in scriptr.io please refer to the [documentation](https://www.scriptr.io/documentation#documentation-bridges)

## Scriptr.io configuration, part 2
Once the board is started, it will print out the needed information about the quickstart account, by providing the generated device id.
In order to process the data pushed to bluemix in scriptr.io, you will need to start a new mqtt bridge, link it to channel "nebula-demo" and subscribe it to your "echo" script.
Configuration of the bridge will be printed out on the terminal in the following format:
```
Protocol: MQTTS
URL: quickstart.messaging.internetofthings.ibmcloud.com
Port: 8883
Topic: iot-2/type/scriptr-demo/id/0rzkzdj/evt/scriptr-demo/fmt/json
ClientId: a:quickstart:scriptr-demo
```
