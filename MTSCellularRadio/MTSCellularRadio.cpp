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
    std::string regStat = response.substr(start + 1, stop - start - 1);
    int value;
    sscanf(regStat.c_str(), "%d", &value);
    return value;
}
 
int MTSCellularRadio::pdpContext(const std::string& apn){
    std::string command = "AT+CGDCONT=";
    command.append(_cid);
    command.append(",\"IP\",\"");
    command.append(apn);
    command.append("\"");
    return sendBasicCommand(command);
}

 /*
Code setDns(const std::string& primary, const std::string& secondary){
    return MTS_SUCCESS;
}
*/

int MTSCellularRadio::sendBasicCommand(const std::string& command, unsigned int timeoutMillis)
{
    _parser.setTimeout(200);
    _parser.flush();

    std::string response = sendCommand(command, timeoutMillis);
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
    logInfo("connecting context %s...", _cid.c_str());
    //The apn must be configured for some radios or connect will not work.
    if ((_type == MTQ_H5 || _type == MTQ_LAT3) && !isAPNset()) {
        return MTS_FAILURE;
    }

    if(isConnected()) {
        return MTS_SUCCESS;
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

    //Attempt context activation. Example successful response #SGACT: 50.28.201.151.
    logDebug("Making connection attempt");
    std::string command = "AT#SGACT=";
    command.append(_cid);
    command.append(",1");
    std::string response = sendCommand(command, 10000);

    std::size_t pos = response.find("#SGACT: ");
    if (pos == std::string::npos) {
        return MTS_FAILURE;
    }
    // Make sure it connected.
    if (!isConnected()) {
        return MTS_FAILURE;
    }
    std::string ipAddr = response.substr(pos+8);
    _ipAddress = ipAddr;

    logInfo("connected context %s; IP = %s", _cid.c_str(), ipAddr.c_str());
    return MTS_SUCCESS;
}

int MTSCellularRadio::disconnect(){
    logInfo("disconnecting...");
    
    // Make sure all sockets are closed or AT#SGACT=x,0 will ERROR.
    std::string id;
    char charCommand[16];
    for (int sockId = 1; sockId <= MAX_SOCKET_COUNT; sockId++){
        snprintf(charCommand, 16, "AT#SH=%d", sockId);
        sendBasicCommand(std::string(charCommand));
    }

    // Disconnect.
    std::string command = "AT#SGACT=";
    command.append(_cid);
    command.append(",0");
    sendBasicCommand(command, 5000);

    // Verify disconnecct.
    Timer tmr;
    tmr.start();
    bool connected = true;
    // I have seen the HE910 take as long as 1s to indicate disconnect. So check for a few seconds.
    while(tmr < 5){
        wait_ms(200);
        if (!isConnected()) {
            connected = false;
            break;
        }
    }
    tmr.stop();
    if (connected) {
        logInfo("failed to disconnect");
        return MTS_FAILURE;
    }
    logInfo("disconnected");
    _ipAddress.clear();
    return MTS_SUCCESS;
}

bool MTSCellularRadio::isConnected(){
    std::string command = "AT#SGACT?";
    std::string response = sendCommand(command);

    std::size_t pos = response.find(',');
    std::string act = response.substr(pos);
    if (act.find("1") != std::string::npos) {
        return true;
    }
    return false;
}

bool MTSCellularRadio::isAPNset(){
    std::string response = sendCommand("AT+CGDCONT?");
    std::string delimiter = ",";
    std::string apn;
    std::size_t pos;
    for (int i = 0; i < 3; i++) {
        pos = response.find(delimiter);
        apn = response.substr(0, pos);
        response.erase(0, pos + delimiter.length());
    }
    if (apn.size() < 3) {
        logDebug("APN is not set");
        return false;
    }
    logInfo("APN = %s", apn.c_str()); 
    return true;
}

std::string MTSCellularRadio::getIPAddress(void)
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

int MTSCellularRadio::open(const char *type, int id, const char* addr, int port)
{
    if(id > MAX_SOCKET_COUNT) {
        return MTS_FAILURE;
    }

    if (isSocketOpen(id)) {
        logInfo("socket[%d] already open", id);
        return MTS_SUCCESS;
    }

    char charCommand[64];
    if (type == "TCP") {
        snprintf(charCommand, 64, "AT#SD=%d,0,%d,\"%s\",0,0,1", id, port, addr);
    } else {
        snprintf(charCommand, 64, "AT#SD=%d,1,%d,\"%s\",0,0,1", id, port, addr);
    }
    std::string response = sendCommand(std::string(charCommand), 65000);
    if (response.find("OK") == std::string::npos){
        logInfo("open failed");
        return MTS_FAILURE;
    }
    logInfo("open success");
    return MTS_SUCCESS;
}


int MTSCellularRadio::send(int id, const void *data, uint32_t amount)
{
    int count;

    //disable echo so we don't collect all the echoed characters sent. _parser.flush() is not clearing them.
    sendBasicCommand("ATE0");

    char charCommand[16];
    snprintf(charCommand, 16, "AT#SSEND=%d", id);
    std::string response = sendCommand(std::string(charCommand));
    if (response.find("> ") != std::string::npos){
        count = _parser.write((const char*)data, amount);
    }
    response = sendCommand("", 5000, CTRL_Z);

    if (response.find("OK") != std::string::npos){
        count = MTS_FAILURE;
    }

    // re-enable echo.
    sendBasicCommand("ATE1");

    return count;
}


int MTSCellularRadio::receive(int id, void *data, uint32_t amount)
{   
    char charCommand[32];
    snprintf(charCommand, 32, "AT#SRECV=%d,%d", id, amount);
    std::string response = sendCommand(charCommand);

    char buf[16];
    snprintf(buf, sizeof(buf), "#SRECV: %d", id);
    std::size_t pos = response.find(std::string(buf));
    if (pos == std::string::npos) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    response = response.substr(pos);
    int connId, count;
    sscanf(response.c_str(), "#SRECV: %d,%d %s", &connId, &count, data);

    return count;
}


bool MTSCellularRadio::close(int id)
{
    char charCommand[16];
    snprintf(charCommand, 16, "AT#SH=%d", id);
    sendBasicCommand(charCommand, 2000);

    if (!isSocketOpen(id)) {
        logInfo("socket[%d] closed", id);
        return true;
    }

    logInfo("socket close failed");
    return false;
}


bool MTSCellularRadio::isSocketOpen(int id)
{
    std::string response = sendCommand("AT#SS");
    char buf[16];
    snprintf(buf, sizeof(buf), "#SS: %d", id);
    std::size_t pos = response.find(std::string(buf));
    response = response.substr(pos);
    pos = response.find(",");
    response = response.substr(pos);    
    int status;
    sscanf(response.c_str(), ",%d", &status);
    if (status != 0) {
        logInfo ("socket not closed, state = %d", status);
        return true;
    }
    return false;
}

/*
bool ping(const std::string& address){
    return true;
}
*/

int MTSCellularRadio::sendSMS(const char* phoneNumber, const char* message, int messageSize){
    if (sendBasicCommand("AT+CMGF=1") != MTS_SUCCESS) {
        logWarning("CMGF failed to set text mode");
    }

    std::string command;
    if (_type == MTQ_H5 || _type == MTQ_LAT3) {
        command = "AT+CSMP=17,167,0,0";
    } else if (_type == MTQ_EV3 || _type == MTQ_C2 || _type == MTQ_LVW3) {
        command = "AT+CSMP=,4098,0,2";
    } else {
        logError("unknown radio type [%d]", _type);
        return MTS_FAILURE;
    }
    
    if (sendBasicCommand(command) != MTS_SUCCESS) {
        logWarning("CSMP set text mode parameters failed");
    }

    command = "AT+CMGS=\"+";
    command.append(phoneNumber);
    command.append("\",145");
    std::string response = sendCommand(command, 2000);
    if (response.find("> ") != std::string::npos) {
        _parser.write(message, messageSize);
    }
    response = sendCommand("", 15000, CTRL_Z);

    if (response.find("+CMGS:") != std::string::npos) {
        return MTS_SUCCESS;
    }        
    return MTS_FAILURE;
}

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
