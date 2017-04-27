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

/*bool MTSCellularInterface::radioPower(Power option){
    return true;
}
*/
int MTSCellularInterface::set_credentials(const char *apn, const char *username, const char *password){
    return _radio.pdpContext(apn);
}

int MTSCellularInterface::connect(const char *apn, const char *username, const char *password){
    int result;
    result = _radio.pdpContext(apn);
    if (result != NSAPI_ERROR_OK) {
        return result;
    }
    return _radio.connect();
}
    
int MTSCellularInterface::connect(){
    return _radio.connect();
}
     
int MTSCellularInterface::disconnect(){
    return _radio.disconnect();
}
/*
bool MTSCellularInterface::isConnected(){
    return true;
}
*/

const char *MTSCellularInterface::get_ip_address()
{
    return _radio.getIPAddress();
}

/*
const char *MTSCellularInterface::get_mac_address()
{
    return _radio.getMACAddress();
}

const char *MTSCellularInterface::get_gateway()
{
    return _radio.getGateway();
}

const char *MTSCellularInterface::get_netmask()
{
    return _radio.getNetmask();
}


void MTSCellularInterface::reset(){
    return;
}

bool MTSCellularInterface::ping(const char *address){
    return true;
} 	
	
Code MTSCellularInterface::sendSMS(const char *phoneNumber, const char *message){
    return MTS_SUCCESS;
}

Code MTSCellularInterface::sendSMS(const Sms& sms){
    return MTS_SUCCESS;    
}

std::vector<Sms> MTSCellularInterface::getReceivedSms(){
    std::vector<Sms> vSms;
    return vSms;
}

Code MTSCellularInterface::deleteAllReceivedSms(){
    return MTS_SUCCESS;
}

Code MTSCellularInterface::deleteOnlyReceivedReadSms(){
    return MTS_SUCCESS;
}	

bool MTSCellularInterface::GPSenable(){
    return true;
}

bool MTSCellularInterface::GPSdisable(){
    return true;
}

bool MTSCellularInterface::GPSenabled(){
    return true;
}
        
gpsData MTSCellularInterface::GPSgetPosition(){
    gpsData response;
    response.success = true;
    return response;
}

bool MTSCellularInterface::GPSgotFix(){
    return true;    
}	
*/

struct cellular_socket {
    int id;
    nsapi_protocol_t proto;
    bool connected;
    SocketAddress addr;
};

int MTSCellularInterface::socket_open(void **handle, nsapi_protocol_t proto){
    // Look for an unused socket
    int id = -1;
 
    for (int i = 0; i < CELLULAR_SOCKET_COUNT; i++) {
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
    
    socket->id = id+1;  // +1 the first telit socket available starts at 1.
    socket->proto = proto;
    socket->connected = false;
    *handle = socket;
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::socket_close(void *handle){
    struct cellular_socket *socket = (struct cellular_socket *)handle;
    int err = NSAPI_ERROR_OK;
 
    if (!_radio.close(socket->id)) {
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
    if (!_radio.open(proto, socket->id, address.get_ip_address(), address.get_port())) {
        logInfo("socket_connect error");
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    logInfo("socket_connect success");
    
    socket->connected = true;
    return NSAPI_ERROR_OK;
}

int MTSCellularInterface::socket_accept(void *handle, void **socket, SocketAddress *address){
    return 0;
}

int MTSCellularInterface::socket_send(void *handle, const void *data, unsigned size){
    struct cellular_socket *socket = (struct cellular_socket *)handle;

    if (_radio.send(socket->id, data, size)){
        return size;
    }
    return NSAPI_ERROR_DEVICE_ERROR;
}

int MTSCellularInterface::socket_recv(void *handle, void *data, unsigned size){
    struct cellular_socket *socket = (struct cellular_socket *)handle;

    return _radio.receive(socket->id, data, size);
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
    for (int i = 0; i < CELLULAR_SOCKET_COUNT; i++) {
        if (_cbs[i].callback) {
            _cbs[i].callback(_cbs[i].data);
        }
    }
}

