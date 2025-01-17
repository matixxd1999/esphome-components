#include "ble_adv_light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ble_adv_controller {

static const char *TAG = "ble_adv_light";

float ensure_range(float f, float mini = 0.0f, float maxi = 1.0f) {
  return (f > maxi) ? maxi : ( (f < mini) ? mini : f );
}

void BleAdvLightBase::dump_config() {
  light::LightState::dump_config();
  BleAdvEntity::dump_config_base(TAG);
  ESP_LOGCONFIG(TAG, "  Secondary: %s", this->secondary_ ? "YES" : "NO");
}

void BleAdvLightBase::command(CommandType cmd, float value1, float value2, float value3) {
  BleAdvEntity::command(this->secondary_ ? (CommandType)((uint16_t)cmd + 10) : cmd, value1, value2, value3);
}

void BleAdvLightBase::publish(const BleAdvGenCmd & gen_cmd) {
  // Do not process if the light is not the target
  if (!gen_cmd.is_light_cmd(this->secondary_)) return;

  // Rewrite command if secondary
  BleAdvGenCmd gen_cmd_rew = gen_cmd;
  if (this->secondary_) {
    gen_cmd_rew.cmd = (CommandType)((uint16_t)gen_cmd.cmd - 10);
  }
  this->publish_impl(gen_cmd_rew);
}

void BleAdvLightCww::set_min_brightness(int min_brightness, int min, int max, int step) { 
  this->number_min_brightness_.traits.set_min_value(min);
  this->number_min_brightness_.traits.set_max_value(max);
  this->number_min_brightness_.traits.set_step(step);
  this->number_min_brightness_.state = min_brightness; 
}

void BleAdvLightCww::set_traits(float cold_white_temperature, float warm_white_temperature) {
  this->traits_.set_supported_color_modes({light::ColorMode::COLD_WARM_WHITE});
  this->traits_.set_min_mireds(cold_white_temperature);
  this->traits_.set_max_mireds(warm_white_temperature);
}

void BleAdvLightCww::setup() {
  BleAdvLightBase::setup();
  if (this->get_parent()->is_show_config()) {
    this->number_min_brightness_.init("Min Brightness", this->get_name());
  }
}

