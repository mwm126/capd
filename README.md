# capd service

The `capd` service listen for CAP packets, and authenticates them to determine
whether access through the firewall should be permitted.

## Build Requirements

 - GCC
 - OpenSSL

For instance, on Ubuntu: `apt install build-essential libssl-dev`

## Build
```
./build.sh
```

## Run
```
./build/capd
```

## Development

To run tests, build with Meson:

```
pip install meson ninja
meson build
meson compile -C build
meson test -C build
```

# mfa_program tool for programming Yubikey


## Install dependencies
The `mfa_program.py` script requires `pyOpenSSL` to run.

```
pip install pyOpenSSL
```

To create certificate:

```
./mfa_program.py ca
```

To program a yubikey:

```
./mfa_program.py yk
```

## Development

The pytests require a Yubikey to run.

*DO NOT RUN THE TESTS ON A PRODUCTION YUBIKEY.*

This  Yubikey will be reset and wiped out.

To run tests:

```
pip install poetry
poetry install
poetry run python -m pytest --mypy
```
