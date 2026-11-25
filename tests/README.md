# Build checks

Run from the Buderus repository with Python 3.9+:

```sh
python3 -B -m unittest discover -s tests
```

The simulation check runs the actual version script in a temporary project.
It verifies generation without `web/output`, unchanged output timestamps on a
repeat run, version updates and preservation of the `web/sim.json` source and
other simulation values. It does not compile firmware or test a device.

For the WebUI generator regression checks, see the sibling EspWebUI repository.
