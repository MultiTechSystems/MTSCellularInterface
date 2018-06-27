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
	_vdd1_8 = new DigitalIn(PC_5);
	_radio_pwr = new DigitalOut(PC_3, 1);
	_3g_onoff = new DigitalOut(PC_13, 1);

	_radio_cts = NULL;
	_radio_rts = NULL;
    _radio_dcd = NULL;
    _radio_dsr = NULL;
    _radio_ri = NULL;
    _radio_dtr = NULL;
    _reset_line = NULL;

    _serial.baud(115200);

    // setup the battery circuit?
        //
        
    // identify the radio "ATI4" gets us the model (HE910, DE910, etc)
    std::string response;
    uint8_t count = 0;
    _type = MTSCellularRadio::NA;
    while (true) {
        count++;
        response = send_command("ATI4");
        if (response.find("HE910") != std::string::npos) {
            _type = MTQ_H5;
            _mts_model = "MTQ-H5";
            _manufacturer_model = "HE910";
            _registration_cmd = "AT+CREG?";
        } else if (response.find("DE910") != std::string::npos) {
            _type = MTQ_EV3;
            _mts_model = "MTQ-EV3";
            _manufacturer_model = "DE910";
            _registration_cmd = "AT+CREG?";
        } else if (response.find("CE910") != std::string::npos) {
            _type = MTQ_C2;
            _mts_model = "MTQ-C2";
            _manufacturer_model = "CE910";
            _registration_cmd = "AT+CREG?";
        } else if (response.find("LE910-NA1") != std::string::npos) {
            _type = MTQ_LAT3;
            _mts_model = "MTQ-LAT3";
            _manufacturer_model = "LE910-NA1";
            _registration_cmd = "AT+CGREG?";
        } else if (response.find("LE910-SV1") != std::string::npos) {
            _type = MTQ_LVW3;
            _cid = "3";
            _mts_model = "MTQ-LVW3";
            _manufacturer_model = "LE910-SV1";
            _registration_cmd = "AT+CGREG?";
        } else if (response.find("ME910C1-NA") != std::string::npos) {
            _type = MTQ_MAT1;
            _mts_model = "MTQ-MAT1";
            _manufacturer_model = "ME910C1-NA";
            _registration_cmd = "AT+CGREG?";
        } else if (response.find("ME910C1-NV") != std::string::npos) {
            _type = MTQ_MVW1;
            _mts_model = "MTQ-MVW1";
            _manufacturer_model = "ME910C1-NV";
            _registration_cmd = "AT+CGREG?";            
        } else {
            logInfo("Determining radio model(%d)", count);
            logTrace("_vdd1_8 = %d", _vdd1_8->read());
            if (count > 30 && _vdd1_8->read() == 0) {
                logWarning("Radio not responding... cycling power.");
                count = 0;
                power_off();
                wait(2);
                power_on();
            }
        }
        if (_type != MTSCellularRadio::NA) {
            logInfo("Radio model: %s", _mts_model.c_str());
            break;
        }
        wait(1);
    }
}


/** Sets up the physical connection pins
*   (CTS, RTS, DCD, DTR, DSR, RI and RESET)
*/
/*bool configureSignals(unsigned int CTS = NC, unsigned int RTS = NC, unsigned int DCD = NC, unsigned int DTR = NC,
    unsigned int RESET = NC, unsigned int DSR = NC, unsigned int RI = NC);
*/

bool MTSCellularRadio::power_on(int timeout){
    _3g_onoff->write(1);
    _radio_pwr->write(1);
    Timer tmr;
    tmr.start();
    while (tmr.read() < timeout) {
        wait(1);
        if (_vdd1_8->read()) {
            if (send_basic_command("AT") == MTS_SUCCESS) {
                tmr.stop();
                return true;
            }
        }
    }
    tmr.stop();
    logWarning("Failed to power on radio.");
    return false;
}

