/* MTSCellularRadio definition
 *
 */

#include "MTSCellularRadio.h"
#include "MTSLog.h"

MTSCellularRadio::MTSCellularRadio(PinName tx, PinName rx/*, PinName cts, PinName rts,
    PinName dcd, PinName dsr, PinName dtr, PinName ri, PinName power, PinName reset*/)
    : _serial(tx, rx, 1024), _parser(_serial), _cid("1")
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
        
    // identify the radio "ATI4" gets us the model (HE910, DE910, etc)
    std::string response, mNumber;
    _type = MTSCellularRadio::NA;
    while (true) {
        response = sendCommand("ATI4");
        if (response.find("HE910") != std::string::npos) {
            _type = MTQ_H5;
            mNumber = "HE910";
        } else if (response.find("DE910") != std::string::npos) {
            _type = MTQ_EV3;
            mNumber = "DE910";
        } else if (response.find("CE910") != std::string::npos) {
            _type = MTQ_C2;
            mNumber = "CE910";
        } else if (response.find("LE910-NAG") != std::string::npos) {
            _type = MTQ_LAT3;
            mNumber = "LE910-NAG";
        } else if (response.find("LE910-SVG") != std::string::npos) {
            _type = MTQ_LVW3;
            _cid = "3";
            mNumber = "LE910-SVG";
        } else {
            logInfo("Determining radio type");
        }
        if (_type != MTSCellularRadio::NA) {
            logInfo("radio model: %s", mNumber.c_str());
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
    std::string response = sendCommand("AT+CSQ", 1000);
    if (response.find("OK") == std::string::npos) {
        return MTS_FAILURE;
    }
    int start = response.find(':');
    int stop = response.find(',', start);
    std::string signal = response.substr(start + 2, stop - start - 2);
    int rssi;
    sscanf(signal.c_str(), "%d", &rssi);
    return rssi;
}


int MTSCellularRadio::getRegistration(){
    std::string response = sendCommand("AT+CREG?", 5000);
    if (response.find("OK") == string::npos) {
        return UNKNOWN;
    }
    int start = response.find(',');
    int stop = response.find(' ', start);
    string regStat = response.substr(start + 1, stop - start - 1);
    int value;
    sscanf(regStat.c_str(), "%d", &value);
    switch (value) {
        case 0:
            return NOT_REGISTERED;
        case 1:
            return REGISTERED;
        case 2:
            return SEARCHING;
        case 3:
            return DENIED;
        case 4:
            return UNKNOWN;
        case 5:
            return ROAMING;
    }
    return UNKNOWN;
}


 
int MTSCellularRadio::pdpContext(const std::string& apn){
    std::string command = "AT+CGDCONT=";
    command.append(_cid);
    command.append(",\"IP\",");
    command.append(apn);
    return sendBasicCommand(command);
}

 /*
Code setDns(const std::string& primary, const std::string& secondary){
    return MTS_SUCCESS;
}
*/


int MTSCellularRadio::sendBasicCommand(const std::string& command)
{
    _parser.setTimeout(200);
    _parser.flush();

    std::string response = sendCommand(command, 2000);
    if (response.size() == 0) {
        return MTS_NO_RESPONSE;
    } else if (response.find("OK") != std::string::npos) {
        return MTS_SUCCESS;
    } else if (response.find("ERROR") != std::string::npos) {
        return MTS_ERROR;
    } else {
        return MTS_FAILURE;
    }
}

std::string MTSCellularRadio::sendCommand(const std::string& command, unsigned int timeoutMillis, char esc)
{
    _parser.setTimeout(200);
    _parser.flush();
    std::string response;

    logInfo("command = %s", command.c_str());

    if (!_parser.send("%s", command.c_str())) {
        logError("failed to send command <%s> to radio\r\n", command.c_str());
        return response;    
    }
    if (esc != 0x00) {
        if (!_parser.send("%c", esc)) {
            logError("failed to send character '%c' (0x%02X) to radio\r\n", esc, esc);
            return response;
        }
    }

    Timer tmr;
    tmr.start();
    int count = 0;
    int c;
    while(tmr.read_ms() < timeoutMillis) {
        c = _parser.getc();
        if (c > -1) {
            response.append(1,c);
            if ((response.find("\r\nOK\r\n")!= std::string::npos) || (response.find("\r\nERROR\r\n")!= std::string::npos)){
                break;
            }
        }
    }
    tmr.stop();
    
    logInfo("response = %s", response.c_str());
    return response;
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
    if (_type == MTQ_H5 || _type == MTQ_LAT3) {
        if(sizeof(_apn) == 0) {
            logDebug("APN is not set");
            return MTS_FAILURE;
        }
    }

//    if(isConnected()) {
//        return MTS_SUCCESS;
//    }
    
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
            return MTS_FAILURE;
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
            return MTS_FAILURE;
        }
        wait(1);
    } while(1);
    tmr.stop();

    //Make cellular connection
    if (_type == MTQ_H5 || _type == MTQ_LAT3) {
        logDebug("Making PPP Connection Attempt. APN[%s]", _apn.c_str());
    } else {
        logDebug("Making PPP Connection Attempt");
    }
    //Attempt context activation. Example successful response #SGACT: 50.28.201.151.
    std::string command = "AT#SGACT=";
    command.append(_cid);
    command.append(",1");
    std::string response = sendCommand(command, 10000);

    std::size_t pos = response.find("#SGACT: ");
    std::string ipAddr = response.substr(pos+8);

    logInfo("PPP Connection Established: IP = %s", ipAddr.c_str());
    return MTS_SUCCESS;
}

