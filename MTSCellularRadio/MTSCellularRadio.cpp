/* MTSCellularRadio definition
 *
 */

#include "MTSCellularRadio.h"
#include "MTSLog.h"

MTSCellularRadio::MTSCellularRadio(PinName tx, PinName rx/*, PinName cts, PinName rts,
    PinName dcd, PinName dsr, PinName dtr, PinName ri, PinName power, PinName reset*/)
    : _serial(tx, rx, 1024), _parser(_serial)
{

    echoMode = true;
    gpsEnabled = false;
    pppConnected = false;
    //socketMode = TCP;
    socketOpened = false;
    socketCloseable = true;
    
	radio_cts = NULL;
	radio_rts = NULL;
    radio_dcd = NULL;
    radio_dsr = NULL;
    radio_ri = NULL;
    radio_dtr = NULL;
    resetLine = NULL;

    _serial.baud(115200);

//    _parser.debugOn(1);

    // setup the battery circuit?
        //
        
    // wait for radio to get into a good state
    while (true) {
        if (sendBasicCommand("AT\r\n") == MTS_SUCCESS){
            logInfo("radio replied\r\n");
            break;
        } else {
            logInfo("waiting on radio...\r\n");
        }
        wait(1);
    }

    // identify the radio "ATI4" gets us the model (HE910, DE910, etc)
    const char command[] = "ATI4";
    char response[32];
    memset(response, 0, sizeof(response));
    char mNumber[16];
    memset(mNumber, 0, sizeof(mNumber));
    
    type = MTSCellularRadio::NA;
    while (true) {
        sendCommand(command, sizeof(command), response, sizeof(response), 1000);
        if (strstr(response,"HE910")) {
            type = MTSCellularRadio::MTSMC_H5;
            strcpy(mNumber, "HE910");
        } else if (strstr(response,"DE910")) {
            type = MTSCellularRadio::MTSMC_EV3;
            strcpy(mNumber, "DE910");
        } else if (strstr(response,"CE910")) {
            type = MTSCellularRadio::MTSMC_C2;
            strcpy(mNumber, "CE910");
        } else if (strstr(response,"GE910")) {
            type = MTSCellularRadio::MTSMC_G3;
            strcpy(mNumber, "GE910");
        } else if (strstr(response,"LE910-NAG")) {
            type = MTSCellularRadio::MTSMC_LAT1;
            strcpy(mNumber, "LE910-NAG");
        } else if (strstr(response,"LE910-SVG")) {
            type = MTSCellularRadio::MTSMC_LVW2;
            cid = 3;
            strcpy(mNumber, "LE910-SVG");
        } else if (strstr(response,"LE910-EUG")) {
            type = MTSCellularRadio::MTSMC_LEU1;
            strcpy(mNumber, "LE910-EUG");
        } else {
            logInfo("Determining radio type");
        }
        if (type != MTSCellularRadio::NA) {
            logInfo("radio model: %s", mNumber);
            break;
        }
        wait(1);
    }
}


//bool power(Power option);
    
/** Sets up the physical connection pins
*   (CTS, RTS, DCD, DTR, DSR, RI and RESET)
*/
/*bool configureSignals(unsigned int CTS = NC, unsigned int RTS = NC, unsigned int DCD = NC, unsigned int DTR = NC,
    unsigned int RESET = NC, unsigned int DSR = NC, unsigned int RI = NC);

Code test(){
    return MTS_SUCCESS;
}
*/

int MTSCellularRadio::getSignalStrength(){
    const char command[] = "AT+CSQ";
    char response[64];
    memset(response, 0, sizeof(response));
    sendCommand(command, sizeof(command), response, sizeof(response), 2000);
    if (!strstr(response, "OK")) {
        return -1;
    }
    char * ptr;
    ptr = strstr(response, "+CSQ:");
        
    int rssi, ber;
    sscanf(ptr, "+CSQ: %d,%d", &rssi, &ber);
    return rssi;
}


uint8_t MTSCellularRadio::getRegistration(){
    const char command[] = "AT+CREG?";
    char response[64];
    memset(response, 0, sizeof(response));
    sendCommand(command, sizeof(command), response, sizeof(response), 2000);
    char value = *(strchr(response, ',')+1);

    switch (value) {
        case '0':
            return NOT_REGISTERED;
        case '1':
            return REGISTERED;
        case '2':
            return SEARCHING;
        case '3':
            return DENIED;
        case '4':
            return UNKNOWN;
        case '5':
            return ROAMING;
    }
    return UNKNOWN;

}


