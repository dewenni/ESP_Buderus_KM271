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

Alarm memory decoding and state transitions (Node.js 18+, no packages):

```sh
node --test tests/alarms.test.cjs
```

These checks execute the actual WebUI decoder and cover open/cleared messages,
relative day offsets, unknown codes, incomplete reception, disabled reception,
serial/browser disconnects and cached slots after reconnect. They do not verify
serial hardware, the meaning of a controller acknowledgement, or browser layout.
The second timestamp still needs comparison with real fault clearing and manual
acknowledgement on the test controller. The WebUI currently labels it “Cleared”.

The Python host suite also invokes these Node.js checks when `node` is on PATH;
it reports a skipped test otherwise. Install Node.js 18+ for the full host suite.
