- [ ] gateway: update `worm_relay` component to additionally send relay states as in spec-canbus.md:
      * on boot
      * whenever the state reported by the relay module does not match gateway's state
- [ ] gateway: maybe batch relay updates in custom component. investigate why all relay update from HA does not work reliably

- [ ] relay: locate feature using USR LED (`worm_relay` gpio pin number 0)
- [ ] input: locate feature using USR LED (special `worm_input` gpio, also needs implementation in wormcontrol-input.yaml and specification in spec-canbus.md)
- [ ] gateway: should we configure relay & input qos for mqtt or add a comment about it? maybe only in an alternative mqtt config that we will ship
- [ ] gateway: handle monitoring data coming from modules
- [ ] gateway: how does ota/password work when using HA `api:`?
