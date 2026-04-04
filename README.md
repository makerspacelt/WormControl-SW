# Hardware

This is Worm Control. A modular home/lab automation and control solution.

The full system consists of three modules:
1. WormControl Relay - one module consists of 15 smart relays. The relays can switch AC/DC up to 2A each (not exceeding 15A in total).
2. WormControl Input - one module with 32 general purpose input channels like buttons, light switches or any device that connects two wires together.
3. WormControl Gateway - acts as a gateway between Relay/Input modules and your network. Inter-module communication uses the CANbus protocol. Network communication uses Ethernet.

It's called WormControl because the idea is to daisy-chain up to 16 Relay and up to 16 Input modules - like worm segments, connecting them all to a single Gateway.

All three devices are mountable on DIN rails in European electrical cabinets.

Both relay and input modules use a 4-bit DIP switch for hardware addressing, allowing up to 16 modules of each type:
* Relay modules: address 0-15, each with 15 relays (240 total relays possible per gateway)
* Input modules: address 0-15, each with 32 inputs (512 total inputs possible per gateway)

# Software

Communication between the gateway and relay/input modules is implemented using ESPHome custom components:
* `worm_relay` custom component for gw ←→ relay module communication
* `worm_input` custom component for gw ←→ input module communication

Example esphome yaml snippet showing how to use these components with the gateway module:

```yaml
external_components:
  - source:
      type: local
      path: mycomponents
 
canbus:
  - platform: esp32_can
    id: espcan
    tx_pin: GPIO1
    rx_pin: GPIO0
    can_id: 0
    bit_rate: 125kbps 

# Single worm_relay component manages all relay modules
# Each module has USR LED + 15 relays (gpio pin numbers 0-15)
worm_relay:
  id: wrl
  canbus: espcan

# Switches for controlling relays
switch:
  # - Here `M00_R01` can be any string, we use M00 for module zero and R01 for relay 1.
  # - Address addr: "0000" informs the worm_relay component about the module's address and must match its physical dip switch settings.
  # - Number 1 means relay number 1 ("R1" label on the module housing).
  - { platform: gpio, id: M00_R01, name: M00_R01, pin: { worm_relay: wrl, addr: "0000", number: 1 } }

# Single worm_input component manages all input modules
# Each module has 32 inputs (gpio pin numbers 1-32)
worm_input:
  id: win
  canbus: espcan

# Binary sensors for input modules
binary_sensor:
  # - Here `M00_IN01` can be any string, we use M00 for module zero and IN01 for input 01.
  # - Address addr: "0000" informs the worm_input component about the module's address and must match its physical dip switch settings.
  # - Number 1 means input number 1 ("01" label on the module housing).
  - { platform: gpio, id: M00_IN01, name: M00_IN01, pin: { worm_input: win, addr: "0000", number: 1 } }
```

# Inter-module CANbus communication specification

We use 11 bit can bus address:
| priority | dev type | msg type | dev address |
|----------|----------|----------|-------------|
|        0 |      000 |      000 |        0000 |

priority (1 bit):
* 0 - higher priority (per canbus spec)
* 1 - lower priority

dev types (3 bits):
* 000 gateway
* 001 relay module
* 010 input module

msg types (3 bits):
* 000 set relay (gw -> relay module)
* 001 hello & state (relay/input module -> gw)
* 010 monitoring (relay/input module -> gw)

dev address (4 bits) - the module's dip switch setting

## CAN data

We have 0-64 bits for data.

## Messages

### Control (set relays) message: Gateway -> Relay module

Control message format:
```
prio|devtyp|msgtyp|addr|data
   0|   000|   000|xxxx|(16 bits, one bit for each relay)
```

Send the control message from Gateway to the Relay module in these cases:
* Immediately, when gateway receives updated state from automation.
* Immediately, when gateway receives current state from relay module and it does not match the state known by gateway.
  State mismatch is ignored after boot until first automation (api/mqtt) connection.

### Relay module address & state message: Relay module -> Gateway

Relay module state message format:
```
prio|devtyp|msgtyp|addr|data
   0|   001|   001|xxxx|(16 bits, one bit for each relay)
``` 

Send the state message from the relay module to the gateway in these cases:
1. On boot with 1s delay.
2. Repeated every 10s.
3. Sent immediately after relay module address change.

### Input module address & state message: Input module -> Gateway

Input module state message:
```
prio|devtyp|msgtyp|addr|data
   0|   010|   001|xxxx|(32 bits, one bit for each input)
``` 

Send the state message from the input module to the gateway in these cases:
1. On boot with 1s delay.
2. Repeated every 10s.
3. Immediately after input module address change.
4. Immediately after input state change.

### Monitoring message: Any Module -> Gateway

Monitoring message format, sent from relay/input module to the gateway:
```
prio|devtyp|msgtyp|addr|data
   1|   xxx|   010|xxxx|0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 
                        ^^^^^^^^^ ^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                        type     idx  value (float32)
```

* `type` is the message type (see below)
* `idx` is the sensor index (usually 0000)
* `value` is the float32 value

Monitoring message types (8 bits):
* 0000 0000
* 0000 0001 - temperature - sent every ~4s
* 0000 0010 - uptime      - not implemented
* 0000 0011 - voltage     - sent every ~4s
* 0000 0100
* 0000 0101
* 0000 0110
* 0000 0111
* 0000 1000
* 0000 1001
* 0000 1010
* 0000 1011
* 0000 1100
* 0000 1101
* 0000 1110
* 0000 1111
