/* Cellular radio implementation of NetworkInterface API
*
*/

#include <string.h>
#include "MTSCellularInterface.h"

// Various timeouts for cellular radio operations
#define CELL_RADIO_CONNECT_TIMEOUT 15000
#define CELL_RADIO_SEND_TIMEOUT    500
#define CELL_RADIO_RECV_TIMEOUT    0
#define CELL_RADIO_MISC_TIMEOUT    500

// MTSCellularInterface implementation
MTSCellularInterface::MTSCellularInterface(PinName tx, PinName rx/*, PinName rts, PinName cts,
	PinName dcd, PinName dsr, PinName dtr, PinName ri, PinName power, PinName reset*/)
	: _radio(tx, rx/*, rts, cts, dcd, dsr, dtr, ri, power, reset*/)
{

}

bool MTSCellularInterface::radioPower(Power option){
    return true;
}

int MTSCellularInterface::set_credentials(const char *apn, const char *username, const char *password){
    return 0;
}

int MTSCellularInterface::connect(const char *apn, const char *username, const char *password){
    return 0;
}
    
int MTSCellularInterface::connect(){
    return 0;
}
     
int MTSCellularInterface::disconnect(){
    return 0;
}

bool MTSCellularInterface::isConnected(){
    return true;
}

const char *MTSCellularInterface::get_ip_address()
{
    return _radio.getIPAddress();
}

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

bool MTSCellularInterface::ping(const std::string& address){
    return true;
} 	
	
Code MTSCellularInterface::sendSMS(const std::string& phoneNumber, const std::string& message){
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

int MTSCellularInterface::socket_open(void **handle, nsapi_protocol_t proto){
    return 0;
}

int MTSCellularInterface::socket_close(void *handle){
    return 0;
}

int MTSCellularInterface::socket_bind(void *handle, const SocketAddress &address){
    return 0;
}

int MTSCellularInterface::socket_listen(void *handle, int backlog){
    return 0;
}

int MTSCellularInterface::socket_connect(void *handle, const SocketAddress &address){
    return 0;
}

int MTSCellularInterface::socket_accept(void *handle, void **socket, SocketAddress *address){
    return 0;
}

int MTSCellularInterface::socket_send(void *handle, const void *data, unsigned size){
    return 0;
}

int MTSCellularInterface::socket_recv(void *handle, void *data, unsigned size){
    return 0;
}

int MTSCellularInterface::socket_sendto(void *handle, const SocketAddress &address, const void *data, unsigned size){
    return 0;
}

int MTSCellularInterface::socket_recvfrom(void *handle, SocketAddress *address, void *buffer, unsigned size){
    return 0;
}

void MTSCellularInterface::socket_attach(void *handle, void (*callback)(void *), void *data){
    return;
}
