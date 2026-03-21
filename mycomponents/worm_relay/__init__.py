import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import canbus as canbus_module
from esphome.const import (
    CONF_ALLOW_OTHER_USES,
    CONF_ID,
    CONF_INVERTED,
    CONF_MODE,
    CONF_NUMBER,
    CONF_OUTPUT,
)

DEPENDENCIES = ["canbus"]

worm_relay_ns = cg.esphome_ns.namespace("worm_relay")

WormRelayComponent = worm_relay_ns.class_("WormRelayComponent", cg.Component)
WormRelayGPIOPin = worm_relay_ns.class_(
    "WormRelayGPIOPin", cg.GPIOPin, cg.Parented.template(WormRelayComponent)
)

CONF_WORM_RELAY = "worm_relay"
CONF_ADDR = "addr"

# Validate address as 4-digit binary string like "0000", "0001", ..., "1111"
def validate_addr(value):
    if isinstance(value, int):
        return cv.int_range(min=0, max=15)(value)
    if isinstance(value, str):
        if len(value) != 4 or not all(c in "01" for c in value):
            raise cv.Invalid("Address must be a 4-digit binary string (e.g., '0000', '0001') or integer 0-15")
        return int(value, 2)
    raise cv.Invalid("Address must be a 4-digit binary string (e.g., '0000', '0001') or integer 0-15")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.declare_id(WormRelayComponent),
        cv.Required("canbus"): cv.use_id(canbus_module.CanbusComponent),
    }
).extend(cv.COMPONENT_SCHEMA)

FINAL_VALIDATE_SCHEMA = None


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    canbus_var = await cg.get_variable(config["canbus"])
    cg.add(var.set_canbus(canbus_var))


def _validate_output_mode(value):
    if value.get(CONF_OUTPUT) is not True:
        raise cv.Invalid("Only output mode is supported")
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
    
    # Validate
    addr = validate_addr(addr)
    number = cv.int_range(min=0, max=15, max_included=True)(number)
    
    # Compute internal pin number for uniqueness
    internal_pin = addr * 16 + number
    
    result = dict(value)
    result[CONF_ADDR] = addr
    result[CONF_NUMBER] = internal_pin  # Replace with internal pin for uniqueness tracking
    result['_relay_pin'] = number  # Store for code generation
    
    return result


# Base schema without number transformation
_WORM_RELAY_PIN_BASE = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WormRelayGPIOPin),
        cv.Required(CONF_WORM_RELAY): cv.use_id(WormRelayComponent),
        cv.Required(CONF_ADDR): validate_addr,
        cv.Required(CONF_NUMBER): cv.int_range(min=0, max=15, max_included=True),
        cv.Optional(CONF_ALLOW_OTHER_USES): cv.boolean,
        cv.Optional(CONF_MODE, default={CONF_OUTPUT: True}): cv.All(
            {cv.Optional(CONF_OUTPUT): cv.boolean}, _validate_output_mode
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)

# Full schema with transformation
WORM_RELAY_PIN_SCHEMA = cv.All(_WORM_RELAY_PIN_BASE, _transform_pin_number)


def worm_relay_pin_final_validate(pin_config, parent_config):
    pass


@pins.PIN_SCHEMA_REGISTRY.register(
    CONF_WORM_RELAY, WORM_RELAY_PIN_SCHEMA, worm_relay_pin_final_validate
)
async def worm_relay_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_parented(var, config[CONF_WORM_RELAY])

    # config[CONF_NUMBER] is now the internal pin (addr*16 + relay)
    internal_pin = config[CONF_NUMBER]
    addr = internal_pin // 16
    relay_pin = internal_pin % 16

    cg.add(var.set_pin(internal_pin))
    cg.add(var.set_addr(addr))
    cg.add(var.set_relay_pin(relay_pin))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    return var