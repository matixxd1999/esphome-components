# ble_adv_controller

## Description
Allows to control devices using BLE ADV protocol.

One controller is needed for each device to be controlled, you can have several controllers with different id in the same esp32.

To each controller you can potentially associate one or several of the following:
* A Main Light (RGB, Cold / Warm White, Binary)
* A Secondary Light (RGB, Cold / Warm White, Binary)
* A Fan with control of speed / oscillation / direction

Needs `ble_adv_handler` (forced include) to work.

## Main Variables
- **encoding** (Required): the encoding, can be any of:
    - 'zhijia', for 'Zhi Jia' app
    - 'zhiguang', for 'Zhi Guang' app, same settings as 'zhijia'
    - 'zhimei', for 'ZhiMeiDengKong' app
    - 'fanlamp_pro', for 'FanLamp Pro' app or 'ApplianceSmart' App
    - 'lampsmart_pro', for 'LampSmart Pro' Android App, 'LampSmart Pro-Soft Lighting' App or 'Vmax smart' App
    - 'lampsmart_ios', for 'LampSmart Pro' iOS App
    - 'agarce', for 'Smart Light' App
    - 'remote', for some of the remotes we know
    - 'other', for legacy variants from initial repos, may correspond to removed app 'FanLamp' or 'ControlSwitch'

    Allowed / available values for other variable depends on the value of this parameter. 'lampsmart_pro', 'other' or 'remote' are very similar to 'fanlamp_pro' have globally the same options than 'fanlamp_pro'. 'zhiguang' has the same options than 'zhijia'.

- **variant** (Optional): the variant of the encoding
    - For 'zhijia': Can be v0 (MSC16), v1 (MSC26) or v2 (MSC26A), default is v2
    - For 'zhimei': Can be vf0 or vf1 for Lamp with Fan, or v1, v1b or v2 for light only, default is v1
    - For 'fanlamp_pro': Can be any of v1, v2 or v3, default is v3
    - For 'lampsmart_pro': Can be v1, v2, v3 or v3b, default is v3
    - For 'lampsmart_ios': Can be v1, v1b, v3 or v3b, default is v3
    - For 'agarce': Can be v3 or v4, default is v4
    - For 'remote': can be v1 or v3 (only remotes we know for now..), default is v3
    - For 'other': Can be any of v1a / v1b / v2 / v3, they are corresponding to legacy variants extracted from old version of this repo.
    The variant can be configured dynamically in HA directly, device 'Configuration' section, "Encoding".

- **forced_id** (Optional, Default: 0): the unique identifier of a remote or a phone, 2 to 4 bytes
    For ZhiJia, default to 0xC630B8 which was the value hard-coded in ble_adv_light component. Max 0xFFFFFF.
    For FanLamp and other: default to 0, uses the hash id computed by esphome from the id/name of the controller if not provided.

- **index** (Optional, Default: 0): the index, a supplementary identifier used by Phone Apps to distinguish in between the controlled device.

- **max_duration** (Optional, Default 1000, range 300 -> 10000): the maximum duration in ms during which the command is advertized. If a command is received before the 'max_duration' but after the 'duration', it is processed immediately. Increasing this parameter will have no major consequences, the component will just keep advertize the command, still this may slower the response time in case of multiple controllers used at the same time. Only interesting at pairing time to have the pairing command advertized for a long time.

- **duration** (Optional, Default 200, range 100 -> 500): the MINIMUM duration in ms during which the command is sent. It corresponds to the maximum time the controlled device is taking to process a command and be ready to receive a new one. If a command is received before the 'duration' it is queued and processed later, if there is already a similar command pending, in this case the pending command is removed from the queue. Increasing this parameter will make the combination of commands slower. Can be configured dynamically in HA directly, device 'Configuration' section, "Duration", See 'Dynamic Configuration'.

- **reversed** (Optional, Default: False) reversing the cold / warm at encoding time, needed for some controllers but honestly a non-sense as this is not what the phone apps are generating......

- **show_config** (Optional, Default:true): DEPRECATED. Deactivate the configuration Entities directly in HA if you want to hide them.

