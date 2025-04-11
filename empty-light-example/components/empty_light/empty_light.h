#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace empty_light {

class EmptyLightOutput : public light::LightOutput, public PollingComponent {
 public:
  //EmptyLightOutput( uint32_t update_interval = 5000);
  void setup() override;
  void loop() override;
  void update() override {/*ESP_LOGE("APP","Update called");*/}
  light::LightTraits get_traits() override;
  void set_output(output::FloatOutput *output) { output_ = output; }
  void setup_state(light::LightState *state) override { this->lightstate_ = state; ESP_LOGE("APP", "Setup State Called"); }
  void write_state(light::LightState *state) override;
  //void set_update_interval(uint32_t update_interval) override {ESP_LOGE("APP", "--------   Update Interval to %d called",update_interval);}
  void dump_config() override;
 
 protected:
  output::FloatOutput *output_;
  light::LightState *lightstate_{nullptr};
};

} //namespace empty_light
} //namespace esphome