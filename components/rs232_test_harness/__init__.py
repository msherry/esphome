import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import uart, text_sensor, binary_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]

rs232_ns = cg.esphome_ns.namespace("rs232_test_harness")

RS232TestHarness = rs232_ns.class_(
    "RS232TestHarness",
    cg.Component,
    uart.UARTDevice,
)

CONF_RESPONSE_SENSOR = "response_sensor"
CONF_DATA_AVAILABLE_SENSOR = "data_available_sensor"

CONFIG_SCHEMA = uart.UART_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(RS232TestHarness),
        cv.Optional(CONF_RESPONSE_SENSOR): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_DATA_AVAILABLE_SENSOR): cv.use_id(binary_sensor.BinarySensor),
    }
)

async def to_code(config):
    uart_component = await cg.get_variable(config[uart.CONF_UART_ID])

    var = cg.new_Pvariable(config[CONF_ID], uart_component)

    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_RESPONSE_SENSOR in config:
        sens = await cg.get_variable(config[CONF_RESPONSE_SENSOR])
        cg.add(var.set_response_sensor(sens))

    if CONF_DATA_AVAILABLE_SENSOR in config:
        sens = await cg.get_variable(config[CONF_DATA_AVAILABLE_SENSOR])
        cg.add(var.set_data_available_sensor(sens))
