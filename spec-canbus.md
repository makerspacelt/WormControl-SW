# CAN IDs

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

# CAN data

We have 0-64 bits for data.

# Messages

## Control (set relays) message: Gateway -> Relay module

```
prio|devtyp|msgtyp|addr|data
   0|   000|   000|xxxx|(16 bits, one bit for each relay)
```

Send:
* on boot
* immediately on change
* whenever the state reported by the relay module does not match gateway's state

## Relay module address & state message: Relay module -> Gateway

This message is sent:
1. On boot with 1s delay.
2. Repeated every 10s.
3. Sent immediatelly after relay module address change.

```
prio|devtyp|msgtyp|addr|data
   0|   010|   001|xxxx|(16 bits, one bit for each relay)
``` 

## Input module address & state message: Input module -> Gateway

This message is sent:
1. On boot with 1s delay.
2. Repeated every 10s.
3. Immediatelly after input module address change.
4. Immediatelly after input state change.

```
prio|devtyp|msgtyp|addr|data
   0|   010|   001|xxxx|(32 bits, one bit for each input)
``` 

## Monitoring message: Any Module -> Gateway

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
* 0000 0001 - temperature - sent every 3s
* 0000 0010
* 0000 0011 - voltage - sent every 3s
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
