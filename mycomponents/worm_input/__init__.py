import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import canbus as canbus_module
from esphome.const import (
    CONF_ALLOW_OTHER_USES,
    CONF_ID,
    CONF_INPUT,
    CONF_INVERTED,
    CONF_MODE,
    CONF_NUMBER,
)

DEPENDENCIES = ["canbus"]

worm_input_ns = cg.esphome_ns.namespace("worm_input")

WormInputComponent = worm_input_ns.class_("WormInputComponent", cg.Component)
WormInputGPIOPin = worm_input_ns.class_(
    "WormInputGPIOPin", cg.GPIOPin, cg.Parented.template(WormInputComponent)
)

CONF_WORM_INPUT = "worm_input"
CONF_ADDR = "addr"

# Validate address as 4-digit binary string like "0000", "0001", ..., "1111"
def validate_addr(value):
    if isinstance(value, str):
        if len(value) != 4 or not all(c in "01" for c in value):
            raise cv.Invalid('Address must be a 4-digit binary string (e.g., "0000", "0001")')
        return int(value, 2)
    raise cv.Invalid('Address must be a 4-digit binary string (e.g., "0000", "0001")')

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.declare_id(WormInputComponent),
        cv.Required("canbus"): cv.use_id(canbus_module.CanbusComponent),
    }
).extend(cv.COMPONENT_SCHEMA)

FINAL_VALIDATE_SCHEMA = None


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    canbus_var = await cg.get_variable(config["canbus"])
    cg.add(var.set_canbus(canbus_var))


def _validate_input_mode(value):
    if value.get(CONF_INPUT) is not True:
        raise cv.Invalid("Only input mode is supported")
    return value


# Transform user-facing (addr, number) to internal pin number
def _transform_pin_number(value):
    if not isinstance(value, dict):
        raise cv.Invalid("Expected a dictionary")
    
    addr = value.get(CONF_ADDR)
    number = value.get(CONF_NUMBER)
    
    if addr is None:
        raise cv.Invalid("'addr' is required")
    if number is None:
        raise cv.Invalid("'number' is required")
    
    # User-facing number is 1-32, internal is 0-31
    number = cv.int_range(min=1, max=32, max_included=True)(number)
    
    # Compute internal pin number for uniqueness
    # internal_pin = addr * 32 + (number - 1)
    internal_pin = addr * 32 + (number - 1)
    
    result = dict(value)
    result[CONF_ADDR] = addr
    result[CONF_NUMBER] = internal_pin  # Replace with internal pin for uniqueness tracking
    return result


# Base schema without number transformation
_WORM_INPUT_PIN_BASE = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WormInputGPIOPin),
        cv.Required(CONF_WORM_INPUT): cv.use_id(WormInputComponent),
        cv.Required(CONF_ADDR): validate_addr,
        cv.Required(CONF_NUMBER): cv.int_range(min=1, max=32, max_included=True),
        cv.Optional(CONF_ALLOW_OTHER_USES): cv.boolean,
        cv.Optional(CONF_MODE, default={CONF_INPUT: True}): cv.All(
            {cv.Optional(CONF_INPUT): cv.boolean}, _validate_input_mode
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)

# Full schema with transformation
WORM_INPUT_PIN_SCHEMA = cv.All(_WORM_INPUT_PIN_BASE, _transform_pin_number)


def worm_input_pin_final_validate(pin_config, parent_config):
    pass


@pins.PIN_SCHEMA_REGISTRY.register(
    CONF_WORM_INPUT, WORM_INPUT_PIN_SCHEMA, worm_input_pin_final_validate
)
async def worm_input_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_parented(var, config[CONF_WORM_INPUT])

    # config[CONF_NUMBER] is now the internal pin (addr*32 + (number-1))
    internal_pin = config[CONF_NUMBER]
    addr = internal_pin // 32
    input_pin = internal_pin % 32

    cg.add(var.set_addr(addr))
    cg.add(var.set_input_pin(input_pin))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    return var
