#include "esphome/core/log.h"
#include "vl6180x_light.h"

// If the range should be measured async way.
#define RANGE_ASYNC 1

namespace esphome {
namespace vl6180x_light {

static const char *TAG = "vl6180x";


Vl6180xLightOutput::Vl6180xLightOutput(uint8_t address, uint32_t update_interval)
: PollingComponent(update_interval) {
  this->set_i2c_address(address);
}

void Vl6180xLightOutput::setup() {
  ESP_LOGD(TAG, "[%7lu] LO Setup called", millis());

    // Mandatory Private Registors from Section 9 page 24 of application note
  // required to be loaded onto the VL6180X during the initialisation of the device.
  // https://www.st.com/resource/en/application_note/an4545-vl6180x-basic-ranging-application-note-stmicroelectronics.pdf

  if (read_reg8(VL6180X_REG_SYSTEM_FRESH_OUT_OF_RESET) == 1)
  {
    write_reg(0x0207, 0x01);
    write_reg(0x0208, 0x01);
    write_reg(0x0096, 0x00);
    write_reg(0x0097, 0xfd);
    write_reg(0x00e3, 0x00);
    write_reg(0x00e4, 0x04);
    write_reg(0x00e5, 0x02);
    write_reg(0x00e6, 0x01);
    write_reg(0x00e7, 0x03);
    write_reg(0x00f5, 0x02);
    write_reg(0x00d9, 0x05);
    write_reg(0x00db, 0xce);
    write_reg(0x00dc, 0x03);
    write_reg(0x00dd, 0xf8);
    write_reg(0x009f, 0x00);
    write_reg(0x00a3, 0x3c);
    write_reg(0x00b7, 0x00);
    write_reg(0x00bb, 0x3c);
    write_reg(0x00b2, 0x09);
    write_reg(0x00ca, 0x09);
    write_reg(0x0198, 0x01);
    write_reg(0x01b0, 0x17);
    write_reg(0x01ad, 0x00);
    write_reg(0x00ff, 0x05);
    write_reg(0x0100, 0x05);
    write_reg(0x0199, 0x05);
    write_reg(0x01a6, 0x1b);
    write_reg(0x01ac, 0x3e);
    write_reg(0x01a7, 0x1f);
    write_reg(0x0030, 0x00);
    write_reg(VL6180X_REG_SYSTEM_FRESH_OUT_OF_RESET, 0);
  } else {
    ESP_LOGW(TAG, "[%7lu] VL6180X not out of reset", micros());

  }

  // Recommended : Public registers - See data sheet for more detail
  write_reg(0x0011, 0x10); // Enables polling for 'New Sample ready'
                                  // when measurement completes
  write_reg(0x010a, 0x30); // Set the averaging sample period
                                  // (compromise between lower noise and
                                  // increased execution time)
  write_reg(0x003f, 0x46); // Sets the light and dark gain (upper
                                  // nibble). Dark gain should not be
                                  // changed.
  write_reg(0x0031, 0xFF); // Sets the # of range measurements after
                                  // which auto calibration of system is performed
  write_reg(0x0041, 0x63); // Set ALS integration time to 100ms
  write_reg(0x002e, 0x01); // Perform a single temperature calibration of the ranging sensor
  
  write_reg(0x0014, 0x04); // Enables the notification of a new value being available.
  //writing_register(0x001b, 0x80); // continuous mode interval
 
  // Optional: Public registers - See data sheet for more detail
  //writing_register(0x001b, 0x09); // Set default ranging inter-measurement
                                    // period to 100ms
  //writing_register(0x003e, 0x31); // Set default ALS inter-measurement period
                                    // to 500ms
  //writing_register(0x0014, 0x24); // Configures interrupt on 'New Sample
                                    // Ready threshold event'
  //write_reg(VL6180XRegister::VL6180X_REG_SYSRANGE_MAX_CONVERGENCE_TIME,25);
  this->state_ = SENSOR_STATE_IDLE;
}


float Vl6180xLightOutput::read_als() {
  // Define the ALS lux resolution and integration time (in milliseconds)
  //float als_lux_resolution = 0.32; // Example value, adjust as needed
  //float als_integration_time = 100; // Example value, adjust as needed
  float gainxvalue = 1;
  // Add the code to read the ALS data from the VL6180X sensor

  uint8_t reg;

  // Read lux
  float als_count = read_reg16(VL6180XRegister::VL6180X_REG_RESULT_ALS_VAL);

  switch (gain_) {
    case VL6180X_ALS_GAIN_1:
      gainxvalue = 1;
    break;
    case VL6180X_ALS_GAIN_1_25:
      gainxvalue = 1.25;
    break;
    case VL6180X_ALS_GAIN_1_67:
      gainxvalue = 1.67;
    break;
    case VL6180X_ALS_GAIN_2_5:
      gainxvalue = 2.5;
    break;
    case VL6180X_ALS_GAIN_5:
      gainxvalue = 5;
    break;
    case VL6180X_ALS_GAIN_10:
      gainxvalue = 10;
    break;
    case VL6180X_ALS_GAIN_20:
      gainxvalue = 20;
    break;
    case VL6180X_ALS_GAIN_40:
      gainxvalue = 40;
    break;
  };

 
  // Calculate the light level in lux
  float light_level_lux = 0.32 * ((float)als_count / (float)gainxvalue) * (100.0F / (float)alx_integration_period_);
  ESP_LOGD(TAG, "[%7lu] ALS: %8.2f lx at %gx gain in %2lu ms",millis(), light_level_lux, gainxvalue, millis() - t_als_);

  // Adjust gain for the next call.
  if (als_count == 65535 && gain_ != VL6180X_ALS_GAIN_1) {
      ESP_LOGI(TAG, "ALS overflow detected, adjusting gain");
      switch (gain_) {
          case VL6180X_ALS_GAIN_1:
            // nowhere to lower
          break;
          case VL6180X_ALS_GAIN_1_25:
            gain_ = VL6180X_ALS_GAIN_1;
          break;
          case VL6180X_ALS_GAIN_1_67:
            gain_ = VL6180X_ALS_GAIN_1_25;
          break;
          case VL6180X_ALS_GAIN_2_5:
            gain_ = VL6180X_ALS_GAIN_1_67;
          break;
          case VL6180X_ALS_GAIN_5:
            gain_ = VL6180X_ALS_GAIN_2_5;
          break;
          case VL6180X_ALS_GAIN_10:
            gain_ = VL6180X_ALS_GAIN_5;
          break;
          case VL6180X_ALS_GAIN_20:
            gain_ = VL6180X_ALS_GAIN_10;
          break;
          case VL6180X_ALS_GAIN_40:
            gain_ = VL6180X_ALS_GAIN_20;
          break;
        };
        // request immediate measure.
        if (gain_ != VL6180X_ALS_GAIN_1)
          als_requested_ = true;
    }
    if (als_count < 10000 && gain_ != VL6180X_ALS_GAIN_40) {
      ESP_LOGI(TAG, "ALS underflow detected, adjusting gain");

      switch (gain_) {
          case VL6180X_ALS_GAIN_1:
            gain_ = VL6180X_ALS_GAIN_1_25;
          break;
          case VL6180X_ALS_GAIN_1_25:
            gain_ = VL6180X_ALS_GAIN_1_67;
          break;
          case VL6180X_ALS_GAIN_1_67:
            gain_ = VL6180X_ALS_GAIN_2_5;
          break;
          case VL6180X_ALS_GAIN_2_5:
            gain_ = VL6180X_ALS_GAIN_5;
          break;
          case VL6180X_ALS_GAIN_5:
            gain_ = VL6180X_ALS_GAIN_10;
          break;
          case VL6180X_ALS_GAIN_10:
          gain_ = VL6180X_ALS_GAIN_20;
          break;
          case VL6180X_ALS_GAIN_20:
          gain_ = VL6180X_ALS_GAIN_40;
          break;
          case VL6180X_ALS_GAIN_40:
          //nowhere to increase
          break;
        };
    }

  return light_level_lux;
}

uint8_t Vl6180xLightOutput::read_range() {
  uint32_t t_now = millis();
  uint8_t status = this->read_reg8(
    VL6180XRegister::VL6180X_REG_RESULT_RANGE_STATUS);
    status = status >> 4;
  //ESP_LOGD(TAG, "[%7lu] return status: %d, after %lu ms", micros(),status, t_now - t_rs_);

  // Check for errors
  if (status != VL6180XError::VL6180X_ERROR_NONE) {
    handle_error(status);
    this->write_reg(VL6180XRegister::VL6180X_REG_SYSTEM_INTERRUPT_CLEAR, 0x07);  // Clear the interrupt
    return 255;
  }
  uint8_t range = this->read_reg8(VL6180XRegister::VL6180X_REG_RESULT_RANGE_VAL);
  this->write_reg(VL6180XRegister::VL6180X_REG_SYSTEM_INTERRUPT_CLEAR, 0x07);  // Clear the interrupt
  //ESP_LOGI(TAG, "[%7lu] Distance : %3d mm in %2d ms",t_now, range,t_now - t_rs_);
  return range;

}


// Handle errors
void Vl6180xLightOutput::handle_error(uint8_t status) {
  
  switch (status) {
    case VL6180XError::VL6180X_ERROR_NONE:
      // No error
      break;
    case VL6180XError::VL6180X_ERROR_SYSERR_1:
      ESP_LOGE(TAG, "System error; VCSEL Continuity Test; No measurement possible");
      return;
    case VL6180XError::VL6180X_ERROR_SYSERR_2:
      ESP_LOGE(TAG, "System error; VCSEL Watchdog Test; No measurement possible");
      return;
    case VL6180XError::VL6180X_ERROR_SYSERR_3:
      ESP_LOGE(TAG, "System error; VCSEL Watchdog; No measurement possible");
      return;
    case VL6180XError::VL6180X_ERROR_SYSERR_4:
      ESP_LOGE(TAG, "System error; PLL1 Lock; No measurement possible");
      return;
    case VL6180XError::VL6180X_ERROR_SYSERR_5:
      ESP_LOGE(TAG, "System error; PLL2 Lock; No measurement possible");
      return;
    case VL6180XError::VL6180X_ERROR_ECEFAIL_6:
      ESP_LOGV(TAG, "Early Convergence Estimate failed");
      return;
    case VL6180XError::VL6180X_ERROR_NOCONVERGE_7:
      ESP_LOGV(TAG, "No target detected, system did not converge before the specified max convergence time limit");
      return;
    case VL6180XError::VL6180X_ERROR_RANGEIGNORE_8:
      ESP_LOGV(TAG, "Ignore threshold check failed");
      return;
    // Bits 9-10 Not used.
    case VL6180XError::VL6180X_ERROR_SNR_11:
      ESP_LOGV(TAG, "Ambient conditions too high. Measurement not valid");
      return;
    // 12/14 Range value < 0
    // If the target is very close (0-10mm) and the offset
    // is not correctly calibrated it could lead to a small
    // negative value.
    case VL6180XError::VL6180X_ERROR_RAWUFLOW_12:
      ESP_LOGV(TAG, "Raw range algo underflow; Target too close");
      return;
    // 13/15 Range value out of range. This occurs when the
    // target is detected by the device but is placed at a
    // high distance (> 200mm) resulting in internal
    // variable overflow.
    case VL6180XError::VL6180X_ERROR_RAWOFLOW_13:
      ESP_LOGV(TAG, "Raw range algo overflow; Target too far");
      return;
    case VL6180XError::VL6180X_ERROR_RANGEUFLOW_14:
      ESP_LOGV(TAG, "Range algo underflow; Target too close");
      return;
    case VL6180XError::VL6180X_ERROR_RANGEOFLOW_15:
      ESP_LOGV(TAG, "Range algo overflow; Target too far");
      return;
    case VL6180XError::VL6180X_ERROR_RANGINGFILTERED_16:
      ESP_LOGE(TAG, "Distance filtered by Wrap Around Filter (WAF). Occurs when a high reflectance target is detected between 600mm to 1.2m");
      return;
    // Bit 17 Not used.
    case VL6180XError::VL6180X_ERROR_DATANOTREADY_18:
      ESP_LOGE(TAG, "Error returned by VL6180x_RangeGetMeasurementIfReady() when ranging data is not ready.");
      return;
    default:
      ESP_LOGE(TAG, "Unknown error: %d",status);
      return;
  }
}


void Vl6180xLightOutput::loop() {
  uint32_t t_now = 0;
  t_now = millis();
  //this->Vl6180xLightOutput::LightOutput::write_state()
  //ESP_LOGD(TAG, "[%7lu]: Loop state: %d", micros(),this->state_);
    // Range state machine
  switch (this->state_) {
    case SENSOR_STATE_IDLE: {

      if (als_requested_) {
        //ESP_LOGD(TAG, "[%7lu] Start ALS", micros());
        als_requested_ = false;
        uint8_t reg;
        reg = read_reg8(VL6180XRegister::VL6180X_REG_SYSTEM_INTERRUPT_CONFIG);
        //ESP_LOGD(TAG, "[%lu] ALS start reg start: %d", micros(),reg);
        reg &= ~0x38;
        reg |= (0x4 << 3); // IRQ on ALS ready
        write_reg(VL6180XRegister::VL6180X_REG_SYSTEM_INTERRUPT_CONFIG, reg);
        //ESP_LOGD(TAG, "[%lu] ALS start reg my: %d", micros(),reg); // 36
      
        // Set integration period
        write_reg(VL6180XRegister::VL6180X_REG_SYSALS_INTEGRATION_PERIOD_HI, 0);
        write_reg(VL6180XRegister::VL6180X_REG_SYSALS_INTEGRATION_PERIOD_LO, alx_integration_period_);
      
        // Analog gain
        write_reg(VL6180XRegister::VL6180X_REG_SYSALS_ANALOGUE_GAIN, 0x40 | gain_);
        // Start ALS
        //ESP_LOGD(TAG, "Requesting ALS at %lu", micros());
        write_reg(VL6180XRegister::VL6180X_REG_SYSALS_START, 0x1);


        this->state_ = SENSOR_STATE_WAIT_ALS;
        t_als_ = millis();
        

      } else { // No ALS request - continue with range.
        this->write_reg(VL6180X_REG_SYSRANGE_START, 0x01);
        t_rs_ = millis();

        #ifdef RANGE_ASYNC 
        this->state_ = SENSOR_STATE_WAIT_RANGE;
        #else
         t_now = millis();
         while (t_now - t_rs_ < 50) {
          uint8_t reg_status = read_reg8(VL6180XRegister::VL6180X_REG_RESULT_INTERRUPT_STATUS_GPIO);
          if (reg_status == 4) 
            break;

          t_now = millis();
         }
         if (t_now - t_rs_ >= 50) {
          ESP_LOGE(TAG,"VL6180X Range Timed out!");
          //ESP_LOGD(TAG, "[%7lu] return status: %d, after %lu ms", micros(),status, t_now - t_rs_);
          this->write_reg(VL6180XRegister::VL6180X_REG_SYSTEM_INTERRUPT_CLEAR, 0x07);  // Clear the interrupt
          this->state_ = SENSOR_STATE_DATA_READY;
          break;
        }
         range_ = read_range();
         this->state_ = SENSOR_STATE_IDLE;

        #endif
        //ESP_LOGD(TAG, "[%7lu] Start range", micros());
        
      }
    }
      break;
    #ifdef RANGE_ASYNC // If we do want to measure the range asyncronously. 
    case SENSOR_STATE_WAIT_RANGE: {
      //this->check_measurement();
      
      //ESP_LOGD(TAG, "[%7lu] Check measurement, reg: %d time: %d ms", micros(),reg_status, t_now - t_rs_ );

      if (t_now - t_rs_ > 100) {
        ESP_LOGE(TAG,"VL6180X Range Timed out!");
        //ESP_LOGD(TAG, "[%7lu] return status: %d, after %lu ms", micros(),status, t_now - t_rs_);
        this->write_reg(VL6180XRegister::VL6180X_REG_SYSTEM_INTERRUPT_CLEAR, 0x07);  // Clear the interrupt
        this->state_ = SENSOR_STATE_DATA_READY;
        break;
      }

      
      uint8_t reg_status = read_reg8(VL6180XRegister::VL6180X_REG_RESULT_INTERRUPT_STATUS_GPIO);
      if (reg_status == 4) {
        
        range_ = read_range();
        this->state_ = SENSOR_STATE_IDLE;
      }
    }
      break;
      #endif
    case SENSOR_STATE_WAIT_ALS: {
      uint8_t als_reg_status = read_reg8(VL6180XRegister::VL6180X_REG_RESULT_INTERRUPT_STATUS_GPIO);
      //ESP_LOGD(TAG, "[%lu] ALS reg status: %d", micros(),als_reg_status);
      if (4 == ((als_reg_status >> 3) & 0x7)) {
        //ESP_LOGD(TAG, "Got ALS at %lu", micros());
        als_ = read_als();
        // If next measurement not immediately requested due to an overflow
        if (this->als_sensor_ != nullptr && ! als_requested_) 
            als_sensor_->publish_state(als_);
        // Clear interrupt
        write_reg(VL6180XRegister::VL6180X_REG_SYSTEM_INTERRUPT_CLEAR, 0x07);
        this->state_ = SENSOR_STATE_IDLE;
      }

      // timeouts
      if (t_now - t_als_ > 200) {
        ESP_LOGE(TAG,"VL6180X ALS Timed out!");
        ESP_LOGD(TAG, "[%7lu] ALS reg status: %d", micros(),als_reg_status);
        this->write_reg(VL6180XRegister::VL6180X_REG_SYSTEM_INTERRUPT_CLEAR, 0x07);  // Clear the interrupt
        this->state_ = SENSOR_STATE_DATA_READY;
        break;

      }
    }
    break;
    
    case SENSOR_STATE_DATA_READY:
        // noop, one cycle wait?
        this->state_ = SENSOR_STATE_IDLE;
      break;
      
  } // End range state machine

  //ESP_LOGD(TAG, "[%7lu] Switch loop, range: %3d", millis(),range_);
  moving_average_ = (7 * moving_average_ + (float)range_)/8;
  if (t_now - t_sent_ >= 100000) {
    ESP_LOGD(TAG, "[%7lu] Range: %d (MA: %0.1f) ambient: %0.1f",
            t_now, range_, moving_average_,als_);
    t_sent_ = t_now;
  }

  // Light switch state machine.
  uint32_t adj_pct = 0;
  // If we do have a hand
  hand_near_ = (range_ < 255 && moving_average_ < range_max_ + 5);
  switch (sw_state_) {
    case SW_STATE_IDLE:   // Idling, nobody near. -----------------------------------
      if (hand_near_) {
          ESP_LOGD(TAG,"Hand Near, to STATE_HAND: r %d ma %0.1f",range_, moving_average_);
          t_hand_ = t_now;
          sw_state_ = SW_STATE_HAND; 
      }

    break;
    case SW_STATE_HAND: // We have a hand! ------------------------------------------
      if (hand_near_) {
          hold_ms_ = t_now - t_hand_;
          if (hold_ms_ > 1000 ) {
            ESP_LOGD(TAG,"Hold for %d, to STATE_ADJ",hold_ms_);
            sw_state_ = SW_STATE_ADJ;
            t_hand_ = t_now;
          }
      } else {
          if ( hold_ms_ < 1000 ) {
            //out_state ^= 1;
            //ledc_set_duty(out_state * pwm_value);

            ESP_LOGD(TAG,"--- TOGGLE --- ");
            ESP_LOGD(TAG,"Hold for %d, to STATE_ACK",hold_ms_);
            //if (this->output_ != nullptr)
            //  this->output_->set_state(false);
            // === TODO - : toggle output  =======================
            //this->output_->turn_on();
            this->lightstate_->toggle();
            this->lightstate_->publish_state();
            //this->lightstate_->set_immediately_();
            sw_state_ = SW_STATE_ACK;
          }
          

      }
      break;
    case SW_STATE_ADJ: // Hand near for a long time - adjusting the brightness. 

      if (hand_near_) {
          hold_ms_ = t_now - t_hand_;
          if (hold_ms_ < 3000) { 

            if (moving_average_ > range_max_)
                moving_average_ = range_max_;
            if (moving_average_ < range_min_)
                moving_average_ = range_min_;   
            
            adj_pct = (uint32_t)(100 * (range_max_ - moving_average_) / (range_max_ - range_min_));

            // === TODO - : set brightness while adjusting.  =======================
            this->output_->set_level((range_max_ - moving_average_) / (range_max_ - range_min_));
            
            
            //ledc_set_duty(expo[adj_pct]);
            ESP_LOGD(TAG,"Adjust, %0.1f %d",moving_average_,adj_pct);
          } else {
            ESP_LOGD(TAG,"Adjust end, to STATE_WAIT at %d",adj_pct);
            // === TODO - : set final brightness.  =======================
            //pwm_value = expo[adj_pct];
            //out_state = 1;
            //ledc_set_duty(0);
            //vTaskDelay(pdMS_TO_TICKS(150));
            //ledc_set_duty(pwm_value);
            sw_state_ = SW_STATE_WAIT;
          }
      } else {  // No hand
            ESP_LOGD(TAG,"Adjust cancelled at %d, to STATE_IDLE",range_);
            sw_state_ = SW_STATE_IDLE;
            // === TODO - : set original brightness.  =======================
            //ledc_set_duty(pwm_value);
      }
      break;
    case SW_STATE_ACK:   // Acknowledge command by blink
      ESP_LOGD(TAG,"ACK ACK ACK, to STATE_IDLE");

      // === TODO - : blink 3 times.  =======================
      /*for (short i=0; i<3; i++) {
      gpio_set_level(AUXLED, 1);
      vTaskDelay(pdMS_TO_TICKS(80));
      gpio_set_level(AUXLED, 0);
      vTaskDelay(pdMS_TO_TICKS(80));
      };
      vTaskDelay(pdMS_TO_TICKS(300));*/
      sw_state_ = SW_STATE_IDLE;
      break;

    case SW_STATE_WAIT: // Wait for the hand to leave -------------------------------
      if (!hand_near_) {
      hold_ms_ = t_now;
      ESP_LOGD(TAG,"Hand left, to STATE_ACK");
      sw_state_ = SW_STATE_ACK;
      }
      break;
  } // End case
} // End loop()

void Vl6180xLightOutput::update() {
     //ESP_LOGD(TAG, "[%7lu] Update called", millis());
     als_requested_ = true;

}

light::LightTraits Vl6180xLightOutput::get_traits() {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;

}

void Vl6180xLightOutput::write_state(light::LightState *state) {
    float bright;
    state->current_values_as_brightness(&bright);
    this->output_->set_level(bright);
    
    ESP_LOGI(TAG, "Output level: %3.1f", bright);
    

}

void Vl6180xLightOutput::dump_config(){
  ESP_LOGCONFIG(TAG, "VL6180X Light:");
  LOG_SENSOR("  ", "Range", this);
  LOG_UPDATE_INTERVAL(this);
  LOG_I2C_DEVICE(this);
}

// Write 1 byte to the VL6180X at 'address'
void Vl6180xLightOutput::write_reg(uint16_t reg, uint8_t data) {
  this->write_register16(reg, &data, 1);
}

// Read 1 byte from the VL6180X at 'address'
uint8_t Vl6180xLightOutput::read_reg8(uint16_t reg) {
  uint8_t data = 0;
  this->read_register16(reg, &data, 1);
  return data;
}

// Read 2 byte from the VL6180X at 'address'
uint16_t Vl6180xLightOutput::read_reg16(uint16_t reg) {
  uint8_t data[2];
  this->read_register16(reg, data, 2);
  return (data[0]) << 8 | data[1];
}



} //namespace vl6180x_light
} //namespace esphome