int MTSCellularRadio::pdpContext(const char* apn){
    char command[64];
    snprintf(command, sizeof(command), "AT+CGDCONT=%d,\"IP\",%s", cid, apn);
    if (sendBasicCommand(command) == MTS_SUCCESS){
        return NSAPI_ERROR_OK;
    }
    return NSAPI_ERROR_PARAMETER;
}
 /*
Code setDns(const std::string& primary, const std::string& secondary){
    return MTS_SUCCESS;
}
*/


uint8_t MTSCellularRadio::sendBasicCommand(const char *command)
{
/*    if(socketOpened) {
        logError("socket is open. Can not send AT commands");
        return MTS_ERROR;
    }
*/
    _parser.setTimeout(100);
    _parser.flush();

    if (_parser.send(command) && _parser.recv("OK")) {
        return MTS_SUCCESS;
    }
    return MTS_FAILURE;
}


uint8_t MTSCellularRadio::sendCommand(const char *command, int command_size, char* response, int response_size,
    unsigned int timeoutMillis, char esc){
    
/*    if(io == NULL) {
            logError("MTSBufferedIO not set");
            return "";
        }
        if(socketOpened) {
            logError("socket is open. Can not send AT commands");
            return "";
        }
    
        io->rxClear();
        io->txClear();
*/
    _parser.setTimeout(100);
    _parser.flush();

    logInfo("command= %s", command);

    if (!_parser.send("%s", command)) {
        logError("failed to send command <%s> to radio\r\n", command);
        return MTS_FAILURE;    
    }
    if (esc != 0x00) {
        if (!_parser.send("%c", esc)) {
            logError("failed to send character '%c' (0x%02X) to radio\r\n", esc, esc);
            return MTS_FAILURE;
        }
    }

    Timer tmr;
    tmr.start();
    int count = 0;
    int c;
    while(tmr.read_ms() < timeoutMillis) {
        c = _parser.getc();
        if (c > -1) {
            response[count++] = (char)c;
            if ((strstr(response , "\r\nOK\r\n")) || (strstr(response , "\r\nERROR\r\n"))){
    logInfo("response= %s", response);
                return count;
            }
        }
        if (count >= response_size) {
            logWarning("%s response exceeds response size [%d]", command, response_size);
            wait(1);
            break;
        }
    }
    logInfo("response= %s", response);
    if (count > 0) {
        return count;
    }
    return MTS_FAILURE;
    
}

