import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, output, sensor, i2c
from esphome.const import (
  CONF_OUTPUT_ID, 
  CONF_OUTPUT,
  CONF_UPDATE_INTERVAL,
  DEVICE_CLASS_ILLUMINANCE,
  ICON_BRIGHTNESS_5,
  UNIT_LUX,
)

AUTO_LOAD = ["sensor"]
DEPENDENCIES = ['i2c']

#from ./__init__.py import Vl6180xLightOutput
vl6180x_light_ns = cg.esphome_ns.namespace('vl6180x_light')
Vl6180xLightOutput = vl6180x_light_ns.class_('Vl6180xLightOutput', light.LightOutput, cg.PollingComponent, i2c.I2CDevice)

CONF_AMBIENT = "ambient"

CONFIG_SCHEMA = (
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
        {
           cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(Vl6180xLightOutput),
           cv.Required(CONF_OUTPUT): cv.use_id(output.FloatOutput),
           cv.Optional(CONF_AMBIENT): sensor.sensor_schema(
             unit_of_measurement=UNIT_LUX,
             icon=ICON_BRIGHTNESS_5,
             accuracy_decimals=2,
             device_class=DEVICE_CLASS_ILLUMINANCE,
           ),
        }
    )
    .extend(cv.polling_component_schema('10s'))
    .extend(i2c.i2c_device_schema(0x29))
)



async def to_code(config):
    config.pop(
        CONF_UPDATE_INTERVAL
    )  # drop UPDATE_INTERVAL as it does not apply to the light component
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    out = await cg.get_variable(config[CONF_OUTPUT])
    cg.add(var.set_output(out))
    if CONF_AMBIENT in config:
      als = await sensor.new_sensor(config[CONF_AMBIENT])
      cg.add(var.set_als_sensor(als))