void BleAdvLightCww::dump_config() {
  BleAdvLightBase::dump_config();
  ESP_LOGCONFIG(TAG, "  BleAdvLight - Cold / Warm White");
  ESP_LOGCONFIG(TAG, "  Cold White Temperature: %f mireds", this->traits_.get_min_mireds());
  ESP_LOGCONFIG(TAG, "  Warm White Temperature: %f mireds", this->traits_.get_max_mireds());
  ESP_LOGCONFIG(TAG, "  Constant Brightness: %s", this->constant_brightness_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Minimum Brightness: %.0f%%", this->get_min_brightness() * 100);
}

float BleAdvLightCww::get_ha_brightness(float device_brightness) {
  return ensure_range((ensure_range(device_brightness, this->get_min_brightness()) - this->get_min_brightness()) / (1.f - this->get_min_brightness()), 0.01f);
}

float BleAdvLightCww::get_device_brightness(float ha_brightness) {
  return ensure_range(this->get_min_brightness() + ha_brightness * (1.f - this->get_min_brightness()));
}

float BleAdvLightCww::get_ha_color_temperature(float device_color_temperature) {
  return ensure_range(device_color_temperature) * (this->traits_.get_max_mireds() - this->traits_.get_min_mireds()) + this->traits_.get_min_mireds();
}

float BleAdvLightCww::get_device_color_temperature(float ha_color_temperature) {
  return ensure_range((ha_color_temperature - this->traits_.get_min_mireds()) / (this->traits_.get_max_mireds() - this->traits_.get_min_mireds()));
}

void BleAdvLightCww::publish_impl(const BleAdvGenCmd & gen_cmd) {
  light::LightCall call = this->make_call();
  call.set_color_mode(light::ColorMode::COLD_WARM_WHITE);

  if (gen_cmd.cmd == CommandType::LIGHT_ON) {
    call.set_state(true).perform();
  } else if (gen_cmd.cmd == CommandType::LIGHT_OFF) {
    call.set_state(false).perform();
  } else if (gen_cmd.cmd == CommandType::LIGHT_TOGGLE) {
    call.set_state(this->is_off_).perform();
  } else if (this->is_off_) {
    ESP_LOGD(TAG, "Change ignored as entity is OFF.");
    return;
  }
  
  if (gen_cmd.cmd == CommandType::LIGHT_CCT) {
    if (gen_cmd.param == 0) {
      call.set_color_temperature(this->get_ha_color_temperature(gen_cmd.args[0])).perform();
    } else if (gen_cmd.param == 1) { // Color Temp +
      call.set_color_temperature(this->get_ha_color_temperature(this->warm_color_ + gen_cmd.args[0])).perform();
    } else if (gen_cmd.param == 2) { // Color Temp -
      call.set_color_temperature(this->get_ha_color_temperature(this->warm_color_ - gen_cmd.args[0])).perform();
    }
  } else if (gen_cmd.cmd == CommandType::LIGHT_DIM) {
    if (gen_cmd.param == 0) {
      call.set_brightness(this->get_ha_brightness(gen_cmd.args[0])).perform();
    } else if (gen_cmd.param == 1) { // Brightness +
      call.set_brightness(this->get_ha_brightness(this->brightness_ + gen_cmd.args[0])).perform();
    } else if (gen_cmd.param == 2) { // Brightness -
      call.set_brightness(this->get_ha_brightness(this->brightness_ - gen_cmd.args[0])).perform();
    }
  } else if (gen_cmd.cmd == CommandType::LIGHT_WCOLOR) {
    // standard cold(args[0]) / warm(args[1]) update
    call.set_color_temperature(this->get_ha_color_temperature(gen_cmd.args[1] / (gen_cmd.args[0] + gen_cmd.args[1])));
    call.set_brightness(this->get_ha_brightness(std::max(gen_cmd.args[0], gen_cmd.args[1])));
    call.perform();
  } else if (gen_cmd.cmd == CommandType::LIGHT_DIM_CCT) {
    call.set_color_temperature(this->get_ha_color_temperature(1.0 - gen_cmd.args[0]));
    call.set_brightness(this->get_ha_brightness(gen_cmd.args[1]));
    call.perform();
  }
}

void BleAdvLightCww::control() {
  // If target state is off, switch off
  if (this->current_values.get_state() == 0) {
    ESP_LOGD(TAG, "Switch OFF");
    this->command(CommandType::LIGHT_OFF);
    this->is_off_ = true;
    return;
  }

  // If current state is off, switch on
  if (this->is_off_) {
    ESP_LOGD(TAG, "Switch ON");
    this->command(CommandType::LIGHT_ON);
    this->is_off_ = false;
  }

  // Compute Corrected Brigtness / Warm Color Temperature (potentially reversed) as float: 0 -> 1
  float updated_brf = this->get_device_brightness(this->current_values.get_brightness());
  float updated_ctf = this->get_device_color_temperature(this->current_values.get_color_temperature());
  updated_ctf = this->get_parent()->is_reversed() ? 1.0 - updated_ctf : updated_ctf;
  // During transition(current / remote states are not the same), do not process change 
  //    if Brigtness / Color Temperature was not modified enough
  float br_diff = abs(this->brightness_ - updated_brf) * 100;
  float ct_diff = abs(this->warm_color_ - updated_ctf) * 100;
  bool is_last = (this->current_values == this->remote_values);
  if ((br_diff < 3 && ct_diff < 3 && !is_last) || (is_last && br_diff == 0 && ct_diff == 0)) {
    return;
  }
  
  this->brightness_ = updated_brf;
  this->warm_color_ = updated_ctf;

  if(!this->split_dim_cct_) {
    light::LightColorValues eff_values = this->current_values;
    eff_values.set_brightness(updated_brf);
    float cwf, wwf;
    if (this->get_parent()->is_reversed()) {
      eff_values.as_cwww(&wwf, &cwf, 0, this->constant_brightness_);
    } else {
      eff_values.as_cwww(&cwf, &wwf, 0, this->constant_brightness_);
    }
    ESP_LOGD(TAG, "Updating Cold: %.0f%%, Warm: %.0f%%", cwf*100, wwf*100);
    this->command(CommandType::LIGHT_WCOLOR, cwf, wwf);
    this->command(CommandType::LIGHT_DIM_CCT, 1.0 - updated_ctf, updated_brf);
  } else {
    if (ct_diff != 0) {
      ESP_LOGD(TAG, "Updating warm color temperature: %.0f%%", updated_ctf*100);
      this->command(CommandType::LIGHT_CCT, updated_ctf);
    }
    if (br_diff != 0) {
      ESP_LOGD(TAG, "Updating brightness: %.0f%%", updated_brf*100);
      this->command(CommandType::LIGHT_DIM, updated_brf);
    }
  }
}

/*********************
Binary Light
**********************/

void BleAdvLightBinary::dump_config() {
  BleAdvLightBase::dump_config();
  ESP_LOGCONFIG(TAG, "  BleAdvLight - Binary");
}

void BleAdvLightBinary::publish_impl(const BleAdvGenCmd & gen_cmd) {
  light::LightCall call = this->make_call();
  call.set_color_mode(light::ColorMode::ON_OFF);

  if (gen_cmd.cmd == CommandType::LIGHT_ON) {
    call.set_state(true).perform();
  } else if (gen_cmd.cmd == CommandType::LIGHT_OFF) {
    call.set_state(false).perform();
  } else if (gen_cmd.cmd == CommandType::LIGHT_TOGGLE) {
    bool binary;
    this->current_values_as_binary(&binary);
    call.set_state(!binary).perform();
  }
}

void BleAdvLightBinary::control() {
  bool binary;
  this->current_values_as_binary(&binary);
  if (binary) {
    ESP_LOGD(TAG, "Switch ON");
    this->command(CommandType::LIGHT_ON);
  } else {
    ESP_LOGD(TAG, "Switch OFF");
    this->command(CommandType::LIGHT_OFF);
  }
}

/*********************
RGB Light
**********************/

void BleAdvLightRGB::dump_config() {
  BleAdvLightBase::dump_config();
  ESP_LOGCONFIG(TAG, "  BleAdvLight - RGB");
}

void BleAdvLightRGB::publish_impl(const BleAdvGenCmd & gen_cmd) {
  light::LightCall call = this->make_call();
  call.set_color_mode(light::ColorMode::RGB);

  if (gen_cmd.cmd == CommandType::LIGHT_ON) {
    call.set_state(true).perform();
  } else if (gen_cmd.cmd == CommandType::LIGHT_OFF) {
    call.set_state(false).perform();
  } else if (gen_cmd.cmd == CommandType::LIGHT_TOGGLE) {
    call.set_state(this->is_off_).perform();
  } else if (this->is_off_) {
    ESP_LOGD(TAG, "Change ignored as entity is OFF.");
    return;
  }

  if (gen_cmd.cmd == CommandType::LIGHT_RGB_DIM) {
    call.set_brightness(gen_cmd.args[0]).perform();
  } else if (gen_cmd.cmd == CommandType::LIGHT_RGB_RGB) {
    call.set_red(gen_cmd.args[0]);
    call.set_green(gen_cmd.args[1]);
    call.set_blue(gen_cmd.args[2]);
    call.perform();
  }

}

void BleAdvLightRGB::control() {
  // If target state is off, switch off
  if (this->current_values.get_state() == 0) {
    ESP_LOGD(TAG, "Switch OFF");
    this->command(CommandType::LIGHT_OFF);
    this->is_off_ = true;
    return;
  }

  // If current state is off, switch on
  if (this->is_off_) {
    ESP_LOGD(TAG, "Switch ON");
    this->command(CommandType::LIGHT_ON);
    this->is_off_ = false;
  }

  // Compute Corrected Brigtness
  float upd_br = this->current_values.get_brightness();
  float upd_r = this->current_values.get_red();
  float upd_g = this->current_values.get_green();
  float upd_b = this->current_values.get_blue();

  // During transition(current / remote states are not the same), do not process change 
  //    if Brigtness / RGB was not modified enough
  float diff_br = abs(this->brightness_ - upd_br) * 100;
  float diff_r = abs(this->red_ - upd_r) * 100;
  float diff_g = abs(this->green_ - upd_g) * 100;
  float diff_b = abs(this->blue_ - upd_b) * 100;
  bool is_last = (this->current_values == this->remote_values);
  if ((diff_br < 3 && diff_r < 3 && diff_g < 3 && diff_b < 3 && !is_last) 
      || (is_last && diff_br == 0 && diff_r == 0 && diff_g == 0 && diff_b == 0)) {
    return;
  }
  
  this->brightness_ = upd_br;
  this->red_ = upd_r;
  this->green_ = upd_g;
  this->blue_ = upd_b;

  if (!this->split_dim_rgb_) {
    // Only one message to be sent: use the esphome feature to compute the effective RGB, but we could do basic multi
    float red, green, blue;
    this->current_values.as_rgb(&red, &green, &blue, 0, false);
    ESP_LOGD(TAG, "Updating raw r: %.0f%%, g: %.0f%%, b: %.0f%%", red*100, green*100, blue*100);
    this->command(CommandType::LIGHT_RGB_RGB, red, green, blue);
  } else {
    // Send 2 different messages, each one only if needed: 1 for brightness and one for RGB
    if (diff_br != 0) {
      ESP_LOGD(TAG, "Updating brightness: %.0f%%", upd_br*100);
      this->command(CommandType::LIGHT_RGB_DIM, upd_br);
    }
    if (diff_r != 0 || diff_g != 0 || diff_b !=0 ) {
      ESP_LOGD(TAG, "Updating relative r: %.0f%%, g: %.0f%%, b: %.0f%%", upd_r*100, upd_g*100, upd_b*100);
      this->command(CommandType::LIGHT_RGB_RGB, upd_r, upd_g, upd_b);
    }
  }
}

} // namespace ble_adv_controller
} // namespace esphome