int MTSCellularRadio::power_off(){
    if (!_radio_pwr->read()) { 
        return MTS_SUCCESS;
    }
    disconnect();   // If the radio is not responding, disconnect() could take a while (currently 16s).
    if (send_basic_command("AT#SHDN", 2000) != MTS_SUCCESS) {
        logWarning("Powering off using ON_OFF. AT#SHDN not successful.");
        _3g_onoff->write(0);
        wait(1);
    }
    if (_type == MTQ_C2) {
        wait(30);
    } else {
        Timer tmr;
        tmr.start();
        while (tmr.read() < 25 && _vdd1_8->read()) {
            wait_ms(100);
        }
        tmr.stop();
    }
    if (_vdd1_8->read()) {
        logWarning("Powering off with 1.8v still present.");
    }
    _radio_pwr->write(0);
    _3g_onoff->write(0);    
/* This can be used for MTQ boards prior to revision E where 3.8v to the tiny9 is not power by 3.8v.
    if (board_rev != rev_E) {
        _3g_onoff->write(0);
        wait(1);    // Make sure 3g_onoff is off for at least 1s so the tiny9 goes to the ONOFF off state..
    }
*/    
    return MTS_SUCCESS;
}

bool MTSCellularRadio::is_powered(){
    if (_radio_pwr->read()) {
        return true;
    }
    return false;
}

std::string MTSCellularRadio::get_registration_names(uint8_t registration)
{
    switch(registration) {
        case NOT_REGISTERED:
            return "NOT_REGISTERED";
        case REGISTERED:
            return "REGISTERED";
        case SEARCHING:
            return "SEARCHING";
        case DENIED:
            return "DENIED";
        case UNKNOWN:
            return "UNKNOWN";
        case ROAMING:
            return "ROAMING";
        default:
            return "UNKNOWN ENUM";
    }
}