/*leon
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
    
    snprintf(_command, _cmdBufSize, "AT#SGACT=%d,0", _type == MTQ_LVW3 ? 3 : 1);
    sendCommand(_command, strlen(_command), _response, _rspBufSize, 10000);
    if (!strstr(_response, "OK")) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    logInfo("disconnected");

    memset(_ipAddress, 0, 16);
    strcpy(_ipAddress, "UNKNOWN");

    return NSAPI_ERROR_OK;
}
*/
/*leon
bool MTSCellularRadio::isConnected(){
    strcpy(_command, "AT#SGACT?");
    sendCommand(_command, strlen(_command), _response, _rspBufSize, 1000);

    char buf[8];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%d,1", _type == MTQ_LVW3 ? 3 : 1);
    if (strstr(_response, buf)) {
        return true;
    } else {
        return false;
    }
}
*/
/*leon
const char *MTSCellularRadio::getIPAddress(void)
{
    return _ipAddress;
}
*/
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

/*leon
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
*/
/*leon
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
*/
/*leon
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
*/
/*leon
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
*/
/*leon
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
*/
/*
bool ping(const std::string& address){
    return true;
}
*/
/*leon
int MTSCellularRadio::sendSMS(const char* phoneNumber, const char* message, int messageSize){
    strcpy(_command, "AT+CMGF=1");
    if(sendCommand(_command, strlen(_command), _response, _rspBufSize, 2000) < 0) {
        logError("CMGF failed");
        return MTS_FAILURE;
    }

    if (_type == MTQ_H5 || _type == MTQ_LAT3) {
        strcpy(_command, "AT+CSMP=17,167,0,0");
    } else if (_type == MTQ_EV3 || _type == MTQ_C2 || _type == MTQ_LVW3) {
        strcpy(_command, "AT+CSMP=,4098,0,2");
    } else {
        logError("unknown radio type [%d]", _type);
        return MTS_FAILURE;
    }
    
    if(sendBasicCommand(_command) < 0) {
        logError("CSMP failed");// [%s]", getRadioNames(_type).c_str());
        return MTS_FAILURE;
    }
    
    snprintf(_command, _cmdBufSize, "AT+CMGS=\"+%s\",145", phoneNumber);
    sendCommand(_command, strlen(_command), _response, _rspBufSize, 2000);
    if (strstr(_response, "> ")){
        _parser.write(message, messageSize);
    }
    sendCommand("", 0, _response, _rspBufSize, 15000, CTRL_Z);

    if (strstr(_response, "+CMGS:")) {
        return MTS_SUCCESS;
    }
        
    return MTS_FAILURE;
}
*/
/*
std::vector<Cellular::Sms> MTSCellularRadio::getReceivedSms()
{
    int smsNumber = 0;
    std::vector<Sms> vSms;
    std::string received;
    size_t pos;
    
    Code code = sendBasicCommand("AT+CMGF=1", 2000);
    if (code != MTS_SUCCESS) {
        logError("CMGF failed");
        return vSms;
    }
    
    received = sendCommand("AT+CMGL=\"ALL\"", 5000);
    pos = received.find("+CMGL: ");

    while (pos != std::string::npos) {
        Cellular::Sms sms;
        std::string line(Text::getLine(received, pos, pos));
        if(line.find("+CMGL: ") == std::string::npos) {
            continue;
        }
        //Start of SMS message
        std::vector<std::string> vSmsParts = Text::split(line, ',');
        if (type == MTSMC_H5_IP || type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_LAT1 || type == MTSMC_LEU1) {
            /* format for H5 and H5-IP radios
             * <index>, <status>, <oa>, <alpha>, <scts>
             * scts contains a comma, so splitting on commas should give us 6 items
             */
