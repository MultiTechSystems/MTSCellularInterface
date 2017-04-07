/* MTSCellularRadio definition
 *
 */

#include "MTSCellularRadio.h"

MTSCellularRadio::MTSCellularRadio(PinName tx, PinName rx, PinName cts, PinName rts,
    PinName dcd, PinName dsr, PinName dtr, PinName ri, PinName power, PinName reset)
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
        
    // identify the radio
        && _parser.send("AT+CWMODE=%d", mode)
        && _parser.recv("OK")
        && _parser.send("AT+CIPMUX=1")
        && _parser.recv("OK");        
        // See  CellularFactory.cpp... CellularFactory::create
        
    // power on the radio
        //
        
    // test to see if an AT command works
}

Code MTSCellularRadio::sendBasicCommand(const std::string& command, unsigned int timeoutMillis, char esc)
{
    if(socketOpened) {
        logError("socket is open. Can not send AT commands");
        return MTS_ERROR;
    }

    string response = sendCommand(command, timeoutMillis, esc);
    if (response.size() == 0) {
        return MTS_NO_RESPONSE;
    } else if (response.find("OK") != string::npos) {
        return MTS_SUCCESS;
    } else if (response.find("ERROR") != string::npos) {
        return MTS_ERROR;
    } else {
        return MTS_FAILURE;
    }
}

string MTSCellularRadio::sendCommand(const std::string& command, unsigned int timeoutMillis, char esc)
{
    if(io == NULL) {
        logError("MTSBufferedIO not set");
        return "";
    }
    if(socketOpened) {
        logError("socket is open. Can not send AT commands");
        return "";
    }

    io->rxClear();
    io->txClear();
    std::string result;

    //Attempt to write command
    if(io->write(command.data(), command.size(), timeoutMillis) != command.size()) {
        //Failed to write command
        if (command != "AT" && command != "at") {
            logError("failed to send command to radio within %d milliseconds", timeoutMillis);
        }
        return "";
    }

    //Send Escape Character
    if (esc != 0x00) {
        if(io->write(esc, timeoutMillis) != 1) {
            if (command != "AT" && command != "at") {
                logError("failed to send character '%c' (0x%02X) to radio within %d milliseconds", esc, esc, timeoutMillis);
            }
            return "";
        }
    }
    mbed::Timer tmr;
    char tmp[256];
    tmp[255] = 0;
    bool done = false;
    tmr.start();
    do {
        //Make a non-blocking read call by passing timeout of zero
        int size = io->read(tmp,255,0);    //1 less than allocated (timeout is instant)
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
        
        if(tmr.read_ms() >= timeoutMillis) {
            if (command != "AT" && command != "at") {
                logWarning("sendCommand [%s] timed out after %d milliseconds", command.c_str(), timeoutMillis);
            }
            done = true;
        }
    } while (!done);
   
    return result;
}