int MTSCellularRadio::get_signal_strength(){
    std::string response = send_command("AT+CSQ", 1000);
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

int MTSCellularRadio::get_registration(){
    std::string response = send_command(_registration_cmd, 5000);
    if (response.find("OK") == std::string::npos) {
        return UNKNOWN;
    }
    int start = response.find(',')+1;
    std::string regStat = response.substr(start, 1);
    int networkReg;
    sscanf(regStat.c_str(), "%d", &networkReg);
    return networkReg;
}
 
int MTSCellularRadio::set_apn(const std::string& apn){
    if (_type == MTQ_C2 || _type == MTQ_EV3 || _type == MTQ_LVW3) {
        return MTS_NOT_ALLOWED;
    }
    std::string pdp_type;
    if (_type == MTQ_MAT1 || _type == MTQ_MVW1) {
        pdp_type = ",\"IPV4V6\",\"";
    } else {
        pdp_type = ",\"IP\",\"";
    }        
    std::string command = "AT+CGDCONT=";
    command.append(_cid);
    command.append(pdp_type);
    command.append(apn);
    command.append("\"");
    return send_basic_command(command);
}

void MTSCellularRadio::set_sim_pin(const char *sim_pin)
{
    // Check status and only attempt to set SIM PIN if mobile device needs the SIM PIN and PIN counter is > 1.
    std::string command = "AT+CPIN?";
    std::string response = send_command(command);
    if (response.find("SIM PIN")) {
        // Check the PIN counter to see if another atttempt can be made. Stop before blocking.
        command.clear();
        response.clear();
        command = "AT#PCT";
        response = send_command(command);
        if (response.find("0") || response.find("1")) {
            logWarning("SIM PIN required but remaining tries too low. Aborting set_sim_pin.");
            return;
        }        
        command.clear();
        command = "AT+PIN=";
        command.append(sim_pin);
        if (send_basic_command(command) == MTS_ERROR) {
            logWarning("Wrong SIM PIN password. Too many tries can block SIM.");
        }
    }
}

int MTSCellularRadio::set_pdp_context(const std::string& cgdcont_args){
    std::string command = "AT+CGDCONT=";    
    command.append(cgdcont_args);
    return send_basic_command(command);
}


int MTSCellularRadio::send_basic_command(const std::string& command, unsigned int timeoutMillis)
{
    _parser.setTimeout(200);
    _parser.flush();

    std::string response = send_command(command, timeoutMillis);
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

std::string MTSCellularRadio::send_command(const std::string& command, unsigned int timeoutMillis, char esc)
{
    logTrace("command = %s", command.c_str());

    _parser.setTimeout(200);
    _parser.flush();
    _parser.setDelimiter(&esc);
    std::string response;
    
    if (!_parser.send("%s", command.c_str())) {
        logError("Failed to send command <%s> to the radio.\r\n", command.c_str());
        return response;    
    }

    Timer tmr;
    tmr.start();
    int c;
    while(tmr.read_ms() < (int)timeoutMillis) {
        c = _parser.getc();
        if (c > -1) {
            response.append(1,c);
            if ((response.find("\r\nOK\r\n")!= std::string::npos) || (response.find("\r\nERROR\r\n")!= std::string::npos)){
                break;
            }
        }
    }
    tmr.stop();
    
    logTrace("response(%d bytes) = %s", response.length(), response.c_str());
    return response;
}

int MTSCellularRadio::connect(const char cid){
    std::string cid_str;
    if (cid == 0) {
        cid_str = _cid;
    } else {
        char buff[3];
        sprintf(buff, "%d", (int)cid);
        cid_str = std::string(buff);
    }

    if (!is_apn_set()) {
        // Some radios require and APN.
        logError("Activation failed: no APN.");
        return MTS_NO_APN;    
    }
    int return_value = MTS_SUCCESS;    
    //Attempt context activation. Example successful response #SGACT: 50.28.201.151.
    std::string command = "AT#SGACT=";
    std::string response;
    command.append(cid_str);
    command.append(",1");
    //PDP context activation timeout up to 150s per 3gpp
    response = send_command(command, 60000);
    std::size_t pos = response.find("\r\n\r\nOK");
    if (pos == std::string::npos) {
        if(is_connected()) {
            // Do nothing, return_value already == MTS_SUCCESS.
            // Also there's no IP address to parse from response.
            logInfo("Already activated.");
        } else if (!is_sim_inserted()) {
        // Some radios require a SIM.
            logError("Activation failed: no SIM.");
            return_value = MTS_NO_SIM;        
        } else if ((_type == MTQ_H5 || _type == MTQ_LAT3) && !is_apn_set()) {
        // Some radios require and APN.
            logError("Activation failed: no APN.");
            return_value = MTS_NO_APN;
        } else {
        //Check Registration
            Registration registration = (Registration)get_registration();
            if (registration != REGISTERED || registration != ROAMING) {
                logError("Activation failed: not registered.");
                return_value = MTS_NOT_REGISTERED;
            } else {
            //Check RSSI: AT+CSQ
                int rssi = get_signal_strength();
                if (rssi == 99) {
                    logError("Activation failed: no signal.");
                    return MTS_NO_SIGNAL;
                } else {
                    logError("Activation failed: signal = %d.", rssi);
                    return_value = MTS_NO_CONNECTION;
                }
            }
        }
    } else {
        if(!is_connected()) {
            logError("Activation failed.");
            return_value = MTS_NO_CONNECTION;
        } else {
            response = response.substr(0, pos);     //strip trailing \r\n\r\nOK\r\n
            pos = response.find("#SGACT: ");
            std::string ip_addr = response.substr(pos+8);
            _ip_address = ip_addr;
            logInfo("Activated context %s; IP = %s", cid_str.c_str(), ip_addr.c_str());        
        }
    }   
    return return_value;
}

int MTSCellularRadio::disconnect(){
    // Make sure all sockets are closed or AT#SGACT=x,0 will ERROR.
    std::string id;
    char char_command[16];
    memset(char_command, 0, sizeof(char_command));
    for (int sockId = 1; sockId <= MAX_SOCKET_COUNT; sockId++){
        snprintf(char_command, 16, "AT#SH=%d", sockId);
        send_basic_command(std::string(char_command));
    }

    // Disconnect.
    std::string command = "AT#SGACT=";
    command.append(_cid);
    command.append(",0");
    send_basic_command(command, 5000);

    // Verify disconnecct.
    Timer tmr;
    tmr.start();
    bool connected = true;
    // I have seen the HE910 take as long as 1s to indicate disconnect. So check for a few seconds.
    while(tmr.read() < 5){
        wait_ms(500);
        if (!is_connected()) {
            connected = false;
            break;
        }
    }
    tmr.stop();
    if (connected) {
        logError("Failed to disconnect radio.");
        return MTS_FAILURE;
    }
    logInfo("Radio disconnected.");
    _ip_address.clear();
    return MTS_SUCCESS;
}

bool MTSCellularRadio::is_connected(){
    std::string response = send_command("AT#SGACT?");
    if (response.find("ERROR") != std::string::npos) {
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

bool MTSCellularRadio::is_sim_inserted()
{
    if (_type == MTQ_C2 || _type == MTQ_EV3) {
        return true;
    }
    if (send_basic_command("AT#QSS=1") != MTS_SUCCESS) {
        logWarning("Query SIM status failed.");
    }
    std::string response = send_command("AT#QSS?");
    if (response.find("OK") == std::string::npos) {
        logWarning("Query SIM status failed.");
    }
    if (response.find("1,0") != std::string::npos) {
        logError("SIM not inserted.");
        return false;
    }
    return true;
}

bool MTSCellularRadio::is_apn_set()
{
    if (_type == MTQ_C2 || _type == MTQ_EV3) {
        return true;
    }
    std::string response = send_command("AT+CGDCONT?");
    std::string delimiter = ",";
    std::string apn;
    std::size_t pos;
    for (int i = 0; i < 3; i++) {
        pos = response.find(delimiter);
        if (pos == std::string::npos) {
            logWarning("APN not found.");
            return false;
        }        
        apn = response.substr(0, pos);
        response.erase(0, pos + delimiter.size());
    }
    if (apn.size() < 3) {
        logWarning("APN is not set.");
        return false;
    }
    logInfo("APN = %s", apn.c_str()); 
    return true;
}

std::string MTSCellularRadio::get_ip_address(void)
{
    return _ip_address;
}

std::string MTSCellularRadio::get_netmask()
{
    // Not implemented.
    std::string netmask = "";
    return netmask;

}

std::string MTSCellularRadio::get_gateway()
{
    // Not implemented.
    std::string gateway = "";
    return gateway;


}

std::string MTSCellularRadio::gethostbyname(const char *name)
{
    char char_command[256];
    memset(char_command, 0, sizeof(char_command));
    snprintf(char_command, 256, "AT#QDNS=%s", name);
    std::string response = send_command(char_command, 30000);
    std::string ip_address;
    if (response.find("OK") == std::string::npos){
        return ip_address;
    }
    if (response.find("NOT SOLVED") != std::string::npos){
        return ip_address;
    }    
    int start = response.find(',')+2;
    int stop = response.find("\"\r\n");
    ip_address = response.substr(start, stop-start);
    return ip_address;    
}

int MTSCellularRadio::open(const char *type, int id, const char* addr, int port)
{
    if (id > MAX_SOCKET_COUNT) {
        logError("Socket[%d] open failed. Max socket count reached.", id);
        return MTS_FAILURE;
    }
    configure_socket(id);
    char char_command[64];
    memset(char_command, 0, sizeof(char_command));
    std::string str_type = type;
    std::string str_tcp = "TCP";
    if (str_type.compare(str_tcp) == 0) {
        snprintf(char_command, 64, "AT#SD=%d,0,%d,\"%s\",0,0,1", id, port, addr);
    } else {
        snprintf(char_command, 64, "AT#SD=%d,1,%d,\"%s\",0,0,1", id, port, addr);
    }
    std::string response = send_command(std::string(char_command), 65000);
    if (response.find("OK") != std::string::npos){
        logInfo("Socket[%d] opened", id);
        return MTS_SUCCESS;
    }

    if (is_socket_open(id)) {
        logInfo("Socket[%d] already open.", id);
        return MTS_SUCCESS;
    }
    if (!is_connected()) {
        logError("Socket[%d] open failed. No cellular connection.", id);
        return MTS_NO_CONNECTION;
    }
    logError("Socket[%d] open failed", id);
    return MTS_FAILURE;    
}

void MTSCellularRadio::configure_socket(int id){
    //Read current settings for the socket.
    std::string response = send_command("AT#SCFG?");
    if (response.find("OK") == std::string::npos){
        return;
    }
    //Strip it down to the settings for the given socket.
    char char_command[32];
    memset(char_command, 0, sizeof(char_command));
    snprintf(char_command, 32, "#SCFG: %d", id);
    std::size_t pos = response.find(char_command);
    response = response.substr(pos+7);
    pos = response.find_first_of("\r\n");
    response = response.substr(0, pos);
    //Build the command to set the inactivity timer.
    std::string command = "AT#SCFG=";
    //append then erase connId and cid
    command.append(response.substr(0,4));
    response = response.substr(4);
    //get to comma after packet size, append packet size, append socket inactivity timeout
    pos = response.find(',');
    command.append(response.substr(0,pos));
    command.append(",0,");
    response = response.substr(pos+1);
    //get to comma after inactivity timeout and remove it
    pos = response.find(',');
    response = response.substr(pos+1);
    //get to comma after connection time out and append it
    pos = response.find(',');
    command.append(response.substr(0,pos));
    response = response.substr(pos+1);
    //set data sending timeout to 10ms
    command.append(",256");
    //Write the value.
    send_command(command);
    return;
}

int MTSCellularRadio::send(int id, const void *data, uint32_t amount)
{
    int count = 0;
    //disable echo so we don't collect all the echoed characters sent. _parser.flush() is not clearing them.
    send_basic_command("ATE0");
    logDebug("radio send: %s", data);
    char char_command[32];
    memset(char_command, 0, sizeof(char_command));
    snprintf(char_command, 32, "AT#SSENDEXT=%d,%lu", id, amount);
    std::string response = send_command(std::string(char_command));
    if (response.find("> ") != std::string::npos){
        count = _parser.write((const char*)data, amount);
    }
    response.clear();
    Timer tmr;
    tmr.start();
    int c;
    while(tmr.read_ms() < 1000) {
        c = _parser.getc();
        if (c > -1) {
            response.append(1,c);
            if ((response.find("\r\nOK\r\n")!= std::string::npos) || (response.find("\r\nERROR\r\n")!= std::string::npos)){
                break;
            }            
        }
    }
    tmr.stop();    
    if (response.find("OK") == std::string::npos){
        logError("Command AT#SSENDEXT=%d failed.", id);
        if (!is_socket_open(id)){
            logError("Send failed. Socket %d closed.", id);
            count = MTS_SOCKET_CLOSED;
        } else {
            logError("Send failed.", id);
            count = MTS_FAILURE;
        }
    }

    // re-enable echo.
    send_basic_command("ATE1");

    return count;
}


int MTSCellularRadio::receive(int id, void *data, uint32_t amount)
{   
    // Send command to receive up to 'amount' characters.
    char char_command[32];
    memset(char_command, 0, sizeof(char_command));
    snprintf(char_command, 32, "AT#SRECV=%d,%lu", id, amount);

    _parser.setTimeout(200);
    _parser.setDelimiter(&CR);
    if (!_parser.send(char_command)) {
        logError("Failed to send command <%s> to the radio.\r\n", char_command);
        return 0;
    } else {
        logTrace("parser send = %s", char_command);
    }

    memset(char_command, 0, sizeof(char_command));
    snprintf(char_command, 32, "#SRECV: %d,", id);
    std::string response;
    Timer tmr;
    tmr.start();
    int c;
    // Removes leading /r/n
    c = _parser.getc();
    c = _parser.getc();
    // Read up to #SRECV='socket' or ERROR in the response.    
    while(tmr.read_ms() < 1000) {
        c = _parser.getc();
        if (c > -1) {
            response.append(1,c);
            if (response.find("\r\nERROR\r\n")!= std::string::npos) {
                // AT#SRECV=1,16 returns ERROR not #SRECV if there is no data to be received.
                tmr.stop();
                logTrace("parser getc = %s", response.c_str());
                return 0;
            }
            if ((response.find(char_command)!= std::string::npos) && (response.find("\r\n")!= std::string::npos)){
                std::size_t pos = response.find(char_command);
//                logInfo("pos = %d", pos);
                if (pos == 0){
                    // Expect response to hold "#SRECV: #,#\r\n"
                    break;
                } else {
                    //remove URC "SRING: # \r\n" and echoed command "AT#SRECV=#,# \r\n"
                    response.erase(0, pos);
                }
            }
        }
    }
    tmr.stop();
    logTrace("parser getc = %s", response.c_str());

    // Get the number of characters to read in.
    std::size_t pos1 = response.find(",");
    if (pos1 == std::string::npos) {
        return 0;
    }
    std::size_t pos2 = response.find("\r");
    if (pos2 == std::string::npos) {
        return 0;
    }

    std::string rcv_str;
    rcv_str.assign(response.begin()+(pos1+1),response.begin()+pos2);
    int rcv_count=0;
    sscanf(rcv_str.c_str(), "%d", &rcv_count);

    // Read the receive characters.
    tmr.start();
    int count = 0;
    response.clear();
    while((tmr.read_ms() < 1000) && (count < rcv_count)) {
        c = _parser.getc();
        if (c > -1) {
            response.append(1,c);
            count++;
        }
    }
    tmr.stop();

    memcpy(data, (void *)response.c_str(), count);
    
    return count;
}


int MTSCellularRadio::close(int id)
{
    char char_command[16];
    memset(char_command, 0, sizeof(char_command));
    snprintf(char_command, 16, "AT#SH=%d", id);
    send_basic_command(char_command, 2000);

    if (!is_socket_open(id)) {
        logInfo("Socket %d closed.", id);
        return MTS_SUCCESS;
    }

    logError("Socket %d not closed.", id);
    return MTS_FAILURE;
}

bool MTSCellularRadio::is_socket_open(int id)
{
    std::string response = send_command("AT#SS");
    char buf[16];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "#SS: %d", id);
    std::size_t pos = response.find(std::string(buf));
    if (pos == std::string::npos) {
        logWarning("Failed to read socket status.");
        return false;
    }
    response = response.substr(pos);
    pos = response.find(",");
    if (pos == std::string::npos) {
        logWarning("Failed to read socket status.");
        return false;
    }    
    response = response.substr(pos);    
    int status;
    sscanf(response.c_str(), ",%d", &status);
    if (status != 0) {
        return true;
    }
    return false;
}

int MTSCellularRadio::get_radio_type(){
    return _type;
}

MTSCellularRadio::statusInfo MTSCellularRadio::get_radio_status()
{
    statusInfo radioStatus;
    // Radio model
    std::string radio;
    std::string response;
    radio = _mts_model;
    radio.append(" (");
    radio.append(_manufacturer_model);
    radio.append(" fw:");
    response = send_command("AT+CGMR");
    std::size_t pos = response.find("\r\n\r\nOK");
    if (pos != std::string::npos) {
        response = response.substr(0, pos);
        pos = response.find_first_not_of("AT+CGMR\r\n");
        if (pos != std::string::npos) {
            response = response.substr(pos);
        }
        radio.append(response);
    }
    radio.append(")");
    radioStatus.model = radio;
    
    // SIM status
    radioStatus.sim = is_sim_inserted();
    
    // APN
    response.clear();
    response = send_command("AT+CGDCONT?");
    pos = response.find("+CGDCONT:");
    if (pos != std::string::npos) {
        response = response.substr(pos);
    }
    pos = response.find("\r\n\r\nOK\r\n");
    if (pos != std::string::npos) {
        response = response.substr(0, pos);
        radioStatus.apn = response;
    }

    // Signal strength
    radioStatus.rssi = get_signal_strength();
    
    // Registration
    radioStatus.registration = get_registration();

    // Connection status
    radioStatus.connection = is_connected();

    // IP address
    radioStatus.ip_address = _ip_address;
    
    // Socket status
    response.clear();
    response = send_command("AT#SS");
    pos = response.find("#SS:");
    if (pos != std::string::npos) {
        response = response.substr(pos);
    }
    pos = response.find("\r\n\r\nOK\r\n");
    if (pos != std::string::npos) {
        response = response.substr(0, pos);    
        radioStatus.sockets = response;
    }

    // GPS capability
    response.clear();
    response = send_command("AT$GPSP?");
    if (response.find("ERROR") != std::string::npos) {
        radioStatus.gps = -1;
    } else if (response.find("0") != std::string::npos) {
        radioStatus.gps = 0;
    } else {
        radioStatus.gps = 1;
    }
    return radioStatus;
}

int MTSCellularRadio::send_sms(const std::string& phone_number, const std::string& message){
    int result = send_basic_command("AT+CMGF=1");
    if ( result != MTS_SUCCESS) {
        logError("CMGF failed");
        return result;
    }

    std::string command;
    if (_type == MTQ_H5 || _type == MTQ_LAT3 || _type == MTQ_MAT1 || _type == MTQ_MVW1) {
        command = "AT+CSMP=17,167,0,0";
    } else if (_type == MTQ_EV3 || _type == MTQ_C2 || _type == MTQ_LVW3) {
        command = "AT+CSMP=,4098,0,2";
    } else {
        logError("Unknown radio type [%d].", _type);
        return MTS_FAILURE;
    }

    result = send_basic_command(command);
    if (result != MTS_SUCCESS) {
        logError("CSMP failed [%s]", _mts_model.c_str());
        return result;
    }

    logInfo("Sending SMS to %s", phone_number.c_str());
    command = "AT+CMGS=\"+";
    command.append(phone_number);
    command.append("\",145");
    std::string response = send_command(command, 2000);
    if (response.find("> ") != std::string::npos) {
        _parser.write(message.c_str(), message.size());
    }
    response = send_command("", 15000, CTRL_Z);

    if (response.find("\r\nOK\r\n") != std::string::npos) {
        return MTS_SUCCESS;
    }
    logError("CMGS message failed");
    return MTS_FAILURE;
}

std::vector<MTSCellularRadio::Sms> MTSCellularRadio::get_received_sms()
{
    int smsNumber = 0;
    std::vector<Sms> vSms;
    std::string received;
    std::size_t pos;
    
    if (send_basic_command("AT+CMGF=1", 2000) != MTS_SUCCESS) {
        logError("CMGF failed");
        return vSms;
    }
    
    received = send_command("AT+CMGL=\"ALL\"", 5000);
    pos = received.find("+CMGL: ");

    while (pos != std::string::npos) {
        Sms sms;
        std::string line(Text::getLine(received, pos, pos));
        if(line.find("+CMGL: ") == std::string::npos) {
            continue;
        }
        //Start of SMS message
        std::vector<std::string> vSmsParts = Text::split(line, ',');
        if (_type == MTQ_H5 || _type == MTQ_LAT3 || _type == MTQ_MAT1 || _type == MTQ_MVW1) {
            /* format for H5 and H5-IP radios
             * <index>, <status>, <oa>, <alpha>, <scts>
             * scts contains a comma, so splitting on commas should give us 6 items
             */
            if(vSmsParts.size() != 6) {
                logWarning("Expected 5 commas. SMS[%d] DATA[%s]. Continuing ...", smsNumber, line.c_str());
                continue;
            }

            sms.phone_number = vSmsParts[2];
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
            
            sms.phone_number = vSmsParts[2];
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


int MTSCellularRadio::delete_only_read_sms()
{
    return send_basic_command("AT+CMGD=1,1");
}

int MTSCellularRadio::delete_all_received_sms()
{
    return send_basic_command("AT+CMGD=1,4");
}

int MTSCellularRadio::gps_enable() {
    if (_type == MTQ_C2 || _type == MTQ_LAT3 || _type == MTQ_LVW3) {
        logError("Cannot enable GPS; not supported on %s radio.", _mts_model.c_str());
        return MTS_NOT_SUPPORTED;
    }    
//Send these two commands without regard to the result. Some radios require
// these settings for GPS and others don't. The ones that don't will return
// an ERROR which we will simply ignore.    
//Enable the LNA(Low Noise Amplifier). This increases the GPS signal.   
    send_basic_command("AT$GPSAT=1", 2000);
//GPS is 'locked'(off) by default. Set to 0 to unlock it.    
    send_basic_command("AT$GPSLOCK=0", 2000);
        
//The HE910 returns an ERROR if you try to enable when it is already enabled.
// That's why we need to check if GPS is enabled before enabling it.
    if(is_gps_enabled()) {
        return MTS_SUCCESS;
    }
 
    if (send_basic_command("AT$GPSP=1", 2000) == MTS_SUCCESS) {
        logInfo("GPS enabled.");
        return MTS_SUCCESS;
    } else {
        logError("Enable GPS failed.");
        return MTS_FAILURE;
    }
}

int MTSCellularRadio::gps_disable() {
    if (_type == MTQ_C2 || _type == MTQ_LAT3 || _type == MTQ_LVW3) {
        return MTS_NOT_SUPPORTED;
    }    
// The HE910 returns an ERROR if you try to disable when it is already disabled.
// That's why we need to check if GPS is disabled before disabling it.
    if(!is_gps_enabled()) {
        logInfo("GPS was already disabled.");
        return true;
    }
    if (send_basic_command("AT$GPSP=0", 2000) == MTS_SUCCESS) {
        logInfo("GPS disabled.");
        return MTS_SUCCESS;
    } else {
        logError("Disable GPS failed.");
        return MTS_FAILURE;
    }
}

bool MTSCellularRadio::is_gps_enabled() {
    if (_type == MTQ_C2 || _type == MTQ_LAT3 || _type == MTQ_LVW3) {
        logError("%s radio does not support GPS.", _mts_model.c_str());
        return false;
    }    
    std::string reply = send_command("AT$GPSP?", 1000);
    if(reply.find("1") != std::string::npos) {
        return true;
    } else {
        return false;
    }
}

MTSCellularRadio::gpsData MTSCellularRadio::gps_get_position(){
    enum gpsFields{time, latitude, longitude, hdop, altitude, fix, cog, kmhr, knots, date, satellites, numOfFields };
    MTSCellularRadio::gpsData position;
    if(!is_gps_enabled()) {
        logError("GPS is disabled... can't get position.");
        position.success = false;
        return position;
    }
    // Get the position information in string format.
    std::string gps = send_command("AT$GPSACP?", 1000);
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
    
bool MTSCellularRadio::gps_has_fix() {
    if (_type == MTQ_C2 || _type == MTQ_LAT3 || _type == MTQ_LVW3) {
        return false;
    }    
    if(!is_gps_enabled()) {
        logError("GPS is disabled... can't get fix.");
        return false;
    }
    MTSCellularRadio::gpsData position = gps_get_position();
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