/*            if(vSmsParts.size() != 6) {
                logWarning("Expected 5 commas. SMS[%d] DATA[%s]. Continuing ...", smsNumber, line.c_str());
                continue;
            }

            sms.phoneNumber = vSmsParts[2];
            sms.timestamp = vSmsParts[4] + ", " + vSmsParts[5];
        } else if (type == MTSMC_EV3_IP || type == MTSMC_EV3 || type == MTSMC_C2_IP || type == MTSMC_C2 || type == MTSMC_LVW2) {
            /* format for EV3 and EV3-IP radios
             * <index>, <status>, <oa>, <callback>, <date>
             * splitting on commas should give us 5 items
             */
/*            if(vSmsParts.size() != 5) {
                logWarning("Expected 4 commas. SMS[%d] DATA[%s]. Continuing ...", smsNumber, line.c_str());
                continue;
            }
            
            sms.phoneNumber = vSmsParts[2];
            /* timestamp is in a nasty format
             * YYYYMMDDHHMMSS
             * nobody wants to try and decipher that, so format it nicely
             * YY/MM/DD,HH:MM:SS
             */
/*            string s = vSmsParts[4];
            if (type == MTSMC_LVW2) {
                sms.timestamp = s.substr(3,2) + "/" + s.substr(5,2) + "/" + s.substr(7,2) + ", " + s.substr(9,2) + ":" + s.substr(11,2) + ":" + s.substr(13,2);
            } else {
                sms.timestamp = s.substr(2,2) + "/" + s.substr(4,2) + "/" + s.substr(6,2) + ", " + s.substr(8,2) + ":" + s.substr(10,2) + ":" + s.substr(12,2);
            }
        }

        if(pos == std::string::npos) {
            logWarning("Expected SMS body. SMS[%d]. Leaving ...", smsNumber);
            break;
        }
        //Check for the start of the next SMS message
        size_t bodyEnd = received.find("\r\n+CMGL:", pos);
        if(bodyEnd == std::string::npos) {
            //This must be the last SMS message
            bodyEnd = received.find("\r\n\r\nOK", pos);
        }
        //Safety check that we found the boundary of this current SMS message
        if(bodyEnd != std::string::npos) {
            sms.message = received.substr(pos, bodyEnd - pos);
        } else {
            sms.message = received.substr(pos);
            logWarning("Expected to find end of SMS list. SMS[%d] DATA[%s].", smsNumber, sms.message.c_str());
        }
        vSms.push_back(sms);
        pos = bodyEnd;
        smsNumber++;
    }
    logInfo("Received %d SMS", smsNumber);
    return vSms;
}
/*
int MTSCellularRadio::deleteOnlyReceivedReadSms()
{
    strcpy(_command, "AT+CMGD=1,1");
    if(sendBasicCommand(_command) < 0) {
        return MTS_FAILURE;
    }
    return MTS_SUCCESS;
}

int MTSCellularRadio::deleteAllReceivedSms()
{
    strcpy(_command, "AT+CMGD=1,4");
    if(sendBasicCommand(_command) < 0) {
        return MTS_FAILURE;
    }
    return MTS_SUCCESS;    
}
*/
/*
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
