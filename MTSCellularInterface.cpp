/* Cellular radio implementation of NetworkInterface API
*
*/

#include <string.h>
#include "MTSCellularInterface.h"

// Various timeouts for cellular radio operations
#define CELL_RADIO_CONNECT_TIMEOUT 15000
#define CELL_RADIO_SEND_TIMEOUT    500
#define CELL_RADIO_RECV_TIMEOUT    0
#define CELL_RADIO_MISC_TIMEOUT    500

// MTSCellularInterface implementation
MTSCellularInterface::MTSCellularInterface()
{
}
