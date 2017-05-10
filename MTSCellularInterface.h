/* MTSCellularInterface API
 *
 */

#ifndef MTSCELLULARINTERFACE_H
#define MTSCELLULARINTERFACE_H

#include "mbed.h"
#include "MTSCellularRadio.h"

class MTSCellularInterface : public NetworkStack, public CellularInterface
{
public:
	MTSCellularInterface(PinName Radio_tx = RADIO_TX, PinName Radio_rx = RADIO_RX/*, PinName Radio_rts = NC, PinName Radio_cts = NC,
		PinName Radio_dcd = NC,	PinName Radio_dsr = NC, PinName Radio_dtr = NC, PinName Radio_ri = NC,
		PinName Radio_Power = NC, PinName Radio_Reset = NC*/);

    /** Power the modem on or off.
    * Power off closes any open sockets, disconnects from the cellular network then powers the modem off.
    * Power on powers up the modem and verifies AT command response.
    */
//	virtual bool radioPower(Power option);


    /** Set the cellular network APN and credentials
    *
    *  @param apn      name of the network to connect to
    *  @param user     Optional username for the APN
    *  @param pass     Optional password fot the APN
    *  @return         0 on success, negative error code on failure
    */
    virtual int set_credentials(const char *apn, const char *username = "", const char *password = "");

    /** A method for sending a basic AT command to the radio. A basic AT command is
	* one that simply has a response of OK or ERROR.
	*
	* @param command string to send to the radio.
	* @param timeoutMillis the time in millis to wait for a response before returning. If
	*   OK or ERROR are detected in the response the timer is short circuited.
	* @returns 0 for success or a negative number for a failure.
	*/
	virtual int sendBasicCommand(const std::string& command, unsigned int timeoutMillis = 1000);
	
    //Cellular Radio Specific
    /** A method for sending AT commands to the radio.
	*
	* @param command string to send to the radio.
	* @param timeoutMillis the time in millis to wait for a response before returning. If
	*   OK or ERROR are detected in the response the timer is short circuited.
	* @param esc escape character to add at the end of the command, defaults to
	*   carriage return (CR).  Does not append any character if esc == 0.
	* @returns a string containing the response to the command. The string will be empty upon failure.
	*/
    virtual std::string sendCommand(const std::string& command, unsigned int timeoutMillis = 1000, char esc = CR);

    /** Make cellular connection
    *
    *  @param apn      name of the network to connect to
    *  @param username Optional username for your APN
    *  @param password Optional password for your APN 
    *  @return         0 on success, negative error code on failure
    */
    virtual int connect(const char *apn, const char *username = "", const char *password = "");
	
    /** Make cellular connection
    *
    *  @return         0 on success, negative error code on failure
    */
	virtual int connect();
     
    /** Cellular disconnect
    *
    *  @return         0 on success, negative error code on failure
    */
	virtual int disconnect();

    /** Get the internally stored IP address
    *
    *  @return             IP address of the interface or null if not yet connected
    */
    virtual const char *get_ip_address();

    /** Get the internally stored MAC address
    *	
    *  @return             MAC address of the interface
    */
//    virtual const char *get_mac_address();

     /** Get the local gateway
     *
     *  @return         Null-terminated representation of the local gateway
     *                  or null if no network mask has been recieved
     */
//    virtual const char *get_gateway();

    /** Get the local network mask
     *
     *  @return         Null-terminated representation of the local network mask
     *                  or null if no network mask has been recieved
     */
     
//    virtual const char *get_netmask();

    /** Checks if the radio is connected to the cell network.
    * Checks antenna signal, cell tower registration, and context activation
    * before finally pinging (4 pings, 32 bytes each) to confirm PPP connection
    * to network. Will return true if there is an open socket connection as well.
    *
    * @returns true if there is a PPP connection to the cell network, false
    * if there is no PPP connection to the cell network.
    */
//    virtual bool isConnected();

    /** Resets the radio/modem.
    * Disconnects all active PPP and socket connections to do so.
    */
//    virtual void reset();

    /** Pings specified DNS or IP address
     * Google DNS server used as default ping address
     * @returns true if ping received alive response else false
     */
//    virtual bool ping(const char *address = "8.8.8.8"); 	

    /** Gets information about the radio and prints it to the debug port.
    *
    * @returns 
    */
	void logRadioStatus();
	
    /** This method is used to send an SMS message.
    *
    * @param phoneNumber the phone number to send the message to as a string.
    * @param message the text message to be sent.
    * @param size of message to be sent.
    *  @return 0 on success, negative error code on failure
    */
    virtual int sendSMS(const char *phoneNumber, const char *message, int messageSize);

    /** This method retrieves all of the SMS messages currently available for
    * this phone number.
    *
    * @returns a vector of existing SMS messages each as an Sms struct.
    */
    virtual std::vector<MTSCellularRadio::Sms> getReceivedSms();

    /** This method can be used to remove/delete all received SMS messages
    * even if they have never been retrieved or read.
    *
    *  @return 0 on success, negative error code on failure
    */
    virtual int deleteAllReceivedSms();

    /** This method can be used to remove/delete all received SMS messages
    * that have been retrieved by the user through the getReceivedSms method.
    * Messages that have not been retrieved yet will be unaffected.
    *
    *  @return 0 on success, negative error code on failure
    */
    virtual int deleteOnlyReceivedReadSms();	

    /** Enables GPS.
    *
    *  @return 0 on success, negative error code on failure
	*/
    virtual int GPSenable();

    /** Disables GPS.
    *
    *  @return 0 on success, negative error code on failure
    */
    virtual int GPSdisable();

    /** Checks if GPS is enabled.
    *
    * @returns true if GPS is enabled, false if GPS is disabled.
    */
    virtual bool GPSenabled();
        
