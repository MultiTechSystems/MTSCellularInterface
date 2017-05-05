/* MTSCellularRadio object definition
 *
*/

#ifndef MTSCELLULARRADIO_H
#define MTSCELLULARRADIO_H

#include "ATParser.h"
#include <string>

//Special Payload Character Constants (ASCII Values)
const char CR	  = 0x0D;	//Carriage Return
const char LF	  = 0x0A;	//Line Feed
const char CTRL_Z = 0x1A;	//Control-Z

#define MAX_SOCKET_COUNT 2

class MTSCellularRadio
{
public:
	MTSCellularRadio(PinName tx, PinName rx/*, PinName Radio_rts, PinName Radio_cts, PinName Radio_dcd,
		PinName Radio_dsr, PinName Radio_dtr, PinName Radio_ri, PinName Radio_Power, PinName Radio_Reset*/);

    // Enumeration for different cellular radio types.
    enum Radio {
        NA, MTQ_H5, MTQ_EV3, MTQ_C2, MTQ_LAT3, MTQ_LVW3
    };

    // An enumeration of radio registration states with a cell tower.
    enum Registration {
        NOT_REGISTERED, REGISTERED, SEARCHING, DENIED, UNKNOWN, ROAMING
    };

	// An enumeration for radio responses.
	enum Code {
		MTS_SUCCESS = 0, MTS_ERROR = -1, MTS_FAILURE = -2, MTS_NO_RESPONSE = -3, MTS_NO_CONNECTION = -4, 
		MTS_NO_SOCKET = -5, MTS_SOCKET_CLOSED = -6, MTS_NOT_REGISTERED = -7, MTS_NO_SIGNAL = -8
	};

    // This structure contains the data from a received SMS message.
    struct Sms {
        // Message Phone Number
        std::string phoneNumber;
        // Message Body
        std::string message;
        // Message Timestamp
        std::string timestamp;
    };	

    // This structure contains the data for GPS position.
    struct gpsData {
		bool success;
		// Format is ddmm.mmmm N/S. Where: dd - degrees 00..90; mm.mmmm - minutes 00.0000..59.9999; N/S: North/South.
		std::string latitude;
		// Format is dddmm.mmmm E/W. Where: ddd - degrees 000..180; mm.mmmm - minutes 00.0000..59.9999; E/W: East/West.
		std::string longitude;
		// Horizontal Diluition of Precision.
		float hdop;
		// Altitude - mean-sea-level (geoid) in meters.
		float altitude;
		// 0 or 1 - Invalid Fix; 2 - 2D fix; 3 - 3D fix.
		int fix;
		// Format is ddd.mm - Course over Ground. Where: ddd - degrees 000..360; mm - minutes 00..59.
		std::string cog;
		// Speed over ground (Km/hr).
		float kmhr;
		// Speed over ground (knots).
		float knots;
		// Total number of satellites in use.
		int satellites;
		// Date and time in the format YY/MM/DD,HH:MM:SS.
		std::string timestamp;
	};   

    /** Controls radio power.
    	* Before power is removed, the connection must be brought down.
    	*
    	* @returns true if successful, false if a failure was detected.
    	*/
//    bool power(Power option);
    	
	/** Sets up the physical connection pins
	*   (CTS, RTS, DCD, DTR, DSR, RI and RESET)
	*/
//    bool configureSignals(unsigned int CTS = NC, unsigned int RTS = NC, unsigned int DCD = NC, unsigned int DTR = NC,
//    	unsigned int RESET = NC, unsigned int DSR = NC, unsigned int RI = NC);

    /** A method for testing command access to the radio.  This method sends the
	* command "AT" to the radio, which is a standard radio test to see if you
	* have command access to the radio.  The function returns when it receives
	* the expected response from the radio.
	*
	* @returns the standard AT Code enumeration.
	*/
//    virtual Code test();

