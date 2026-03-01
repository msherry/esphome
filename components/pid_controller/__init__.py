import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.const import CONF_ID

DEPENDENCIES = []

pid_ns = cg.esphome_ns.namespace("pid_controller")

PIDController = pid_ns.class_("PIDController", cg.Component)

CONF_KP = "kp"
CONF_KI = "ki"
CONF_KD = "kd"
CONF_OUTPUT_MIN = "output_min"
CONF_OUTPUT_MAX = "output_max"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PIDController),

    cv.Required(CONF_KP): cv.float_,
    cv.Required(CONF_KI): cv.float_,
    cv.Required(CONF_KD): cv.float_,
    cv.Required(CONF_OUTPUT_MIN): cv.float_,
    cv.Required(CONF_OUTPUT_MAX): cv.float_,
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(
        config[CONF_ID],
        config[CONF_KP],
        config[CONF_KI],
        config[CONF_KD],
        config[CONF_OUTPUT_MIN],
        config[CONF_OUTPUT_MAX]
    )
    await cg.register_component(var, config)
