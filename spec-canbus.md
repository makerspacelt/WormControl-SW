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
* 001 hello (relay/input module -> gw)
* 010 input event (input module -> gw)
* 011 monitoring (relay/input module -> gw)

# CAN data

We have 0-64 bits for data.

# Messages

## Control (set relays) message: Gateway -> Relay module

```
prio|devtyp|msgtyp|addr|data
   1|   000|   000|xxxx|(16 bits, one bit for each relay)
```

Send:
* on boot
* immediately on change
* whenever the state reported by the relay module does not match gateway's state

## Relay module address & state message: Relay module -> Gateway

On boot:
```
prio|devtyp|msgtyp|addr|data
   1|   xxx|   001|xxxx|(16 bits, one bit for each relay)
``` 

Repeated every 10s and 1s after any relay state change:
```
prio|devtyp|msgtyp|addr|data
   0|   xxx|   001|xxxx|(16 bits, one bit for each relay)
```

## Input event message: Input module -> Gateway

```
prio|devtyp|msgtyp|addr|data
   1|   010|   010|xxxx|0000   0000  0
                        ^^^^   ^^^^  ^
                        inp.id event state
``` 

Send on boot, on any input change and repeat every 10s.

## Monitoring message: Any Module -> Gateway

TODO decide when monitoring messages are sent

```
prio|devtyp|msgtyp|addr|data
   0|   xxx|   011|xxxx|0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 
                        ^^^^^^^^^ ^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                        type      idx  value (float32)
```

Monitoring message types (8 bits):
* 0000 0000 - error
* 0000 0001 - temperature
* 0000 0010 - uptime
* 0000 0011 - voltage
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
* <...>
