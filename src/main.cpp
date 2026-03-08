// Signal K application template file.
//
// This application demonstrates core SensESP concepts in a very
// concise manner. You can build and upload the application as is
// and observe the value changes on the serial port monitor.
//
// You can use this source file as a basis for your own projects.
// Remove the parts that are not relevant to you, and add your own code
// for external hardware libraries.

#include <memory>

#include <Adafruit_BME280.h>
#include <Wire.h>

#include "sensesp.h"
#include "sensesp/sensors/analog_input.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp_app_builder.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/ui/config_item.h"
#include "sensesp_onewire/onewire_temperature.h"

using namespace reactesp;
using namespace sensesp;
using namespace sensesp::onewire;

Adafruit_BME280 bme280;

// The setup function performs one-time application initialization.
void setup() {
  SetupLogging(ESP_LOG_DEBUG);
  ESP_LOGI("main", "Starting Signal K application template");
#if defined(CORE_DEBUG_LEVEL) && defined(ARDUHAL_LOG_LEVEL_DEBUG) && \
    (CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG)
  ESP_LOGI("main", "This program was compiled on: %s at %s", __DATE__, __TIME__);
#endif

  // Construct the global SensESPApp() object
  SensESPAppBuilder builder;
  sensesp_app = (&builder)
                    // Set a custom hostname for the app.
                    ->set_hostname("sensoresp")
                    // Optionally, hard-code the WiFi and Signal K server
                    // settings. This is normally not needed.
                    // ->set_wifi_client("signalkpi", "signalkpi")
                    //->set_wifi_access_point("My AP SSID", "my_ap_password")
                    // ->set_sk_server("192.168.4.0", 80)
                    ->get_app();


  uint8_t pin = 4;
  DallasTemperatureSensors* dts = new DallasTemperatureSensors(pin);
  // Define how often SensESP should read the sensor(s) in milliseconds
  uint read_delay = 500;

  // Measure exhaust temperature
  auto* exhaust_temp =
      new OneWireTemperature(dts, read_delay, "/exhaustTemperature/oneWire");
  ConfigItem(exhaust_temp)
    ->set_title("Exhaust Temperature")
    ->set_description("Temperature of the engine exhaust")
    ->set_sort_order(100);

  auto* exhaust_temp_calibration =
      new Linear(1.0, 0.0, "/exhaustTemperature/linear");
  ConfigItem(exhaust_temp_calibration)
    ->set_title("Exhaust Temperature Calibration")
    ->set_description("Calibration for the exhaust temperature sensor")
    ->set_sort_order(200);

  auto* exhaust_temp_sk_output = new SKOutputFloat(
      "propulsion.mainEngine.exhaustTemperature", "/exhaustTemperature/skPath");

  ConfigItem(exhaust_temp_sk_output)
      ->set_title("Exhaust Temperature Signal K Path")
      ->set_description("Signal K path for the exhaust temperature")
      ->set_sort_order(300);

  exhaust_temp->connect_to(exhaust_temp_calibration)
  ->connect_to(exhaust_temp_sk_output);

  // Measure coolant temperature
  auto coolant_temp =
      new OneWireTemperature(dts, read_delay, "/coolantTemperature/oneWire");
  ConfigItem(coolant_temp)
      ->set_title("Coolant Temperature")
      ->set_description("Temperature of the engine coolant")
      ->set_sort_order(100);

  auto coolant_temp_calibration =
      new Linear(1.0, 0.0, "/coolantTemperature/linear");
  ConfigItem(coolant_temp_calibration)
      ->set_title("Coolant Temperature Calibration")
      ->set_description("Calibration for the coolant temperature sensor")
      ->set_sort_order(200);

  auto coolant_temp_sk_output = new SKOutputFloat(
      "propulsion.mainEngine.coolantTemperature", "/coolantTemperature/skPath");

  ConfigItem(coolant_temp_sk_output)
      ->set_title("Coolant Temperature Signal K Path")
      ->set_description("Signal K path for the coolant temperature")
      ->set_sort_order(300);

  coolant_temp->connect_to(coolant_temp_calibration)
      ->connect_to(coolant_temp_sk_output);

  const uint bme_read_delay = 1000;
  const bool bme280_found = bme280.begin(0x76) || bme280.begin(0x77);

  if (!bme280_found) {
    ESP_LOGW("main", "BME280 not found on I2C address 0x76 or 0x77");
  } else {
    auto* bme_temperature =
        new RepeatSensor<float>(bme_read_delay,
                                []() { return bme280.readTemperature() + 273.15f; });
    auto* bme_humidity =
        new RepeatSensor<float>(bme_read_delay,
                                []() { return bme280.readHumidity() / 100.0f; });
    auto* bme_pressure =
        new RepeatSensor<float>(bme_read_delay,
                                []() { return bme280.readPressure(); });

    auto* bme_temperature_sk_output =
        new SKOutputFloat("environment.outside.temperature",
                          "/bme280/temperature/skPath");
    auto* bme_humidity_sk_output = new SKOutputFloat(
        "environment.outside.relativeHumidity", "/bme280/humidity/skPath");
    auto* bme_pressure_sk_output =
        new SKOutputFloat("environment.outside.pressure",
                          "/bme280/pressure/skPath");

    ConfigItem(bme_temperature_sk_output)
        ->set_title("BME280 Temperature Signal K Path")
        ->set_description("Signal K path for BME280 temperature")
        ->set_sort_order(400);

    ConfigItem(bme_humidity_sk_output)
        ->set_title("BME280 Humidity Signal K Path")
        ->set_description("Signal K path for BME280 humidity")
        ->set_sort_order(500);

    ConfigItem(bme_pressure_sk_output)
        ->set_title("BME280 Pressure Signal K Path")
        ->set_description("Signal K path for BME280 pressure")
        ->set_sort_order(600);

    bme_temperature->connect_to(bme_temperature_sk_output);
    bme_humidity->connect_to(bme_humidity_sk_output);
    bme_pressure->connect_to(bme_pressure_sk_output);
  }

  // To avoid garbage collecting all shared pointers created in setup(),
  // loop from here.
  while (true) {
    loop();
  }
}

void loop() { event_loop()->tick(); }
