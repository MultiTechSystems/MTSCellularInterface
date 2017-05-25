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

bool MTSCellularInterface::power_on(int timeout){
    return _radio.power_on(timeout);
}
  
int MTSCellularInterface::power_off(){
    _radio.power_off();
    return NSAPI_ERROR_OK;
}

bool MTSCellularInterface::is_powered(){
    return _radio.is_powered();
}

int MTSCellularInterface::set_credentials(const char *apn, const char *username, const char *password){
    int result = _radio.set_pdp_context(apn);
    if (result == MTSCellularRadio::MTS_NOT_ALLOWED){
        return NSAPI_ERROR_UNSUPPORTED;
    }
    if (result != MTSCellularRadio::MTS_SUCCESS){
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::send_basic_command(const std::string& command, unsigned int timeoutMillis)
{
    if (_radio.send_basic_command(command, timeoutMillis) != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

std::string MTSCellularInterface::send_command(const std::string& command, unsigned int timeoutMillis, char esc){
    return _radio.send_command(command, timeoutMillis, esc);
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
    return _radio.get_ip_address().c_str();
}

bool MTSCellularInterface::is_registered()
{
    int registration = _radio.get_registration();
    if (registration == MTSCellularRadio::REGISTERED || registration == MTSCellularRadio::ROAMING){
        return true;
    }    
    return false;
}

MTSCellularRadio::statusInfo MTSCellularInterface::get_radio_status()
{
    return _radio.get_radio_status();
}

void MTSCellularInterface::log_radio_status(){
    MTSCellularRadio::statusInfo radioStatus = _radio.get_radio_status();
    // Opening banner
    logInfo("");    // "leading cr/lf"
    logInfo("************ Radio Information ************");
    
    // Radio model
    logInfo("Radio: %s", radioStatus.model.c_str());

    if (_radio.get_radio_type() != MTSCellularRadio::MTQ_C2 && _radio.get_radio_type() != MTSCellularRadio::MTQ_EV3) {
        // SIM status
        if (radioStatus.sim) {
            logInfo("SIM status: Inserted");
        } else {
            logInfo("SIM status: Not inserted");
        }

        // APN
        logInfo("APN: \r\n%s", radioStatus.apn.c_str());
    }
    
    // Signal strength
    logInfo("Signal strength: %d", radioStatus.rssi);

    // Network registration
    logInfo("Registration: %s", _radio.get_registration_names(radioStatus.registration).c_str());

    // Connection status and IP address if connected
    if (radioStatus.connection) {
        logInfo("Cellular connection: active");
        logInfo("IP address: %s", radioStatus.ip_address.c_str());
    } else {
        logInfo("Cellular connection: not active");
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
    logInfo("*******************************************\r\n");
}

int MTSCellularInterface::send_sms(const std::string& phone_number, const std::string& message){
    if (_radio.send_sms(phone_number, message) != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::send_sms(const MTSCellularRadio::Sms& sms)
{
    return send_sms(sms.phone_number, sms.message);
}

std::vector<MTSCellularRadio::Sms> MTSCellularInterface::get_received_sms(){
    return _radio.get_received_sms();
}


int MTSCellularInterface::delete_all_received_sms(){
    if (_radio.delete_all_received_sms() != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::delete_only_read_sms(){
    if (_radio.delete_only_read_sms() != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}	

int MTSCellularInterface::gps_enable(){
    if (_radio.gps_enable() == MTSCellularRadio::MTS_NOT_SUPPORTED) {
        return NSAPI_ERROR_UNSUPPORTED;
    }
    if (_radio.gps_enable() != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::gps_disable(){
    if (_radio.gps_disable() == MTSCellularRadio::MTS_NOT_SUPPORTED) {
        return NSAPI_ERROR_UNSUPPORTED;
    }    
    if (_radio.gps_disable() != MTSCellularRadio::MTS_SUCCESS) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return NSAPI_ERROR_OK;
}

bool MTSCellularInterface::is_gps_enabled(){
    return _radio.is_gps_enabled();
}
        
MTSCellularRadio::gpsData MTSCellularInterface::gps_get_position(){
    return _radio.gps_get_position();
}

bool MTSCellularInterface::gps_has_fix(){
    return _radio.gps_has_fix();    
}	

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
    if (socket) {
        delete socket;
    }
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
    if (rcv >= 0){
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

int MTSCellularInterface::socket_sendto(void *handle, const SocketAddress &address, const void *data, unsigned size) {
    struct cellular_socket *socket = (struct cellular_socket *)handle;

    if (socket->connected && socket->addr != address) {
        if (!_radio.close(socket->id)) {
            return NSAPI_ERROR_DEVICE_ERROR;
        }
        socket->connected = false;
    }

    if (!socket->connected) {
        int err = socket_connect(socket, address);
        if (err < 0) {
            return err;
        }
        socket->addr = address;
    }
    
    return socket_send(socket, data, size);
}

int MTSCellularInterface::socket_recvfrom(void *handle, SocketAddress *address, void *data, unsigned size) {
    struct cellular_socket *socket = (struct cellular_socket *)handle;
    int ret = socket_recv(socket, data, size);
    if (ret >= 0 && address) {
        *address = socket->addr;
    }

    return ret;
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

