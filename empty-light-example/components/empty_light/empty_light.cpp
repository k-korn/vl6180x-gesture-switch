#include "esphome/core/log.h"
#include "empty_light.h"

namespace esphome {
namespace empty_light {

static const char *TAG = "empty_light.light";

void EmptyLightOutput::setup() {
   ESP_LOGD(TAG, "[%7lu]: Light Setup ", millis());
}
void EmptyLightOutput::loop() {
    
	// Simple "random every few seconds"
	if (random_float() < 0.005) {
		ESP_LOGD(TAG, "[%7lu]: Toggle Light ", millis());
		
		// here?
		this->lightstate_->toggle();
		
		// this blinks the output.
		this->output_->set_level(random_float());
	}
}

light::LightTraits EmptyLightOutput::get_traits() {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;
}

void EmptyLightOutput::write_state(light::LightState *state) {
    float bright;
    state->current_values_as_brightness(&bright);
    this->output_->set_level(bright);
}

void EmptyLightOutput::dump_config(){
    ESP_LOGCONFIG(TAG, "Empty custom light");
}



} //namespace empty_light
} //namespace esphome