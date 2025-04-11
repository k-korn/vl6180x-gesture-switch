import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, output
from esphome.const import CONF_OUTPUT_ID, CONF_OUTPUT, CONF_UPDATE_INTERVAL

empty_light_ns = cg.esphome_ns.namespace('empty_light')
EmptyLightOutput = empty_light_ns.class_('EmptyLightOutput', light.LightOutput, cg.PollingComponent)



CONFIG_SCHEMA = (
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
        {
           cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(EmptyLightOutput),
           cv.Required(CONF_OUTPUT): cv.use_id(output.FloatOutput),
        }
    )
    .extend(cv.polling_component_schema('10s'))
) 

async def to_code(config):
    config.pop(
        CONF_UPDATE_INTERVAL
    )  # drop UPDATE_INTERVAL as it does not apply to the light component
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
	
    await light.register_light(var, config)
    await cg.register_component(var, config)

    out = await cg.get_variable(config[CONF_OUTPUT])
    cg.add(var.set_output(out))