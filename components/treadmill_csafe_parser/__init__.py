import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]

treadmill_csafe_parser_ns = cg.esphome_ns.namespace("treadmill_csafe_parser")

TreadmillCSAFEParser = treadmill_csafe_parser_ns.class_(
    "TreadmillCSAFEParser",
    cg.Component,
    uart.UARTDevice
)

CONF_UART_ID = "uart_id"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(TreadmillCSAFEParser),

    cv.Required(CONF_UART_ID):
        cv.use_id(uart.UARTComponent),

}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):

    uart_component = await cg.get_variable(config[CONF_UART_ID])

    var = cg.new_Pvariable(config[CONF_ID], uart_component)

    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