    /** A method for getting the signal strength of the radio. This method allows you to
	* get a value that maps to signal strength in dBm. Here 0-1 is Poor, 2-9 is Marginal,
	* 10-14 is Ok, 15-19 is Good, and 20+ is Excellent.  If you get a result of 99 the
	* signal strength is not known or not detectable.
	*
	* @returns an integer representing the signal strength or -1 if command fails.
	*/
    virtual int getSignalStrength();

    /** This method is used to check the registration state of the radio with the cell tower.
	* If not appropriatley registered with the tower you cannot make a cellular connection.
	*
	* @returns the registration state as an enumeration type or -1 if the command fails.
	*/
    virtual int getRegistration();

    /** This method is used to configure the radio PDP context. Some radio models require
    * the APN be set correctly before it can make a data connection. The APN for your SIM
	* can be obtained by contacting your cellular service provider.
	*
	* @param the APN as a string.
	* @returns 0 on success, a negative value upon failure.
	*/
    virtual int pdpContext(const std::string& apn);

    /** This method is used to set the DNS which enables the use of URLs instead
	* of IP addresses when making a socket connection.
	*
	* @param the DNS server address as a string in form xxx.xxx.xxx.xxx.
	* @returns the standard AT Code enumeration.
	*/
//    virtual Code setDns(const std::string& primary, const std::string& secondary = "0.0.0.0");

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

    /** A static method for getting a string representation for the Registration
	* enumeration.
	*
	* @param code a Registration enumeration.
	* @returns the enumeration name as a string.
	*/
//    static std::string getRegistrationNames(Registration registration);

    /** A static method for getting a string representation for the Radio
	* enumeration.
	*
	* @param type a Radio enumeration.
	* @returns the enumeration name as a string.
	*/
//    static std::string getRadioNames(Radio radio);
    
    /** A method for changing the echo commands from radio.
	* @param state Echo mode is off (an argument of 1 turns echos off, anything else turns echo on)
	* @returns standard Code enumeration
	*/
//    virtual Code echo(bool state);

	/** Gets the device IP
	* @returns string containing the IP address
	*/
//	virtual std::string getDeviceIP();

	/** Get the device IMEI or MEID (whichever is available)
	* @returns string containing the IMEI for GSM, the MEID for CDMA, or an empty string
	* if it failed to parse the number.
	*/
//	std::string getEquipmentIdentifier();

	/** Get string representation of radio type
	* @returns string containing the radio type (MTQ-H5, etc)
	*/
//	std::string getRadioType();

	/** Cellular connect / context activation.
	*
	* @returns 0 on connection success, else a negative value.
	*/
	virtual int connect();
    
    /** Cellular disconnect / context deactivation.	
    * Closes any and all sockets before disconnect
    *
    * @returns 0 on disconnect success, else a negative value.
    */
	virtual int disconnect();	    		

    /** Checks if the radio is connected to the cell network.
    * Checks context activation.
    *
    * @returns true if connected, false if disconnected.
    */
    virtual bool isConnected();

    /** Checks if the radio's APN is set
    *
    * @returns true if set, false if empty.
    */
	virtual bool isAPNset();

    /**
    * Get the radio's IP address
    *
    * @return a string containing the IP address or an empty string if no IP address is assigned
    */
    std::string getIPAddress(void);

    /**
    * Attach a function to call whenever network state has changed
    *
    * @param func A pointer to a void function, or 0 to set as none
    */
    void attach(Callback<void()> func);

    /**
    * Attach a function to call whenever network state has changed
    *
    * @param obj pointer to the object to call the member function on
    * @param method pointer to the member function to call
    */
    template <typename T, typename M>
    void attach(T *obj, M method) {
        attach(Callback<void()>(obj, method));
    }

    /**
    * Get the MAC address of ESP8266
    *
    * @return null-terminated MAC address or null if no MAC address is assigned
    */
//    const char *getMACAddress(void);

     /** Get the local gateway
     *
     *  @return         Null-terminated representation of the local gateway
     *                  or null if no network mask has been recieved
     */
//    const char *getGateway();