- **cancel_timer_on_any_change** (Optional, Default: True). When True, any change on any entity cancels any previously setup timer. This behavior must be aligned on the one of the controlled device in order to keep it in sync. If this is not the behavior of the controlled device, it is possible to put this option to false and setup automations on each trigger that would cancel the timer by calling action `cancel_timer`, see [Timer reset](#timer-reset).

## Actions
- **pair**:
  - Description: Pairs the configured controller on the controlled device by issuing a PAIR command. It is needed in case you could not setup the controller listening to the traffic with [ble_adv_handler](../ble_adv_handler/README.md)
  - Options:
    - **id**: the controller id
- **unpair**:
  - Description: TRIES to unpair the configured controller from the controlled device by issuing an UNPAIR command. It may work or not, depending on the device and the knowledge of the controller protocol.
  - Options:
    - **id**: the controller id
- **all_on**:
  - Description: equivalent to turn on all fan / lights associated to this controller.
  - Options:
    - **id**: the controller id
- **all_off**:
  - Description: equivalent to turn off all fan / lights associated to this controller.
  - Options:
    - **id**: the controller id
- **cancel_timer**:
  - Description: Cancels the timer setup with 1H/2H/... features of the Phone Apps and Remotes. As all devices work in a different way, this action could be called when a turn_off action on the fan or light is issued (if this is what your device is doing)
  - Options:
    - **id**: the controller id
- **raw_inject**:
  - Description: Injects a raw command. Such raw commands can be listen with whatever tool sniffing traffic, including the [ble_adv_handler](../ble_adv_handler/README.md). Can be useful in case you want to build a template entity such as a fan or a light while the encoding / decoding is not supported by those components.
  - Options:
    - **id**: the controller id
    - **raw**: the raw command to send, in hexa string format, supported formats:
        ```
        0201021B03F9084913F069254E3151BA32080A24CB3B7C71DC8BB89708D04C
        0x0201021B03F9084913F069254E3151BA32080A24CB3B7C71DC8BB89708D04C
        02 01 02 1B 03 F9 08 49 13 F0 69 25 4E 31 51 BA 32 08 0A 24 CB 3B 7C 71 DC 8B B8 97 08 D0 4C
        02.01.02.1B.03.F9.08.49.13.F0.69.25.4E.31.51.BA.32.08.0A.24.CB.3B.7C.71.DC.8B.B8.97.08.D0.4C
        02.01.02.1B.03.F9.08.49.13.F0.69.25.4E.31.51.BA.32.08.0A.24.CB.3B.7C.71.DC.8B.B8.97.08.D0.4C (31)
        ```
- **cmd**
  - Description: Injects a custom command directly with the langage of the codec used. 
  - Options:
    - **id**: the controller id
    - **cmd**: the 1 byte command code
    - **param**: the 1 byte param
    - **arg0, arg1, arg2**: the args
    
  The simplest way to know the cmd / param / args values to be entered is to copy a command generated by a physical remote / phone application. Just setup your ESP32 to listen to the traffic using [ble_adv_handler](../ble_adv_handler/README.md) while you do the action you want to copy, this will produce such logs:
  ```
  [13:59:59][D][remote - v3:261]: Decoded OK - tx: 39, gen: LIGHT_ON, enc: 0x10 - param1 0x00 - args[0,0]
  ```
  And you have the info: `enc: 0x10 - param1 0x00 - args[0,0]` !

## Automation Triggers

- **ble_adv_controller.on_emitted**

This trigger is activated each time a command is effectivelly emitted by the controller. An Instance of BleAdvGenCmd is given as 'g' parameter, and BleAdvEncCmd as 'e'.

```yaml
ble_adv_controller:
  - id: my_controller
    encoding: xxxx
    variant: xxxx
    on_emitted:
      - logger.log:
          format: "Emitted command: gen - %s, enc - %s"
          args: ["g.str().c_str()", "e.str().c_str()"]
```

## Call Actions from HA
All those actions can also be called as HA action directly with the following name and the same options (except the controller_id that is in the name instead of the options) using the [developers tools](https://www.home-assistant.io/docs/tools/dev-tools/):
```
esphome.<esp32_name>_<action>_<controller_id>
```

## Finding Configuration
The main parameters of the configuration of a ble_adv_controller (encoding/variant/forced_id/index) can be found using the [ble_adv_handler](../ble_adv_handler/README.md) to listen to the traffic generated by a phone app or a remote already paired to the device to be controlled.

## Example configuration: basic Cold / Warm White lamp using ZhiJia encoding v2 and Pair button

```yaml
ble_adv_controller:
  - id: my_controller
    encoding: zhijia
    variant: v2
    forced_id: xxxxxx
    index: 1

light:
  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: Kitchen Light
    type: cww

button:
  - platform: template
    name: Pair
    on_press:
      ble_adv_controller.pair: my_controller
```

## Example configuration: basic device using FanLamp Pro encoding v3, with Main CWW light, secondary Binary light and fan

```yaml
ble_adv_controller:
  - id: my_controller
    encoding: fanlamp_pro
    variant: v3
    forced_id: xxxxxx
    index: 1

light:
  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: Main Light
    type: cww
  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: Secondary Light
    type: onoff
    index: 1

fan:
  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: Fan
```

## Configuration for Cold / Warm White Light (Default if `type` not specified)

### Variables
- **ble_adv_controller_id** (Required), the id of the related ble_adv_controller
- **name** (Required), the name of the entity as it will appear in HA
- **type** (Optional, Default: 'cww') must be 'cww' for this light if specified
- **index** (Optional, Default: 0) defines the index of the entity: 0 for Main Light, 1 for Secondary, ...
- **secondary** (Optional) DEPRECATED, to be replaced by option `index: 1`
- **min_brightness** (Optional, Default: 2%) % minimum brightness supported by the light before it shuts done. Just setup this value to 0, then test your lamp by decreasing the brightness percent by percent, when it switches off, you have the min_brightness to setup here. Can be setup using Dynamic Configuration. To be set to 0 for 'Smart Lights' devices.
- **separate_dim_cct** (Optional, Default: False). DEPRECATED, see [replacement for flickering issue](#my-zhi-jia-lamp-flickers).

All variables of standard light are also available, see [ESPHome doc](https://esphome.io/components/light/). In particular you may be interested by `constant_brightness` or `default_transition_length`

## Configuration for Binary Light

### Variables
- **ble_adv_controller_id** (Required), the id of the related ble_adv_controller
- **name** (Required), the name of the entity as it will appear in HA
- **type** (Required) must be 'onoff' for this light
- **index** (Optional, Default: 0) defines the index of the entity: 0 for Main Light, 1 for Secondary, ...
- **secondary** (Optional) DEPRECATED, to be replaced by option `index: 1` and `type: onoff`

All variables of standard light are also available, see [ESPHome doc](https://esphome.io/components/light/).

## Configuration for RGB Light

### Variables
- **ble_adv_controller_id** (Required), the id of the related ble_adv_controller
- **name** (Required), the name of the entity as it will appear in HA
- **type** (Required) must be 'rgb' for this light
- **index** (Optional, Default: 0) defines the index of the entity: 0 for Main Light, 1 for Secondary, ...
- **secondary** (Optional) DEPRECATED, to be replaced by option `index: 1`
- **separate_dim_rgb** (Optional, Default: False). DEPRECATED.

All variables of standard light are also available, see [ESPHome doc](https://esphome.io/components/light/).

## Configuration for Fan

### Variables
- **ble_adv_controller_id** (Required), the id of the related ble_adv_controller
- **name** (Required), the name of the entity as it will appear in HA
- **index** (Optional, Default: 0) defines the index of the entity: 0 for Main Fan, 1 for Secondary (?one day?), ...
- **speed_count** (Optional, Default: 6) the number of Speed level available (0, 3 or 6)
- **use_direction** (Optional, Default: true) if the direction is available for the fan
- **use_oscillation** (Optional, Default: false) if the oscillation is available for the fan.
- **forced_refresh_on_start** (Optional, Default: false): forces the send of oscillation / direction at fan start. Some fans are resetting the direction / oscillation when the fan is stopped so when they are switched on, the direction / oscillation state in HA is no more in sync with the effective state of the device. If your fan automatically switches to reverse at each start when this option is `true` it means it considers the `direction` as a toggle and then does not supports this feature. Same for Oscillation. See the `fan.publish_state` action in this case.

All variables of standard fan are also available, see [ESPHome doc](https://esphome.io/components/fan/).

### Actions
- **fan.publish_state**:
  - Description: Updates the HA state with the given values without any call to the controlled device. Can be useful if your controlled device performs some automatic actions when turn off, such as reseting 'direction' or 'oscillation' and that you cannot use `forced_refresh_on_start`.
  - Options:
    - **id** (Required, ID): the ID of the fan. WARN, must be a fan with template ble_adv_controller else it will result in issues at runtime.
    - **state** (Optional, boolean, templatable): Set the state of the fan ON / OFF.
    - **speed** (Optional, int, templatable): Set the speed level of the fan. Can be a number between 1 and the maximum speed level of the fan.
    - **oscillating** (Optional, boolean, templatable): Set the oscillation state of the fan.
    - **direction** (Optional, string, templatable): Set the direction of the fan. Can be either `forward` or `reverse`.

## Configuration for Button
The ble_adv_button has been REMOVED as the features are now available as ble_adv_controller actions and can then be called with a standard [template button](https://esphome.io/components/button/template.html) and its 'on_press' trigger.

Example for PAIR Button:
```yaml
button:
  - platform: template
    name: Pair
    entity_category: config
    on_press:
      ble_adv_controller.pair: my_controller
```

## Good to know

### Dynamic configuration
It could be painful to find the correct variant or the correct duration by each time modifying the option in the yaml configuration of esphome. In order to help a dynamic configuration is available in Home Assistant 'Configuration' part of the esphome device:

![choice encoding](../../doc/images/Choice_encoding.jpg)

* `Variant` is customizable in the encoding selection part, the idea is to do the following:
  * Listen to the traffic generated by your Phone App and find all the config it generates (2 or 3)
  * Setup your config with the higher variant, and select the 'All' variant in the dynamic config in HA
  * Once done you can test to switch ON / OFF the main light to check if the config is OK
  * Then you can try the variants one by one and switch ON / OFF to find the exact variant used by your lamp

* `Duration` is customizable, the lowest the better: it makes the device answer faster. It is recommended to try to switch very fast ON/OFF the main light several times: if you end up with wrong state (light ON whereas HA state is OFF, or the reverse) it means the duration is too low and needs to be increased.

Once you managed to define the relevant values (without the need to re flash each time!), you can save the values in the yaml config.

### Setup without pairing
If you already have a phone app or a remote already paired with the device to be controlled, then it means there is already an existing `identifier` (and possibly `index`) setup on the control device, so you just need to find it and setup your controller accordingly (`forced_id` and `index`).

Check the [ble_adv_handler doc](../ble_adv_handler/README.md) to know how to capture and log those parameters.

If you already captured some traffic with app such as `nRF Connect` or `wireshark`, you can inject them directly using the [raw_inject Action](#actions), that can be [called directly from HA](#call-actions-from-ha).

This setup have several advantages:
* To have the ESP32 immediately able to communicate with your device without the need to Pair, as the device thinks it is your already paired Phone App communicating with it.
* To allow the Phone App to continue controlling the device, whereas pairing the ESP32 usually automatically unpairs the phone App.
* To have the ESP32 automatically listen to the commands sent by the Phone App and shadow them to HA in order to keep HA entity states always up-to-date!

### Pairing the ESP32 with the device ###
This is not the recommended method as for most of the devices it means the phone App will be unpaired automatically.
Still it is possible by calling the action `ESPHome: <my_device_name>_pair_<my_remote_id>` from the [HA developer dashboard](https://www.home-assistant.io/docs/tools/dev-tools/) directly, or by setting a [template button](https://esphome.io/components/button/template.html) that will call `ble_adv_controller.pair` on press (see above).

The procedure is then (usually, but refer to the device pairing procedure) the following:
1. Disconnect and reconnect your device from its power supply.
2. Within the 5s after power supply reconnection, press the "Pair" button or call the action from HA.
3. The device will (usually) either flash or ring to indicate the pairing process is successful.

### Reverse Cold / Warm
If this component works, but the cold and warm temperatures are reversed (that is, setting the temperature in Home Assistant to warm results in cold/blue light, and setting it to cold results in warm/yellow light), add a `reversed: true` line to your `ble_adv_controller` config.

### My Zhi Jia lamp flickers
Well [shit happens](https://github.com/aronsky/esphome-components/issues/18)... You can try to control your lamp in a different way using the zhijia 'v2_fl' variant designed as an attempt to solve this (as a replacement for the `separate_dim_cct` option).

### Minimum Brightness
If the minimum brightness is too bright, and you know that your light can go darker - try changing the minimum brightness via the `min_brightness` configuration option (it takes a percentage) or directly via the dynamic configuration in HA `Min Brightness`.

### Saving state on ESP32 reboot
Fan and Light entities are inheriting properties from their ESPHome parent [Fan](https://esphome.io/components/fan/index.html) and [Light](https://esphome.io/components/light/index.html), in particular they implement the `restore_mode` which has default value `ALWAYS_OFF` on ESPHome base lights and fans.

The default has been forced to value `RESTORE_DEFAULT_OFF` on the fan and light entities of this component so that they could remember their last state (ON/OFF, but also brightness, color temperature and fan speed). You can still modify this value if it is not OK for you.

### Action on turn on/off
Some devices perform some automatic actions when the main light or the fan are switched off, as for instance switch off the secondary light, or reset the Fan Direction or Oscillation status.
In order to update the state the same way in Home Assistant, simply add an [automation](https://esphome.io/components/light/index.html#light-on-turn-on-off-trigger) in your config, for instance:
* Switch Off the secondary light at the same time than the main light:
```yaml
light:
  - platform: ble_adv_controller
      ble_adv_controller_id: my_controller
      name: Main Light
      on_turn_off:
        then:
          light.turn_off: secondary_light
    - platform: ble_adv_controller
      ble_adv_controller_id: my_controller
      id: secondary_light
      name: Secondary Light
      index: 1
```
* Reset Fan Direction and Oscillation at Fan turn_on:
```yaml
fan:
  - platform: ble_adv_controller
      ble_adv_controller_id: my_controller
      id: my_fan
      name: My Fan
      on_turn_on:
        then:
          fan.turn_on:
            id: my_fan
            direction: forward
            oscillating: false
```
This triggers a second ON message, but also the proper state of direction and oscillating if they are reset by the device at turn off.

* Reset Fan Direction and Oscillation (HA state only) at Fan turn_off:
```yaml
fan:
  - platform: ble_adv_controller
      ble_adv_controller_id: my_controller
      id: my_fan
      name: My Fan
      on_turn_off:
        then:
          ble_adv_controller.fan.publish_state:
            id: my_fan
            direction: forward
            oscillating: false
```

### Timer reset
If using the timer feature, you have to be aware that the controlled device receives a 'Timer setup' command and after that it handles everything on its own, including:
- when it will switch OFF the entities: it may not be super precise
- when it cancels the timer: it decides which commands cancel the timer

On our component side, when a Timer setup feature is received (listening to a remote / phone App), a timer is also setup in parallel of the one on the controlled device in order to perform the same actions:
- switch OFF the entities when the timer expires, but it may not be exactly in sync with the controlled device
- cancel the timer when needed, and this needs to be setup in the same way than on the controlled device

By default, this component cancels the timer on each change with option `cancel_timer_on_any_change` setup to `true` by default. If this is not what your controlled device does, you can setup the option to `false` and call the action `ble_adv_controller.cancel_timer` when any relevant action is done on the entity, for example light is turned off:

```yaml
light:
  - platform: ble_adv_controller
      ble_adv_controller_id: my_controller
      name: Main Light
      on_turn_off:
        then:
          ble_adv_controller.cancel_timer: my_controller
```

### Light Transition
Esphome is providing features to handle 'smooth' transitions. While they are not very well supported by this component due to the BLE ADV technilogy used, they can still help reproduce the app phone behavior in such case.

For instance, the Zhi Jia app is always sending at least 2 messages when the brightness or color temperature is updated and this can be achieved the same way by setting the light property 'default_transition_length' to the same value than 'duration', as per default 200ms. (NOT TESTED but may work and solve flickering issues)

### Warning in logs
You can have the following warnings in logs:
```
[16:08:56][W][component:237]: Component 'xxxx' took a long time for an operation (56 ms).
[16:08:56][W][component:238]: Components should block for at most 30 ms.
```
This is not an issue, it just means ESPHome considered it spent too much time in our component and that it should not be the case. It has no real impact but it means the Api / Wifi / BLE may not work properly or work slowly.
In fact this is mostly due to ESPHome itself as 99% of the time spent in our component is due to the logs... Each line of log needs 10ms to be processed ............... So 5 lines of log during a transaction and we are over the limit... A [PR in ESPHome](https://github.com/esphome/esphome/pull/5373) is pending to solve this.

In order to avoid this, once you have finalized your config and all is working OK, I recommend to [setup the log level to INFO](https://esphome.io/components/logger.html) instead of DEBUG (which is the default).

### No encoder is working, help !!!!!
Two different cases here:
* You have successfully paired your device with one of the referenced app at the top of this guide, but you cannot pair the controller you setup whereas you followed this guide. This is not normal, open an Issue on this git repo specifying your full config (anonymized), the phone App to whcich it is paired, the steps you followed and the corresponding DEBUG logs.
* Your device is not working with any of the phone app referenced (another one or a remote?), but you want to have it work with HA! Try to [capture the advertising messages](../ble_adv_handler/README.md) generated by your controlling app or remote.
  * If nothing is captured, your device is not controlled by BLE advertising, and we cannot do anything for you.
  * If something is captured and a config is extracted, then all is OK, just use the captured config!
  * If something is captured but no config is extracted but your are in a hurry, you can still build a HA Template light from the captured messages, using the [raw_inject Action](#actions), that can be [called directly from HA](#call-actions-from-ha). It may or may not work as usually the devices remember the last messages and discard copies...
  * If something is captured but no config is extracted and you are not in a hurry, and you manage to control your device from another phone app, then open an Issue to have your phone app integrated to this component!

## Known Limitations
The technical solution implemented by manufacturers to control those devices is `BLE Advertising` and it comes with limitations:
* Each command needs to be maintained for a given minimum `duration` which is customizable by configuration but has drawbacks:
  * If the value is too small, the targetted device may not receive it and then not process the command
  * If the value is too high, each command is queued one after the other and then sending commands at a high rate will make delay more and more the commands.
  * The use of ESPHome light `transitions` is not recommended (and deactivated by default) as it generates high command rate. A mitigation has been implemented in order to remove commands of the same type from the processing queue when a new one is received, it seriously improves the behavior of the component but it is still not perfect.
* Some commands are the same for ON and OFF, working as a Toggle in fact. Sending high rate commands will cause the mix of ON and OFF commands and result in flickering and desynchronization of states.

## Duration(s)
The setting of the multiple duration parameters is important and should respect rules:
* The `seq_duration` should be significantly higher than the ESP BLE minimum interval (20ms). If setup to 30ms, the message will be advertised twice during this duration. If setup to 150ms it will be advertised 8 times. 
  * It corresponds to the maximum time needed by the device to receive (but not process) a command. 
  * Recommended value is 50ms.
  * If the device controlled is far from the ESP32 controller, there could be garbage on the line and this seq_duration would need to be increased.
  * If the seq_duration is too low your device will go in panic mode so be careful
* The `duration` should be higher than `seq_duration`, and should correspond to the time the controlled device is taking to effectively process a command and be ready to process the next one.
* The `max_duration` should be significantly higher than `duration`. It correspond to the maximum time the command is emitted in case no other command is coming for this controller. It is mostly useful for Pairing commands, recommended is 1000 (1s).

