# Experimental FIO Ledger App

Experimental FIO Ledger App for Ledger Nano S

## Building

### Setup

The app can be compiled using a Docker container with all dependencies. A detailed guide on setting the
container up can be found in ledger-app-builder [repository](https://github.com/LedgerHQ/ledger-app-builder).

### Loading the app

`docker run --rm -ti -v "$(realpath .):/app" --privileged ledger-app-builder:latest`
`cd experimental-ledger-app/ledger-app-fio`
`make clean`
`make load`

Builds and loads the application into the connected device. Just make sure to close the Ledger app on the device before running the command.


### Debug version

In `Makefile`, uncomment

    #DEVEL = 1
    #DEFINES += HEADLESS

also comment out

    DEFINES += RESET_ON_CRASH

and then run `make clean load`.

### Troubleshooting connection problems

The quickstart guide's script sets up your udev rules, but there still might be problems.
- https://support.ledger.com/hc/en-us/articles/115005165269-Fix-connection-issues

## Deploying

The build process is managed with [Make](https://www.gnu.org/software/make/).

### Make Commands

* `load`: Load signed app onto the Ledger device
* `clean`: Clean the build and output directories
* `delete`: Remove the application from the device
* `build`: Build obj and bin api artefacts without loading
* `format`: Format source code.

See `Makefile` for list of included functions.
