/* MTSCellularRadio definition
 *
 */

#include "MTSCellularRadio.h"
#include "MTSLog.h"

MTSCellularRadio::MTSCellularRadio(PinName tx, PinName rx/*, PinName cts, PinName rts,
    PinName dcd, PinName dsr, PinName dtr, PinName ri, PinName power, PinName reset*/)
    : _serial(tx, rx, 1024), _parser(_serial), _cid(1)
{

    _echoMode = true;
    _gpsEnabled = false;
    _pppConnected = false;
    //_socketMode = TCP;
    _socketOpened = false;
    _socketCloseable = true;
    
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
    memset(_command,0,_cmdBufSize);
    strcpy(_command, "ATI4");
    char mNumber[16];
    memset(mNumber, 0, sizeof(mNumber));
    
    _type = MTSCellularRadio::NA;
    while (true) {
        sendCommand(_command, strlen(_command), _response, _rspBufSize, 1000);
        if (strstr(_response,"HE910")) {
            _type = MTSCellularRadio::MTSMC_H5;
            strcpy(mNumber, "HE910");
        } else if (strstr(_response,"DE910")) {
            _type = MTSCellularRadio::MTSMC_EV3;
            strcpy(mNumber, "DE910");
        } else if (strstr(_response,"CE910")) {
            _type = MTSCellularRadio::MTSMC_C2;
            strcpy(mNumber, "CE910");
        } else if (strstr(_response,"GE910")) {
            _type = MTSCellularRadio::MTSMC_G3;
            strcpy(mNumber, "GE910");
        } else if (strstr(_response,"LE910-NAG")) {
            _type = MTSCellularRadio::MTSMC_LAT1;
            strcpy(mNumber, "LE910-NAG");
        } else if (strstr(_response,"LE910-SVG")) {
            _type = MTSCellularRadio::MTSMC_LVW2;
            _cid = 3;
            strcpy(mNumber, "LE910-SVG");
        } else if (strstr(_response,"LE910-EUG")) {
            _type = MTSCellularRadio::MTSMC_LEU1;
            strcpy(mNumber, "LE910-EUG");
        } else {
            logInfo("Determining radio type");
        }
        if (_type != MTSCellularRadio::NA) {
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
    memset(_command, 0, _cmdBufSize);
    strcpy(_command, "AT+CSQ");
    sendCommand(_command, _cmdBufSize, _response, _rspBufSize, 2000);

    char * ptr;
    ptr = strstr(_response, "+CSQ:");
    if (!ptr) {
        return -1;
    }        
    int rssi, ber;
    sscanf(ptr, "+CSQ: %d,%d", &rssi, &ber);
    return rssi;
}


int MTSCellularRadio::getRegistration(){
    memset(_command, 0, _cmdBufSize);
    strcpy(_command, "AT+CREG?");
    sendCommand(_command, _cmdBufSize, _response, _rspBufSize, 2000);

    char * ptr;
    ptr = strstr(_response, "REG:");
    if (!ptr) {
        return -1;
    }         
    int mode, registration;
    sscanf(ptr, "REG: %d,%d", &mode, &registration);
    return registration;
}


int MTSCellularRadio::pdpContext(const char* apn){
    strncpy(_apn, apn, strlen(apn));
    snprintf(_command, _cmdBufSize, "AT+CGDCONT=%d,\"IP\",%s", _cid, apn);
    if (sendBasicCommand(_command) == MTS_SUCCESS){
        return NSAPI_ERROR_OK;
    }
    return NSAPI_ERROR_PARAMETER;
}
 /*
Code setDns(const std::string& primary, const std::string& secondary){
    return MTS_SUCCESS;
}
*/


int MTSCellularRadio::sendBasicCommand(const char *command)
{
    _parser.setTimeout(200);
    _parser.flush();

    int result = sendCommand(command, sizeof(command), _response, _rspBufSize);
    if (result < 0) {
        return MTS_FAILURE;
    }
    if (strstr(_response, "\r\nOK\r\n")) {
        return MTS_SUCCESS;
    }
}

int MTSCellularRadio::sendCommand(const char *command, int command_size, char* response, int response_size,
    unsigned int timeoutMillis, char esc){

    memset(_response, 0, _rspBufSize);    // clear the member response buffer of any previous responses.
    memset(response, 0, response_size);   // clear the passed response buffer of any previous responses.
    
    _parser.setTimeout(200);
    _parser.flush();

    logInfo("command = %s", command);

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
                break;
            }
        }
        if (count >= response_size) {
            logWarning("%s response exceeds response size [%d]", command, response_size);
            break;
        }
    }
    tmr.stop();
    
    logInfo("response = %s", response);
    if (count > 0) {
        return count;
    }
    memset(_command, 0, _cmdBufSize);    // clear the command buffer of any previous commands.
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
    if (_type == MTSMC_H5_IP || _type == MTSMC_H5 || _type == MTSMC_G3 || _type == MTSMC_LAT1 || _type == MTSMC_LEU1) {
        if(sizeof(_apn) == 0) {
            logDebug("APN is not set");
            return NSAPI_ERROR_PARAMETER;
        }
    }

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
        }
        if (tmr.read() > 30) {
            tmr.stop();
            return NSAPI_ERROR_AUTH_FAILURE;
        }
        wait(1);
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
        }
        if (tmr.read() > 30) {
            tmr.stop();
            return NSAPI_ERROR_AUTH_FAILURE;
        }
        wait(1);
    } while(1);
    tmr.stop();

    //Make cellular connection
    if (_type == MTSMC_H5 || _type == MTSMC_G3 || _type == MTSMC_LAT1 || _type == MTSMC_LEU1) {
        logDebug("Making PPP Connection Attempt. APN[%s]", _apn);
    } else {
        logDebug("Making PPP Connection Attempt");
    }
    //Attempt context activation. Example successful response #SGACT: 50.28.201.151.
    snprintf(_command, _cmdBufSize, "AT#SGACT=%d,1", _type == MTSMC_LVW2 ? 3 : 1);
    sendCommand(_command, strlen(_command), _response, _rspBufSize, 10000);

    char * ptr;
    ptr = strstr(_response, "#SGACT:");
    if (!ptr) {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    char ipAddr[16];
    sscanf(ptr, "#SGACT: %s", ipAddr);
    memset(_ipAddress, 0, 16);
    strcpy(_ipAddress, ipAddr);

    logInfo("PPP Connection Established: IP[%s]", ipAddr);
    return NSAPI_ERROR_OK;
}

