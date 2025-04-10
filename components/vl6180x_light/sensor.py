import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, i2c
from esphome.const import (
  CONF_ID,
  DEVICE_CLASS_ILLUMINANCE,
  ICON_BRIGHTNESS_5,
  UNIT_LUX,
)
DEPENDENCIES = ['i2c']

vl6180x_light_ns = cg.esphome_ns.namespace('vl6180x_light')

Vl6180xLightOutput = vl6180x_light_ns.class_('Vl6180xLightOutput', sensor.Sensor, cg.PollingComponent, i2c.I2CDevice)

CONF_ALS = "als"

CONFIG_SCHEMA = cv.All(
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(Vl6180xLightOutput),
      cv.Optional(CONF_ALS): sensor.sensor_schema(
          unit_of_measurement=UNIT_LUX,
          icon=ICON_BRIGHTNESS_5,
          accuracy_decimals=2,
          device_class=DEVICE_CLASS_ILLUMINANCE,
      ),

    }
  )
  .extend(cv.polling_component_schema('60s'))
  .extend(i2c.i2c_device_schema(0x29)),
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await i2c.register_i2c_device(var, config)

  if CONF_ALS in config:
    als = await sensor.new_sensor(config[CONF_ALS])
    cg.add(var.set_als_sensor(als))