/*
static std::string MTSCellularRadio::getRegistrationNames(Registration registration){
    std::string result;
    return result;
}

static std::string MTSCellularRadio::getRadioNames(Radio radio){
    std::string result;
    return result;
}

Code MTSCellularRadio::echo(bool state){
    return MTS_SUCCESS;
}

std::string MTSCellularRadio::getDeviceIP(){
    std::string result;
    return result;
}

std::string MTSCellularRadio::getEquipmentIdentifier(){
    std::string result;
    return result;
}

std::string MTSCellularRadio::getRadioType(){
    std::string result;
    return result;
}
*/
int MTSCellularRadio::connect(){
    //Check if APN is not set, if it is not, connect will not work.
    if (type == MTSMC_H5_IP || type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_LAT1 || type == MTSMC_LEU1) {
        if(sizeof(apn) == 0) {
            logDebug("APN is not set");
            return NSAPI_ERROR_PARAMETER;
        }
    }

    //Check if already connected
    if(isConnected()) {
        return NSAPI_ERROR_OK;
    }
    
    Timer tmr;
    //Check Registration: AT+CREG? == 0,1
    tmr.start();
    do {
        Registration registration = (Registration)getRegistration();
        if (registration == REGISTERED || registration == ROAMING) {
            break;
        } else {
            logTrace("Not Registered [%d] ... waiting", (int)registration);
            wait(1);
        }
        if (tmr.read() > 30) {
            return NSAPI_ERROR_AUTH_FAILURE;
        }
    } while(1); 
    
    //Check RSSI: AT+CSQ
    tmr.reset();
    do {
        int rssi = getSignalStrength();
        logDebug("Signal strength: %d", rssi);
        if (rssi < 32 && rssi > 0) {
            break;            
        } else {
            logTrace("No Signal ... waiting");
            wait(1);
        }
        if (tmr.read() > 30) {
            return NSAPI_ERROR_AUTH_FAILURE;
        }        
    } while(1);

    //Make PPP connection
    if (type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_LAT1 || type == MTSMC_LEU1) {
        logDebug("Making PPP Connection Attempt. APN[%s]", apn);
    } else {
        logDebug("Making PPP Connection Attempt");
    }
    //Attempt context activation. Example successful response #SGACT: 50.28.201.151.
    char command[16];
    memset(command, 0, sizeof(command));
    char response[64];
    memset(response, 0, sizeof(response));
    snprintf(command, sizeof(command), "AT#SGACT=%d,1", type == MTSMC_LVW2 ? 3 : 1);
    sendCommand(command, sizeof(command), response, sizeof(response), 10000);
    if (!strstr(response, "OK")) {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    char * ptr;
    ptr = strstr(response, "#SGACT:");
    if (ptr == NULL) {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    char ipAddr[16];
    sscanf(ptr, "#SGACT: %s", ipAddr);

    logInfo("PPP Connection Established: IP[%s]", ipAddr);
    return NSAPI_ERROR_OK;
}

int MTSCellularRadio::disconnect(){
    logInfo("disconnecting radio");
    return NSAPI_ERROR_OK;
}

bool MTSCellularRadio::isConnected(){
    const char command[] = "AT#SGACT?";
    char response[128];
    memset(response, 0, sizeof(response));
    char buf[8];
    memset(buf, 0, sizeof(buf));
    
    sendCommand(command, sizeof(command), response, sizeof(response), 1000);
    snprintf(buf, sizeof(buf), "%d,1", type == MTSMC_LVW2 ? 3 : 1);
    if (strstr(response, buf)) {
        return true;
    } else {
        return false;
    }
}

const char *MTSCellularRadio::getIPAddress(void)
{
    return 0;
}
/*    
const char *MTSCellularRadio::getMACAddress(void){
    return 0;
}

    
const char *MTSCellularRadio::getGateway()
{
    return 0;
}
    
const char *MTSCellularRadio::getNetmask()
{
    return 0;
}
*/

bool MTSCellularRadio::open(const char *type, int id, const char* addr, int port)
{
    // TODO: check if the socket is already open before trying to open it.
    if(id > CELLULAR_SOCKET_COUNT - 1) {
        return false;
    }

    char command[64];
    memset(command, 0, sizeof(command));
    char response[64];
    memset(response, 0, sizeof(response));
    
    if (type == "TCP") {
        snprintf(command, sizeof(command), "AT#SD=%d,0,%d,\"%s\",0,0,1", id, port, addr);
    } else {
        snprintf(command, sizeof(command), "AT#SD=%d,1,%d,\"%s\",0,0,1", id, port, addr);
    }
    if (sendCommand(command, sizeof(command), response, sizeof(response), 65000)) {
        if (!strstr(response, "OK")) {
            logInfo("open failed");
            return false;
        }
    }
    logInfo("open success");
    return true;
}


bool MTSCellularRadio::close(int id)
{
    // TODO: check if the socket is open before trying to close it.
    // #SS
    
    char command[16];
    memset(command, 0, sizeof(command));
    
    snprintf(command, sizeof(command), "AT+SH=%d", id);
    if (sendBasicCommand(command) == MTS_SUCCESS) {
        return true;
    }
    return false;
}

/*
bool ping(const std::string& address){
    return true;
}

Code sendSMS(const std::string& phoneNumber, const std::string& message){
    return MTS_SUCCESS;
}


Code sendSMS(const Sms& sms){
    return MTS_SUCCESS;
}

std::vector<Sms> getReceivedSms(){
    std::vector<Sms> sms;
    return sms;
}

Code deleteAllReceivedSms(){
    return MTS_SUCCESS;
}

Code deleteOnlyReceivedReadSms(){
    return MTS_SUCCESS;
}

bool GPSenable(){
    return true;
}

bool GPSdisable(){
    return true;
}

bool GPSenabled(){
    return true;
}

gpsData GPSgetPosition(){
    gpsData data;
    return data;
}

bool GPSgotFix(){
    return true;
}
*/
