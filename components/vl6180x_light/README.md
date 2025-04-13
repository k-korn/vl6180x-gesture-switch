# WIP Component: VL6180x Gesture Based Light Switch

Basic code of dummy monochromatic light + ambient light sensor from VL6180X ToF distance + ALS device.
ToF ranging sensor to be used as gesture input.

```yaml
# example configuration:

light:
  - platform: vl6180x_light
    name: VL6180x light
    output: pwm_output
    max_range: 120
    min_range: 15
    ambient:
        name: "Ambient"
        unit_of_measurement: lx
        accuracy_decimals: 2

output:
  - platform: ledc
    pin: GPIO25
    id: pwm_output
```