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
    //_parser.debugOn(debug);

    // setup the battery circuit
        //
        
    // wait for radio to get into a good state
    while (true) {
        _parser.send("AT\r\n");
        if (_parser.recv("OK")) {
            logInfo("radio replied\r\n");
            break;
        } else {
            logInfo("waiting on radio...\r\n");
        }
        wait(1);
    }

    // identify the radio "ATI4" gets us the model (HE910, DE910, etc)
    while (true) {
        std::string mNumber;
        std::string model;
        model = sendCommand("ATI4", 3000);
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
    

/*    
    debug1.printf("creating MTSCelluarRadio object!\r\n");
    char buffer[30] = "buffer";
    _parser.setTimeout(10);
    int size;
    for(int i = 0; i < 5; i++){
        debug1.printf(".");
        _parser.send("ati\r\n");
        size = _parser.read(buffer, 30);
        if (size > 0)
            break;
        wait_ms(500);
    }
    debug1.printf("ati read %d characters. They are: %s\r\n", size, buffer);
//    buffer[] = "buffer";
    debug1.printf("Now the buffer contains: %s\r\n", buffer);
    std::string text = "this is a string\r\n";
    debug1.printf("the string = %s\r\n", text.c_str());
*/    
/*        && _parser.send("AT+CWMODE=%d", mode)
        && _parser.recv("OK")
        && _parser.send("AT+CIPMUX=1")
        && _parser.recv("OK");        
*/        // See  CellularFactory.cpp... CellularFactory::create
        
    // power on the radio
        //
        
    // test to see if an AT command works
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


int MTSCellularRadio::sendBasicCommand(const std::string& command, unsigned int timeoutMillis, char esc)
{
/*    if(socketOpened) {
        logError("socket is open. Can not send AT commands");
        return MTS_ERROR;
    }
*/
    std::string response = sendCommand(command, timeoutMillis, esc);
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
    std::string result;

    if (!_parser.send("%s", command.c_str())) {
        logError("failed to send command <%s> to radio within %d milliseconds\r\n", command.c_str(), timeoutMillis);
        return "";
    }
    if (esc != 0x00) {
        if (!_parser.send("%c", esc)) {
            logError("failed to send character '%c' (0x%02X) to radio within %d milliseconds", esc, esc, timeoutMillis);
            return "";
        }
    }

    mbed::Timer tmr;
    char tmp[256];
    tmp[255] = 0;
    bool done = false;
    tmr.start();
    do {
        //Make a non-blocking read call... setting timeout to zero
        _parser.setTimeout(0);        
        int size = _parser.read(tmp,255);
        if(size > 0) {
            result.append(tmp, size);
        }
        
        //Check for a response to signify the completion of the AT command
        //OK, ERROR, CONNECT are the 3 most likely responses
        if(result.size() > (command.size() + 2)) {
                if(result.find("OK\r\n",command.size()) != std::string::npos) {
                    done = true;
                } else if (result.find("ERROR") != std::string::npos) {
                    done = true;
                } else if (result.find("NO CARRIER\r\n") != std::string::npos) {
                    done = true;
                }
                
                if(type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_EV3 || type == MTSMC_C2 || type == MTSMC_LAT1 || type == MTSMC_LEU1 || type == MTSMC_LVW2) {
                    if (result.find("CONNECT\r\n") != std::string::npos) {
                        done = true;
                    } 
                } else if (type == MTSMC_H5_IP || type == MTSMC_EV3_IP || type == MTSMC_C2_IP) {
                    if (result.find("Ok_Info_WaitingForData\r\n") != std::string::npos) {
                        done = true;
                    } else if (result.find("Ok_Info_SocketClosed\r\n") != std::string::npos) {
                        done = true;
                    } else if (result.find("Ok_Info_PPP\r\n") != std::string::npos) {
                        done = true;
                    } else if (result.find("Ok_Info_GprsActivation\r\n") != std::string::npos) {
                        done = true;
                    }
                }
        }
        
        if((tmr.read_ms() >= timeoutMillis) && !done) {
            if (command != "AT" && command != "at") {
                logWarning("sendCommand [%s] timed out after %d milliseconds\r\n", command.c_str(), timeoutMillis);
            }
            done = true;
        }
    } while (!done);
   
    return result;

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
