/* MTSCellularRadio definition
 *
 */

#include "MTSCellularRadio.h"
#include "MTSLog.h"
#include "MTSText.h"

using namespace mts;

MTSCellularRadio::MTSCellularRadio(PinName tx, PinName rx/*, PinName cts, PinName rts,
    PinName dcd, PinName dsr, PinName dtr, PinName ri, PinName power, PinName reset*/)
    : _serial(tx, rx, 1024), _parser(_serial), _cid("1")
{
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
    std::string response;
    _type = MTSCellularRadio::NA;
    while (true) {
        response = sendCommand("ATI4");
        if (response.find("HE910") != std::string::npos) {
            _type = MTQ_H5;
            _radio_model = "MTQ-H5";
        } else if (response.find("DE910") != std::string::npos) {
            _type = MTQ_EV3;
            _radio_model = "MTQ-EV3";
        } else if (response.find("CE910") != std::string::npos) {
            _type = MTQ_C2;
            _radio_model = "MTQ-C2";
        } else if (response.find("LE910-NA1") != std::string::npos) {
            _type = MTQ_LAT3;
            _radio_model = "MTQ-LAT3";
        } else if (response.find("LE910-SV1") != std::string::npos) {
            _type = MTQ_LVW3;
            _cid = "3";
            _radio_model = "MTQ-LVW3";
        } else {
            logInfo("Determining radio model");
        }
        if (_type != MTSCellularRadio::NA) {
            logInfo("radio model: %s", _radio_model.c_str());
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
    int start = response.find(':')+2;
    int stop = response.find(',');
    std::string signal = response.substr(start, stop-start);
    int rssi;
    sscanf(signal.c_str(), "%d", &rssi);
    return rssi;
}

int MTSCellularRadio::getRegistration(){
    std::string response;
    if (_type == MTQ_LAT3 || _type == MTQ_LVW3) {
        response = sendCommand("AT+CGREG?", 5000);
    } else {
        response = sendCommand("AT+CREG?", 5000);
    } 
    if (response.find("OK") == std::string::npos) {
        return UNKNOWN;
    }
    int start = response.find(',')+1;
    std::string regStat = response.substr(start, 1);
    int networkReg;
    sscanf(regStat.c_str(), "%d", &networkReg);
    return networkReg;
}
 
int MTSCellularRadio::pdpContext(const std::string& apn){
    if (_type == MTQ_C2 || _type == MTQ_EV3 || _type == MTQ_LVW3) {
        return MTS_NOT_ALLOWED;
    }
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
        logError("Can't connect. %s needs an APN.", _radio_model.c_str());
        return MTS_NEED_APN;
    }

    if(isConnected()) {
        return MTS_SUCCESS;
    }
    
    Timer tmr;
    //Check Registration
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
            return MTS_NOT_REGISTERED;
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
            return MTS_NO_SIGNAL;
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
    memset(charCommand, 0, sizeof(charCommand));
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
    if (!isSIMinserted()) {
        return false;
    }

    std::string response = sendCommand("AT#SGACT?");
    if (response.find("OK") == std::string::npos) {
        logError("Activation check failed");
        return false;
    }
    std::string reply = "#SGACT: ";
    reply.append(_cid);
    std::size_t pos = response.find(reply);
    if (pos == std::string::npos) {
        logError("Activation check failed, %s not found", reply.c_str());
        return false;
    }
    std::string conStat = response.substr(pos+10, 1);
    int contextAct;
    sscanf(conStat.c_str(), "%d", &contextAct);
    if (contextAct == 0) {
        return false;
    }
    return true;
}

bool MTSCellularRadio::isSIMinserted()
{
    if (_type == MTQ_C2) {
        return true;
    }
    if (sendBasicCommand("AT#QSS=1") != MTS_SUCCESS) {
        logWarning("Query SIM status failed");
    }
    std::string response = sendCommand("AT#QSS?");
    if (response.find("OK") == std::string::npos) {
        logWarning("Query SIM status failed");
    }
    if (response.find("1,0") != std::string::npos) {
        logError("SIM not inserted");
        return false;
    }
    return true;
}

bool MTSCellularRadio::isAPNset()
{
    std::string response = sendCommand("AT+CGDCONT?");
    std::string delimiter = ",";
    std::string apn;
    std::size_t pos;
    for (int i = 0; i < 3; i++) {
        pos = response.find(delimiter);
        if (pos == std::string::npos) {
            logWarning("APN not found");
            return false;
        }        
        apn = response.substr(0, pos);
        response.erase(0, pos + delimiter.length());
    }
    if (apn.size() < 3) {
        logWarning("APN is not set");
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
    if (!isConnected()) {
        return MTS_NO_CONNECTION;
    }

    if(id > MAX_SOCKET_COUNT) {
        return MTS_FAILURE;
    }

    if (isSocketOpen(id)) {
        logInfo("socket[%d] already open", id);
        return MTS_SUCCESS;
    }

    char charCommand[64];
    memset(charCommand, 0, sizeof(charCommand));
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
    if (!isConnected()){
        return MTS_NO_CONNECTION;
    }
    if (!isSocketOpen(id)){
        return MTS_SOCKET_CLOSED;
    }
    
    int count;

    //disable echo so we don't collect all the echoed characters sent. _parser.flush() is not clearing them.
    sendBasicCommand("ATE0");

    char charCommand[16];
    memset(charCommand, 0, sizeof(charCommand));
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
    if (!isConnected()){
        return MTS_NO_CONNECTION;
    }
    if (!isSocketOpen(id)){
        return MTS_SOCKET_CLOSED;
    }

    char charCommand[32];
    memset(charCommand, 0, sizeof(charCommand));
    snprintf(charCommand, 32, "AT#SRECV=%d,%d", id, amount);
    std::string response = sendCommand(charCommand);

    char buf[16];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "#SRECV: %d", id);
    std::size_t pos = response.find(std::string(buf));
    if (pos == std::string::npos) {
        return MTS_FAILURE;
    }
    response = response.substr(pos);
    int connId, count;
    sscanf(response.c_str(), "#SRECV: %d,%d %s", &connId, &count, data);

    return count;
}


int MTSCellularRadio::close(int id)
{
    char charCommand[16];
    memset(charCommand, 0, sizeof(charCommand));
    snprintf(charCommand, 16, "AT#SH=%d", id);
    sendBasicCommand(charCommand, 2000);

    if (!isSocketOpen(id)) {
        logInfo("socket[%d] closed", id);
        return MTS_SUCCESS;
    }

    logError("socket close failed");
    return MTS_FAILURE;
}


bool MTSCellularRadio::isSocketOpen(int id)
{
    std::string response = sendCommand("AT#SS");
    char buf[16];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "#SS: %d", id);
    std::size_t pos = response.find(std::string(buf));
    if (pos == std::string::npos) {
        logWarning("failed to read socket status");
        return false;
    }
    response = response.substr(pos);
    pos = response.find(",");
    if (pos == std::string::npos) {
        logWarning("failed to read socket status");
        return false;
    }    
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

MTSCellularRadio::statusInfo MTSCellularRadio::getRadioStatus()
{
    statusInfo radioInfo;
    // Radio model
    radioInfo.model = _radio_model;
    
    // SIM status
    radioInfo.sim = isSIMinserted();
    
    // APN
    std::string response;
    response = sendCommand("AT+CGDCONT?");
    std::size_t pos = response.find("+CGDCONT:");
    response = response.substr(pos);
    pos = response.find("\r\n\r\nOK\r\n");
    response = response.substr(0, pos);
    radioInfo.apn = response;

    // Signal strength
    radioInfo.rssi = getSignalStrength();
    
    // Registration
    radioInfo.registration = getRegistration();

    // Connection status
    radioInfo.connection = isConnected();

    // IP address
    radioInfo.ipAddress = _ipAddress;
    
    // Socket status
    response.clear();
    response = sendCommand("AT#SS");
    pos = response.find("#SS:");
    response = response.substr(pos);
    pos = response.find("\r\n\r\nOK\r\n");
    response = response.substr(0, pos);    
    radioInfo.sockets = response;

    // GPS capability
    response.clear();
    response = sendCommand("AT$GPSP?");
    if (response.find("ERROR")) {
        radioInfo.gps = -1;
    } else if (response.find("0")) {
        radioInfo.gps = 0;
    } else {
        radioInfo.gps = 1;
    }
    return radioInfo;
}

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


std::vector<MTSCellularRadio::Sms> MTSCellularRadio::getReceivedSms()
{
    int smsNumber = 0;
    std::vector<Sms> vSms;
    std::string received;
    std::size_t pos;
    
    if (sendBasicCommand("AT+CMGF=1", 2000) != MTS_SUCCESS) {
        logError("CMGF failed");
        return vSms;
    }
    
    received = sendCommand("AT+CMGL=\"ALL\"", 5000);
    pos = received.find("+CMGL: ");

    while (pos != std::string::npos) {
        Sms sms;
        std::string line(Text::getLine(received, pos, pos));
        if(line.find("+CMGL: ") == std::string::npos) {
            continue;
        }
        //Start of SMS message
        std::vector<std::string> vSmsParts = Text::split(line, ',');
        if (_type == MTQ_H5 || _type == MTQ_LAT3) {
            /* format for H5 and H5-IP radios
             * <index>, <status>, <oa>, <alpha>, <scts>
             * scts contains a comma, so splitting on commas should give us 6 items
             */
            if(vSmsParts.size() != 6) {
                logWarning("Expected 5 commas. SMS[%d] DATA[%s]. Continuing ...", smsNumber, line.c_str());
                continue;
            }

            sms.phoneNumber = vSmsParts[2];
            sms.timestamp = vSmsParts[4] + ", " + vSmsParts[5];
        } else if (_type == MTQ_EV3 || _type == MTQ_C2 || _type == MTQ_LVW3) {
            /* format for EV3 and EV3-IP radios
             * <index>, <status>, <oa>, <callback>, <date>
             * splitting on commas should give us 5 items
             */
            if(vSmsParts.size() != 5) {
                logWarning("Expected 4 commas. SMS[%d] DATA[%s]. Continuing ...", smsNumber, line.c_str());
                continue;
            }
            
            sms.phoneNumber = vSmsParts[2];
            /* timestamp is in a nasty format
             * YYYYMMDDHHMMSS
             * nobody wants to try and decipher that, so format it nicely
             * YY/MM/DD,HH:MM:SS
             */
            string s = vSmsParts[4];
            if (_type == MTQ_LVW3) {
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


int MTSCellularRadio::deleteOnlyReceivedReadSms()
{
    return sendBasicCommand("AT+CMGD=1,1");
}

int MTSCellularRadio::deleteAllReceivedSms()
{
    return sendBasicCommand("AT+CMGD=1,4");
}

int MTSCellularRadio::GPSenable() {
//Send these two commands without regard to the result. Some radios require
// these settings for GPS and others don't. The ones that don't will return
// an ERROR which we will simply ignore.    
//Enable the LNA(Low Noise Amplifier). This increases the GPS signal.   
    sendBasicCommand("AT$GPSAT=1", 2000);
//GPS is 'locked'(off) by default. Set to 0 to unlock it.    
    sendBasicCommand("AT$GPSLOCK=0", 2000);
        
//The HE910 returns an ERROR if you try to enable when it is already enabled.
// That's why we need to check if GPS is enabled before enabling it.
    if(GPSenabled()) {
        return MTS_SUCCESS;
    }
 
    if (sendBasicCommand("AT$GPSP=1", 2000) == MTS_SUCCESS) {
        logInfo("GPS enabled.");
        return MTS_SUCCESS;
    } else {
        logError("Enable GPS failed.");
        return MTS_FAILURE;
    }
}

int MTSCellularRadio::GPSdisable() {
// The HE910 returns an ERROR if you try to disable when it is already disabled.
// That's why we need to check if GPS is disabled before disabling it.
    if(!GPSenabled()) {
        logInfo("GPS was already disabled.");
        return true;
    }
    if (sendBasicCommand("AT$GPSP=0", 2000) == MTS_SUCCESS) {
        logInfo("GPS disabled.");
        return MTS_SUCCESS;
    } else {
        logError("Disable GPS failed.");
        return MTS_FAILURE;
    }
}

bool MTSCellularRadio::GPSenabled() {
    std::string reply = sendCommand("AT$GPSP?", 1000);
    if(reply.find("1") != std::string::npos) {
        return true;
    } else {
        return false;
    }
}

MTSCellularRadio::gpsData MTSCellularRadio::GPSgetPosition(){
    enum gpsFields{time, latitude, longitude, hdop, altitude, fix, cog, kmhr, knots, date, satellites, numOfFields };
    MTSCellularRadio::gpsData position;
    if(!GPSenabled()) {
        logError("GPS is disabled... can't get position.");
        position.success = false;
        return position;
    }
    // Get the position information in string format.
    std::string gps = sendCommand("AT$GPSACP?", 1000);
    logInfo("AT$GPSACP? = %s", gps.c_str());
    if(gps.find("OK") != std::string::npos) {
        position.success = true;
        // Remove echoed AT$GPSACP and leading non position characters.
        std::size_t pos = gps.find("$GPSACP: ");
        if (pos == std::string::npos) {
            logError("AT$GPSACP? command fialed");
            position.success = false;
            return position;
        }
        gps.erase(0, pos+9);
        
        // Remove trailing CR/LF, CR/LF, OK and CR/LF.
        pos = gps.find("\r\n\r\nOK\r\n");
        if (pos == std::string::npos) {
            logError("AT$GPSACP? command fialed");
            position.success = false;
            return position;
        }        
        gps.erase(gps.begin()+pos, gps.end());
        
        // Split remaining data and load into corresponding structure fields.
        std::vector<std::string> gpsParts = Text::split(gps, ',');
        // Check size.
        if(gpsParts.size() != numOfFields) {
            logError("Expected %d fields but there are %d fields in \"%s\"", numOfFields, gpsParts.size(), gps.c_str());
            position.success = false;
            return position; 
        }
        position.latitude = gpsParts[1];
        position.longitude = gpsParts[longitude];
        position.hdop = atof(gpsParts[hdop].c_str());
        position.altitude = atof(gpsParts[altitude].c_str());
        position.fix = atoi(gpsParts[fix].c_str());
        position.cog = gpsParts[cog];
        position.kmhr = atof(gpsParts[kmhr].c_str());
        position.knots = atof(gpsParts[knots].c_str());
        position.satellites = atoi(gpsParts[satellites].c_str());
        if((gpsParts[date].size() == 6) && (gpsParts[time].size() == 10)) {
            position.timestamp = gpsParts[date].substr(4,2) + "/" + gpsParts[date].substr(2,2) + 
            "/" + gpsParts[date].substr(0,2) + ", " + gpsParts[time].substr(0,2) + 
            ":" + gpsParts[time].substr(2,2) + ":" + gpsParts[time].substr(4,6);        
        }
        return position;     
    } else {
        position.success = false;
        logError("NO \"OK\" returned from GPS position command \"AT$GPSACP?\".");
        return position;
    }
}   
    
bool MTSCellularRadio::GPSgotFix() {
    if(!GPSenabled()) {
        logError("GPS is disabled... can't get fix.");
        return false;
    }
    MTSCellularRadio::gpsData position = GPSgetPosition();
    if(!position.success) {
        return false;
    } else if(position.fix < 2){
        logWarning("No GPS fix. GPS fix can take a few minutes. Check GPS antenna attachment and placement.");
        return false;
    } else {
        logInfo("Got GPS fix.");
        return true;
    }
}