    /** Get GPS position.
    *
    * @returns a structure containing the GPS data field information.
    */
    virtual MTSCellularRadio::gpsData GPSgetPosition();

    /** Check for GPS fix.
    *
    * @returns true if there is a fix and false otherwise.
    */
    virtual bool GPSgotFix();	

	/** Translates a hostname to an IP address with specific version
	 *
	 *	The hostname may be either a domain name or an IP address. If the
	 *	hostname is an IP address, no network transactions will be performed.
	 *
	 *	If no stack-specific DNS resolution is provided, the hostname
	 *	will be resolve using a UDP socket on the stack.
	 *
	 *	@param address	Destination for the host SocketAddress
	 *	@param host 	Hostname to resolve
	 *	@param version	IP version of address to resolve, NSAPI_UNSPEC indicates
	 *					version is chosen by the stack (defaults to NSAPI_UNSPEC)
	 *	@return 		0 on success, negative error code on failure
	 */
//	using NetworkInterface::gethostbyname;

	/** Add a domain name server to list of servers to query
	 *
	 *	@param addr 	Destination for the host address
	 *	@return 		0 on success, negative error code on failure
	 */
//	using NetworkInterface::add_dns_server;

protected:
	/** Open a socket
	*  @param handle       Handle in which to store new socket
	*  @param proto        Type of socket to open, NSAPI_TCP or NSAPI_UDP
	*  @return             0 on success, negative on failure
	*/
	virtual int socket_open(void **handle, nsapi_protocol_t proto);

	/** Close the socket
	*  @param handle       Socket handle
	*  @return             0 on success, negative on failure
	*  @note On failure, any memory associated with the socket must still
	*        be cleaned up
	*/
	virtual int socket_close(void *handle);

	/** Bind a server socket to a specific port
	*  @param handle       Socket handle
	*  @param address      Local address to listen for incoming connections on
	*  @return             0 on success, negative on failure.
	*/
	virtual int socket_bind(void *handle, const SocketAddress &address);

	/** Start listening for incoming connections
	*  @param handle       Socket handle
	*  @param backlog      Number of pending connections that can be queued up at any
	*                      one time [Default: 1]
	*  @return             0 on success, negative on failure
	*/
	virtual int socket_listen(void *handle, int backlog);

	/** Connects this TCP socket to the server
	*  @param handle       Socket handle
	*  @param address      SocketAddress to connect to
	*  @return             0 on success, negative on failure
	*/
	virtual int socket_connect(void *handle, const SocketAddress &address);

	/** Accept a new connection.
	*  @param handle       Handle in which to store new socket
	*  @param server       Socket handle to server to accept from
	*  @return             0 on success, negative on failure
	*  @note This call is not-blocking, if this call would block, must
	*        immediately return NSAPI_ERROR_WOULD_WAIT
	*/
	virtual int socket_accept(void *handle, void **socket, SocketAddress *address);

	/** Send data to the remote host
	*  @param handle       Socket handle
	*  @param data         The buffer to send to the host
	*  @param size        The length of the buffer to send
	*  @return             Number of written bytes on success, negative on failure
	*  @note This call is not-blocking, if this call would block, must
	*		 immediately return NSAPI_ERROR_WOULD_WAIT
	*/

	virtual int socket_send(void *handle, const void *data, unsigned size);

	/** Receive data from the remote host
	*  @param handle       Socket handle
	*  @param handle	   Socket handle
	*  @param data       The buffer in which to store the data received from the host
	*  @param size       The maximum length of the buffer
	*  @return            Number of received bytes on success, negative on failure
	*  @note This call is not-blocking, if this call would block, must
	*		 immediately return NSAPI_ERROR_WOULD_WAIT
	*/

	virtual int socket_recv(void *handle, void *data, unsigned size);

	/** Send a packet to a remote endpoint
	*  @param handle       Socket handle
	*  @param address      The remote SocketAddress
	*  @param data         The packet to be sent
	*  @param size         The length of the packet to be sent
	*  @return             The number of written bytes on success, negative on failure
	*  @note This call is not-blocking, if this call would block, must
	*        immediately return NSAPI_ERROR_WOULD_WAIT
	*/
	virtual int socket_sendto(void *handle, const SocketAddress &address, const void *data, unsigned size);

	/** Receive a packet from a remote endpoint
	*  @param handle       Socket handle
	*  @param address      Destination for the remote SocketAddress or null
	*  @param buffer       The buffer for storing the incoming packet data
	*                      If a packet is too long to fit in the supplied buffer,
	*                      excess bytes are discarded
	*  @param size         The length of the buffer
	*  @return             The number of received bytes on success, negative on failure
	*  @note This call is not-blocking, if this call would block, must
	*        immediately return NSAPI_ERROR_WOULD_WAIT
	*/
	virtual int socket_recvfrom(void *handle, SocketAddress *address, void *buffer, unsigned size);

	/** Register a callback on state change of the socket
	*  @param handle       Socket handle
	*  @param callback     Function to call on state change
	*  @param data         Argument to pass to callback
	*  @note Callback may be called in an interrupt context.
	*/
	virtual void socket_attach(void *handle, void (*callback)(void *), void *data);

	/** Provide access to the NetworkStack object
	*
	*  @return The underlying NetworkStack object
	*/
	virtual NetworkStack *get_stack()
	{
		return this;
	}

private:
	MTSCellularRadio _radio;
    bool _ids[MAX_SOCKET_COUNT+1];

    void event();

	struct {
		void (*callback)(void *);
		void *data;
	} _cbs[MAX_SOCKET_COUNT+1];

	
};
#endif //MTSCELLULARINTERFACE