int MTSCellularRadio::disconnect(){
    logInfo("disconnecting");
    if(!isConnected()) {
        logInfo("already disconnected");
        return NSAPI_ERROR_OK;
    }

    // Make sure all sockets are closed or AT#SGACT=x,0 will ERROR.
    for (int sockId = 1; sockId <= MAX_SOCKET_COUNT; sockId++){
        snprintf(_command, sizeof(_command), "AT#SH=%d", sockId);        
        sendBasicCommand(_command);
        sockId++;
    }

    logInfo("sockets closed");
    
    snprintf(_command, _cmdBufSize, "AT#SGACT=%d,0", _type == MTSMC_LVW2 ? 3 : 1);
    sendCommand(_command, strlen(_command), _response, _rspBufSize, 10000);
    if (!strstr(_response, "OK")) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    logInfo("disconnected");

    memset(_ipAddress, 0, 16);
    strcpy(_ipAddress, "UNKNOWN");

    return NSAPI_ERROR_OK;
}

bool MTSCellularRadio::isConnected(){
    strcpy(_command, "AT#SGACT?");
    sendCommand(_command, strlen(_command), _response, _rspBufSize, 1000);

    char buf[8];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%d,1", _type == MTSMC_LVW2 ? 3 : 1);
    if (strstr(_response, buf)) {
        return true;
    } else {
        return false;
    }
}

const char *MTSCellularRadio::getIPAddress(void)
{
    return _ipAddress;
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
    if(id > MAX_SOCKET_COUNT) {
        return false;
    }

    if (isSocketOpen(id)) {
        logInfo("socket[%d] already open", id);
        return true;
    }

    if (type == "TCP") {
        snprintf(_command, _cmdBufSize, "AT#SD=%d,0,%d,\"%s\",0,0,1", id, port, addr);
    } else {
        snprintf(_command, _cmdBufSize, "AT#SD=%d,1,%d,\"%s\",0,0,1", id, port, addr);
    }
    if (sendCommand(_command, strlen(_command), _response, _rspBufSize, 65000)) {
        if (!strstr(_response, "OK")) {
            logInfo("open failed");
            return false;
        }
    }
    logInfo("open success");
    return true;
}

int MTSCellularRadio::send(int id, const void *data, uint32_t amount)
{
    int count;

    //disable echo so we don't collect all the echoed characters sent. _parser.flush() is not clearing them.
    strcpy(_command, "ATE0");
    sendCommand(_command, strlen(_command), _response, _rspBufSize);

    snprintf(_command, _cmdBufSize, "AT#SSEND=%d", id);
    sendCommand(_command, strlen(_command), _response, _rspBufSize);
    if (strstr(_response, "> ")){
        count = _parser.write((const char*)data, amount);
    }
    sendCommand("", 0, _response, _rspBufSize, 5000, CTRL_Z);

    if (!strstr(_response, "OK")){
        count = -1;
    }

    // re-enable echo.
    strcpy(_command, "ATE1");
    sendCommand(_command, strlen(_command), _response, _rspBufSize);

    return count;
}

int MTSCellularRadio::receive(int id, void *data, uint32_t amount)
{
    snprintf(_command, _cmdBufSize, "AT#SRECV=%d,%d", id, amount);
    sendCommand(_command, strlen(_command), _response, _rspBufSize, 1000);

    char buf[16];
    snprintf(buf, sizeof(buf), "#SRECV: %d", id);
    char * ptr;
    ptr = strstr(_response, buf);
    if (!ptr){
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    int connId, count;
    sscanf(ptr, "#SRECV: %d,%d %s", &connId, &count, data);

    return count;
}

bool MTSCellularRadio::close(int id)
{
    snprintf(_command, sizeof(_command), "AT#SH=%d", id);
    sendCommand(_command, strlen(_command), _response, _rspBufSize, 2000);

    if (!isSocketOpen(id)) {
        logInfo("socket[%d] closed", id);
        return true;
    }

    logInfo("socket close failed");
    return false;
}

bool MTSCellularRadio::isSocketOpen(int id)
{
    strcpy(_command, "AT#SS");
    sendCommand(_command, strlen(_command), _response, _rspBufSize);
    char buf[16];
    snprintf(buf, sizeof(buf), "#SS: %d", id);
    char * ptr;
    ptr = strstr(_response, buf);
    if (ptr) {
        if (ptr[7] != '0') {
            logInfo ("socket not closed, state = %c", ptr[7]);
            return true;
        }
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
