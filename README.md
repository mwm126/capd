# capd service

The `capd` service listen for CAP packets, and authenticates them to determine
whether access through the firewall should be permitted.

# Build
```
meson build
meson compile -C build
```

# Run
```
./build/capd
```
