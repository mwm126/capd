# capd service

The `capd` service listen for CAP packets, and authenticates them to determine
whether access through the firewall should be permitted.

# Build Requirements

 - GCC
 - OpenSSL

For instance, on Ubuntu: `apt install build-essential libssl-dev`

# Build
```
./build.sh
```

# Run
```
./build/capd
```

# Development

To run tests, build with Meson:

```
pip install meson ninja
meson build
meson compile -C build
meson test -C build
```