    /** Get the local network mask
     *
     *  @return         Null-terminated representation of the local network mask 
     *                  or null if no network mask has been recieved
     */
//    const char *getNetmask();

    /** Open a socketed connection
    *
    * @param type the type of socket to open "UDP" or "TCP"
    * @param id id to give the new socket, valid 0-4
    * @param port port to open connection with
    * @param addr the IP address of the destination
    * @return 0 if socket opened successfully otherwise a negative value.
    */
    int open(const char *type, int id, const char* addr, int port);

    /** Send socket data
    *
    * @param socket id
    * @param buffer of data to send
    * @param amount of data in buffer
    * @return amount of data successfully sent or negative value on failure.
    */
	int send(int id, const void *data, uint32_t amount);

    /** Receive socket data
    *
    * @param socket id
    * @param receive data buffer
    * @param amount of space in receive buffer
    * @return amount of data loaded into receive buffer on success or negative value on failure.
    */
	int receive(int id, void *data, uint32_t amount);

	/** Close a socket.
	*
	*@param id is the socket to close.
	*@return true if the socket closed successfully otherwise return false.
	*/
	bool close(int id);

	/** Check for an open socket.
	*
	*@param id is the socket to check.
	*@return true if the socket is not closed otherwise return false.
	*/
	bool isSocketOpen(int id);
	
	/** Pings specified DNS or IP address
	 * Google DNS server used as default ping address
	 * @returns true if ping received alive response else false
	 */
//	virtual bool ping(const std::string& address = "8.8.8.8");	

    /** This method is used to send an SMS message. Note that you cannot send an
    * SMS message and have a data connection open at the same time.
    *
    * @param phoneNumber the phone number to send the message to as a string.
    * @param message the text message to be sent.
    * @param size of the message to be sent.
    * @returns the standard AT Code enumeration.
    */
    virtual int sendSMS(const char *phoneNumber, const char *message, int messageSize);

    /** This method retrieves all of the SMS messages currently available for
    * this phone number.
    *
    * @returns a vector of existing SMS messages each as an Sms struct.
    */
    virtual std::vector<Sms> getReceivedSms();

    /** This method can be used to remove/delete all received SMS messages
    * even if they have never been retrieved or read.
    *
    * @return 0 on success otherwise a negative value.
    */
    virtual int deleteAllReceivedSms();

    /** This method can be used to remove/delete all received SMS messages
    * that have been retrieved by the user through the getReceivedSms method.
    * Messages that have not been retrieved yet will be unaffected.
    *
    * @return 0 on success otherwise a negative value.
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
    virtual gpsData GPSgetPosition();

    /** Check for GPS fix.
    *
    * @returns true if there is a fix and false otherwise.
    */
    virtual bool GPSgotFix();	

protected:
	BufferedSerial _serial;
	ATParser _parser;
	std::string _ipAddress;

    Radio _type;				//The type of radio being used
    std::string _cid;		//context ID=1 for most radios. Verizon LTE LVW2&3 use cid 3.

    bool _echoMode; 			//Specifies if the echo mode is currently enabled.
    bool _pppConnected; 		//Specifies if a PPP session is currently connected.
    //Mode socketMode; 		//The current socket Mode.
    bool _socketOpened; 		//Specifies if a Socket is presently opened.
    bool _socketCloseable; 	//Specifies is a Socket can be closed.
    
	DigitalIn* radio_cts;	//Maps to the radio's cts signal
	DigitalOut* radio_rts;	//Maps to the radio's rts signal
    DigitalIn* radio_dcd;	//Maps to the radio's dcd signal
    DigitalIn* radio_dsr;	//Maps to the radio's dsr signal
    DigitalIn* radio_ri;	//Maps to the radio's ring indicator signal
    DigitalOut* radio_dtr;	//Maps to the radio's dtr signal

    DigitalOut* resetLine;	//Maps to the radio's reset signal 
};

#endif // MTSCELLULARRADIO_H

