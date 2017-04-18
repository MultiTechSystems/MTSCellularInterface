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
    local_port = 0;
    local_address = "";
    host_port = 0;
    host_address = "";
    
	radio_cts = NULL;
	radio_rts = NULL;
    radio_dcd = NULL;
    radio_dsr = NULL;
    radio_ri = NULL;
    radio_dtr = NULL;
    resetLine = NULL;

    _serial.baud(115200);

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
    std::string model;
    std::string mNumber;
    type = MTSCellularRadio::NA;
    while (true) {
        sendCommand(command, (sizeof(command)/sizeof(*command)), response, (sizeof(response)/sizeof(*response)), 1000);
        model = response;
        if (model.find("HE910") != std::string::npos) {
            type = MTSCellularRadio::MTSMC_H5;
            mNumber = "HE910";
        } else if (model.find("DE910") != std::string::npos) {
            type = MTSCellularRadio::MTSMC_EV3;
            mNumber = "DE910";
        } else if (model.find("CE910") != std::string::npos) {
            type = MTSCellularRadio::MTSMC_C2;
            mNumber = "CE910";
        } else if (model.find("GE910") != std::string::npos) {
            type = MTSCellularRadio::MTSMC_G3;
            mNumber = "GE910";
        } else if (model.find("LE910-NAG") != std::string::npos) {
            type = MTSCellularRadio::MTSMC_LAT1;
            mNumber = "LE910-NAG";
        } else if (model.find("LE910-SVG") != std::string::npos) {
            type = MTSCellularRadio::MTSMC_LVW2;
            mNumber = "LE910-SVG";
        } else if (model.find("LE910-EUG") != std::string::npos) {
            type = MTSCellularRadio::MTSMC_LEU1;
            mNumber = "LE910-EUG";
        } else {
            logInfo("Determining radio type");
        }
        if (type != MTSCellularRadio::NA) {
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

int getSignalStrength(){
    return 0;
}

Registration getRegistration(){
    return REGISTERED;
}

Code setApn(const std::string& apn){
    return MTS_SUCCESS;
}

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
    _parser.setTimeout(timeoutMillis);

    if (!_parser.send("%s", command)) {
        logError("failed to send command <%s> to radio within %d milliseconds\r\n", command, timeoutMillis);
        return MTS_FAILURE;    
    }
    if (esc != 0x00) {
        if (!_parser.send("%c", esc)) {
            logError("failed to send character '%c' (0x%02X) to radio within %d milliseconds", esc, esc, timeoutMillis);
            return MTS_FAILURE;
        }
    }

    int size = _parser.read(response, response_size);
    if (size > 0) {
        return MTS_SUCCESS;
    }
    return MTS_FAILURE;
    
}

/*
static std::string getRegistrationNames(Registration registration){
    std::string result;
    return result;
}

static std::string getRadioNames(Radio radio){
    std::string result;
    return result;
}

Code echo(bool state){
    return MTS_SUCCESS;
}

std::string getDeviceIP(){
    std::string result;
    return result;
}

std::string getEquipmentIdentifier(){
    std::string result;
    return result;
}

std::string getRadioType(){
    std::string result;
    return result;
}

bool connect(){
    return true;
}

bool disconnect(){
    return true;
}

bool isConnected(){
    return true;
}
*/
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
