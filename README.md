The MTSCellularInterface library implements the mbed-os CellularInterface for the cellular radios on MultiTech Dragonfly devices.

MTSCellularInterface provides TCP and UDP sockets, SMS messaging capabilities, and GPS location\*.

To get started with MTSCellularInterface, you just need a pointer to the library.
  * Most cellular radios require an APN string to be set before the radio can be used for SMS, TCP, etc.
  * The radio will not usually be able to connect until it is registered with the network.

```c++
#include "MTSCellularInterface.h"

int main() {
    PinName radio_tx = RADIO_TX;
    PinName radio_rx = RADIO_RX;

    MTSCellularInterface* radio = new MTSCellularInterface();
    assert(radio);

    const char apn[] = "<your APN>";

    // wait for registration
    while(!radio->is_registered()){
        wait(2);
    }

    // connect
    if (radio->connect(apn)) != NSAPI_ERROR_OK) {
        // connection failed - handle the failure
    }

    // connection succeeded
    // SMS, socket, etc functions should be available
}
```

The [Dragonfly-Examples](http://developer.mbed.org/teams/MultiTech/code/Dragonfly-Examples) repository demonstrates how to use common MTSCellularInterface features.

The [MultiTech Dragonfly](http://developer.mbed.org/platforms/MTS-Dragonfly) mbed platform page has features, pinout information, and other documentation and resources.

\* Not all radios support GPS. GPS location requires a lock on GPS satellites, which may not be possible indoors.
