/* MTSCellularInterface implementation of NetworkInterface API
*
*/

#include "MTSCellularInterface.h"
#include "MTSLog.h"

// MTSCellularInterface implementation
MTSCellularInterface::MTSCellularInterface(PinName Radio_tx, PinName Radio_rx/*, PinName rts, PinName cts,
	PinName dcd, PinName dsr, PinName dtr, PinName ri, PinName power, PinName reset*/)
	: _radio(Radio_tx, Radio_rx/*, rts, cts, dcd, dsr, dtr, ri, power, reset*/)
{
    memset(_ids, 0, sizeof(_ids));
    memset(_cbs, 0, sizeof(_cbs));
}

int MTSCellularInterface::set_credentials(const char *apn, const char *username, const char *password){
    int result = _radio.pdpContext(apn);
    if (result == MTSCellularRadio::MTS_NOT_ALLOWED){
        return NSAPI_ERROR_UNSUPPORTED;
    }
    if (result != MTSCellularRadio::MTS_SUCCESS){
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::sendBasicCommand(const std::string& command, unsigned int timeoutMillis)
{
    if (_radio.sendBasicCommand(command, timeoutMillis) != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

std::string MTSCellularInterface::sendCommand(const std::string& command, unsigned int timeoutMillis, char esc){
    return _radio.sendCommand(command, timeoutMillis, esc);
}

int MTSCellularInterface::connect(const char *apn, const char *username, const char *password){
    int result = set_credentials(apn, username, password);
    if (result == NSAPI_ERROR_DEVICE_ERROR) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return connect();
}
    
int MTSCellularInterface::connect(){
    int result = _radio.connect();
    switch (result){
        case MTSCellularRadio::MTS_FAILURE:
            return NSAPI_ERROR_DEVICE_ERROR;
        case MTSCellularRadio::MTS_SUCCESS:
            return NSAPI_ERROR_OK;
        default:
            return NSAPI_ERROR_NO_CONNECTION;
    }  
}
     
int MTSCellularInterface::disconnect(){
    if (_radio.disconnect() == MTSCellularRadio::MTS_SUCCESS){
        return NSAPI_ERROR_OK;
    }
    return NSAPI_ERROR_DEVICE_ERROR;
}

const char *MTSCellularInterface::get_ip_address()
{
    return _radio.getIPAddress().c_str();
}

void MTSCellularInterface::logRadioStatus(){
    MTSCellularRadio::statusInfo radioStatus = _radio.getRadioStatus();
    // Opening banner
    logInfo("************ Radio Information ************");
    
    // Radio model
    logInfo("Radio: %s", radioStatus.model.c_str());

    // SIM status
    if (radioStatus.sim) {
        logInfo("SIM status: Inserted");
    } else {
        logInfo("SIM status: Not inserted");
    }

    // APN
    logInfo("APN: \r\n%s", radioStatus.apn.c_str());

    // Signal strength
    logInfo("Signal strength: %d", radioStatus.rssi);

    // Network registration
    switch (radioStatus.registration) {
        case MTSCellularRadio::NOT_REGISTERED:
            logInfo("Network registration: not registered, not searching");
            break;
        case MTSCellularRadio::REGISTERED:
            logInfo("Network registration: registered, home network");
            break;
        case MTSCellularRadio::SEARCHING:
            logInfo("Network registration: not registered, searching");
            break;
        case MTSCellularRadio::DENIED:
            logInfo("Network registration: registration denied");
            break;
        case MTSCellularRadio::UNKNOWN:
            logInfo("Network registration: unknown");
            break;
        case MTSCellularRadio::ROAMING:
            logInfo("Network registration: registered, roaming");
    }

    // Connection status and IP address if connected
    if (radioStatus.connection) {
        logInfo("Cellular connection: active");
        logInfo("IP address: %s", radioStatus.ipAddress.c_str());
    } else {
        logInfo("Cellular connetion: not active");
    }

    // Socket status
    logInfo("Socket status: \r\n%s", radioStatus.sockets.c_str());

    // GPS
    if (radioStatus.gps == 0) {
        logInfo("GPS: disabled");
    } else if (radioStatus.gps == 1) {
        logInfo("GPS: enabled");
    } else {
        logInfo("GPS: not available");
    }
    // Closing banner
    logInfo("*******************************************");    
}

int MTSCellularInterface::sendSMS(const char *phoneNumber, const char *message, int messageSize){
    if (_radio.sendSMS(phoneNumber, message, messageSize) != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

std::vector<MTSCellularRadio::Sms> MTSCellularInterface::getReceivedSms(){
    return _radio.getReceivedSms();
}


int MTSCellularInterface::deleteAllReceivedSms(){
    if (_radio.deleteAllReceivedSms() != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::deleteOnlyReceivedReadSms(){
    if (_radio.deleteOnlyReceivedReadSms() != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}	

int MTSCellularInterface::GPSenable(){
    if (_radio.GPSenable() != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::GPSdisable(){
    if (_radio.GPSdisable() != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

bool MTSCellularInterface::GPSenabled(){
    return _radio.GPSenabled();
}
        
MTSCellularRadio::gpsData MTSCellularInterface::GPSgetPosition(){
    return _radio.GPSgetPosition();
}

bool MTSCellularInterface::GPSgotFix(){
    return _radio.GPSgotFix();    
}	


struct cellular_socket {
    int id;
    nsapi_protocol_t proto;
    bool connected;
    SocketAddress addr;
};

int MTSCellularInterface::socket_open(void **handle, nsapi_protocol_t proto){
    // Look for an unused socket
    int id = -1;
 
    for (int i = 1; i <= MAX_SOCKET_COUNT; i++) {
        if (!_ids[i]) {
            id = i;
            _ids[i] = true;
            break;
        }
    }
 
    if (id == -1) {
        return NSAPI_ERROR_NO_SOCKET;
    }
    
    struct cellular_socket *socket = new struct cellular_socket;
    if (!socket) {
        return NSAPI_ERROR_NO_SOCKET;
    }
    
    socket->id = id;
    socket->proto = proto;
    socket->connected = false;
    *handle = socket;
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::socket_close(void *handle){
    struct cellular_socket *socket = (struct cellular_socket *)handle;
    int err = NSAPI_ERROR_OK;
 
    if (_radio.close(socket->id) != MTSCellularRadio::MTS_SUCCESS) {
        err = NSAPI_ERROR_DEVICE_ERROR;
    }

    _ids[socket->id] = false;
    delete socket;
    return err;
}

int MTSCellularInterface::socket_bind(void *handle, const SocketAddress &address){
    return NSAPI_ERROR_UNSUPPORTED;
}

int MTSCellularInterface::socket_listen(void *handle, int backlog){
    return NSAPI_ERROR_UNSUPPORTED;
}

int MTSCellularInterface::socket_connect(void *handle, const SocketAddress &address){
    struct cellular_socket *socket = (struct cellular_socket *)handle;

    const char *proto = (socket->proto == NSAPI_UDP) ? "UDP" : "TCP";
    int result = _radio.open(proto, socket->id, address.get_ip_address(), address.get_port());
    switch (result) {
        case MTSCellularRadio::MTS_NO_CONNECTION:
            return NSAPI_ERROR_NO_CONNECTION;
        case MTSCellularRadio::MTS_SUCCESS:
            socket->connected = true;
            return NSAPI_ERROR_OK;
        default: 
            return NSAPI_ERROR_DEVICE_ERROR;
    }
}

int MTSCellularInterface::socket_accept(void *handle, void **socket, SocketAddress *address){
    return 0;
}

int MTSCellularInterface::socket_send(void *handle, const void *data, unsigned size){
    struct cellular_socket *socket = (struct cellular_socket *)handle;

    int sent = _radio.send(socket->id, data, size);
    if (sent > 0){
        return sent;
    }
    switch (sent) {
        case MTSCellularRadio::MTS_NO_CONNECTION:
            return NSAPI_ERROR_NO_CONNECTION;
        case MTSCellularRadio::MTS_SOCKET_CLOSED:
            return NSAPI_ERROR_NO_SOCKET;
        default:
            return NSAPI_ERROR_DEVICE_ERROR;
    }
}

int MTSCellularInterface::socket_recv(void *handle, void *data, unsigned size){
    struct cellular_socket *socket = (struct cellular_socket *)handle;

    int rcv = _radio.receive(socket->id, data, size);
    if (rcv > 0){
        return rcv;
    }
    switch (rcv) {
        case MTSCellularRadio::MTS_NO_CONNECTION:
            return NSAPI_ERROR_NO_CONNECTION;
        case MTSCellularRadio::MTS_SOCKET_CLOSED:
            return NSAPI_ERROR_NO_SOCKET;
        default:
            return NSAPI_ERROR_DEVICE_ERROR;
    }
}

int MTSCellularInterface::socket_sendto(void *handle, const SocketAddress &address, const void *data, unsigned size){
    return 0;
}

int MTSCellularInterface::socket_recvfrom(void *handle, SocketAddress *address, void *buffer, unsigned size){
    return 0;
}

void MTSCellularInterface::socket_attach(void *handle, void (*callback)(void *), void *data){
    logInfo("MTSCellularInterface socket_attach called");    
    struct cellular_socket *socket = (struct cellular_socket *)handle;    
    _cbs[socket->id].callback = callback;
    _cbs[socket->id].data = data;
}

void MTSCellularInterface::event() {
    for (int i = 1; i <= MAX_SOCKET_COUNT; i++) {
        if (_cbs[i].callback) {
            _cbs[i].callback(_cbs[i].data);
        }
    }
}

