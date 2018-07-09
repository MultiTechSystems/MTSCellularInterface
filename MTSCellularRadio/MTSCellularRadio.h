/* MTSCellularRadio object definition
 *
*/

#ifndef MTSCELLULARRADIO_H
#define MTSCELLULARRADIO_H

#include "ATCmdParser.h"
#include <string>
#include <vector>
#include "FileHandle.h"

//Special Payload Character Constants (ASCII Values)
const char CR	  = 0x0D;	//Carriage Return
const char LF	  = 0x0A;	//Line Feed
const char CTRL_Z = 0x1A;	//Control-Z

#define MAX_SOCKET_COUNT 6

class MTSCellularRadio
{
public:
	MTSCellularRadio(PinName tx, PinName rx, int baud);

    // Enumeration for different cellular radio types.
    enum Radio {
        NA, MTQ_H5, MTQ_EV3, MTQ_C2, MTQ_LAT3, MTQ_LVW3, MTQ_MAT1, MTQ_MVW1
    };

    // An enumeration of radio registration states with a cell tower.
    enum Registration {
        NOT_REGISTERED, REGISTERED, SEARCHING, DENIED, UNKNOWN, ROAMING
    };

	// An enumeration for radio responses.
	enum Code {
		MTS_SUCCESS = 0, MTS_ERROR = -1, MTS_FAILURE = -2, MTS_NO_RESPONSE = -3, MTS_NO_CONNECTION = -4, 
		MTS_NO_SOCKET = -5, MTS_SOCKET_CLOSED = -6, MTS_NOT_REGISTERED = -7, MTS_NO_SIGNAL = -8, 
		MTS_NO_APN = -9, MTS_NOT_ALLOWED = -10, MTS_NOT_SUPPORTED = -11, MTS_NO_SIM = -12
	};

	// This structure contains the radio status information.
    struct statusInfo {
    	std::string model;
        bool sim;
        std::string apn;
        uint8_t rssi;
        uint8_t registration;
        bool connection;
        std::string ip_address;
        std::string sockets;
        int gps;
    };	

    // This structure contains the data from a received SMS message.
    struct Sms {
        // Message Phone Number
        std::string phone_number;
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

    /** A method to enable power to the radio. Turns on the radio power regulator.
    * If the regulator is already enabled, it just check for 1.8v and OK response from AT.
    *
    * @param timeout amount of time to wait for 1.8v and OK response for AT.
    * @returns true if 1.8v is high and AT responds with OK, else false.
    */
    bool power_on(int timeout = 15);

    /** A method to safely power off the radio. 
    * 1. Close sockets and disconnect from cellular network.
    * 2. Issue shutdown command. Note: Shutdown is recommended before removing power.
    * 3. If the shutdown command is not successful and 1.8v is still high...
    *     A. Try an I/O shutdown (radio ONOFF I/O) and wait for 1.8v to go low.
    *     B. If that fails, try radio I/O reset (radio RESET I/O) and wait for 1.8v to go low. 
    *     C. If 1.8v low or timed out, remove radio power.
    * 4. If shutdown command worked and 1.8v goes low, remove radio power.
    * 5. For CE910 a 30s delay is required even though 1.8v goes low.
    *
    * @returns 0 on success else a negative value.
    */	
    int power_off();

    /** A method to check if the radio is powered.
    *
    * @returns true if powered on false if off.
    */
    bool is_powered();
	
	/** Sets up the physical connection pins
	*   (CTS, RTS, DCD, DTR, DSR, RI and RESET)
	*/
//    bool configureSignals(unsigned int CTS = NC, unsigned int RTS = NC, unsigned int DCD = NC, unsigned int DTR = NC,
//    	unsigned int RESET = NC, unsigned int DSR = NC, unsigned int RI = NC);

    /** A static method for getting a string representation for the Registration
    * enumeration.
    *
    * @param code a Registration enumeration.
    * @returns the enumeration name as a string.
    */
    std::string get_registration_names(uint8_t registration);

    /** A method for getting the signal strength of the radio. This method allows you to
	* get a value that maps to signal strength in dBm. Here 0-1 is Poor, 2-9 is Marginal,
	* 10-14 is Ok, 15-19 is Good, and 20+ is Excellent.  If you get a result of 99 the
	* signal strength is not known or not detectable.
	*
	* @returns an integer representing the signal strength or -1 if command fails.
	*/
    virtual int get_signal_strength();

    /** This method is used to check the registration state of the radio with the cell tower.
	* If not appropriatley registered with the tower you cannot make a cellular connection.
	*
	* @returns the registration state as an enumeration type or -1 if the command fails.
	*/
    virtual int get_registration();

    /** This method is used to configure the radio APN within a PDP context. Some radio models
    * require the APN to be set correctly others come with predefined APNs that should not be changed.
    * The APN for your SIM can be obtained by contacting your cellular service provider.
    *
    * @param the APN as a string.
    * @returns 0 on success, a negative value upon failure.
    */
    virtual int set_apn(const std::string& apn);

    /** Set the pin code for SIM card
    *
    *  @param sim_pin      PIN for the SIM card
    */
    virtual void set_sim_pin(const char *sim_pin);
	

    /** This method is used to configure a radio PDP context. Some radio models require the
    * APN to be set correctly others come with predefined APNs that should not be changed.
    *
    * @param cgdcont_args string for AT+CGDCONT=<string> command (cid, pdp type, apn ...)
    *    expected format example = 1,"IP",apn
    * @returns 0 on success, a negative value upon failure.
    */
    virtual int set_pdp_context(const std::string& cgdcont_args);
	
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
	virtual int send_basic_command(const std::string& command, unsigned int timeoutMillis = 1000);
	
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
    virtual std::string send_command(const std::string& command, unsigned int timeoutMillis = 1000, char esc = CR);


	/** Cellular connect / context activation.
	*
	* @param cid context ID to connect on. Use 'default' if left at 0.
	* @returns 0 on connection success, else a negative value.
	*/
	virtual int connect(const char cid = 0);
    
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
    virtual bool is_connected();

    /** Checks if the radio's SIM is inserted
    *
    * @returns true if inserted or radio does not require a SIM card, false if not inserted.
    */
	virtual bool is_sim_inserted();

    /** Checks if the radio's APN is set
    *
    * @returns true if set, false if empty.
    */
	virtual bool is_apn_set();

    /**
    * Get the radio's IP address
    *
    * @return a string containing the IP address or an empty string if no IP address is assigned
    */
    virtual std::string get_ip_address(void);

    /** Get the local network mask
     *
     *  @return         Null-terminated representation of the local network mask
     *                  or null if no network mask has been received
     */
    virtual std::string get_netmask();

    /** Get the local gateways
    *
    *  @return         Null-terminated representation of the local gateway
    *                  or null if no network mask has been received
    */
    virtual std::string get_gateway();

    /**
    * Resolve the domain name given to an ip address.
    *
    * @param name is the domain name to be resolved.
    * @return a string containing the IP address or an empty string if no IP address is assigned
    */
    std::string gethostbyname(const char *name);

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


    /** Open a socketed connection
    *
    * @param type the type of socket to open "UDP" or "TCP"
    * @param id id to give the new socket, valid 0-4
    * @param port port to open connection with
    * @param addr the IP address of the destination
    * @return 0 if socket opened successfully otherwise a negative value.
    */
    int open(const char *type, int id, const char* addr, int port);


    /** Disable the socket inactivity timer and set flush time to 10ms.
    *
    * @param socket id
    */
	void configure_socket(int id);

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
    * @return amount of data loaded into receive buffer on success or negative value on failure, 0 if nothing received.
    */
	int receive(int id, void *data, uint32_t amount);

    /** Close a socket.
    *
    *@param id is the socket to close.
    * @return MTS_SUCCESS if the socket closed successfully otherwise return MTS_FAILURE.
    */
    int close(int id);

	/** Check for an open socket.
	*
	*@param id is the socket to check.
	*@return true if the socket is not closed otherwise return false.
	*/
	bool is_socket_open(int id);

    /** Gets the radio type.
    *
    *@returns the radio type.
    */
	int get_radio_type();

    /** Gathers much data about the radio and it's status.
    *
    * @returns a string containing all the radio information gathered.
    */
	statusInfo get_radio_status();
	
//	virtual bool ping(const std::string& address = "8.8.8.8");	

    /** This method is used to send an SMS message.
    *
    * @param phone_number the phone number to send the message to as a string.
    * @param message the text message to be sent.
    * @param size of the message to be sent.
    * @returns 0 on success otherwise a negative value.
    */
    virtual int send_sms(const std::string&  phone_number, const std::string& message);

    /** This method retrieves all of the SMS messages currently available for
    * this phone number.
    *
    * @returns a vector of existing SMS messages each as an Sms struct.
    */
    virtual std::vector<Sms> get_received_sms();

    /** This method can be used to remove/delete all received SMS messages
    * even if they have never been retrieved or read.
    *
    * @returns 0 on success otherwise a negative value.
    */
    virtual int delete_all_received_sms();

    /** This method can be used to remove/delete all received SMS messages
    * that have been retrieved by the user through the get_received_sms method.
    * Messages that have not been retrieved yet will be unaffected.
    *
    * @returns 0 on success otherwise a negative value.
    */
    virtual int delete_only_read_sms();	

    /** Enables GPS.
    *
    *  @returns 0 on success, negative error code on failure
	*/
    virtual int gps_enable();

    /** Disables GPS.
    *
    *  @returns 0 on success, negative error code on failure
    */
    virtual int gps_disable();

    /** Checks if GPS is enabled.
    *
    * @returns true if GPS is enabled, false if GPS is disabled.
    */
    virtual bool is_gps_enabled();
        
    /** Get GPS position.
    *
    * @returns a structure containing the GPS data field information.
    */
    virtual gpsData gps_get_position();

    /** Check for GPS fix.
    *
    * @returns true if there is a fix and false otherwise.
    */
    virtual bool gps_has_fix();	

protected:
    FileHandle* _fh;
    ATCmdParser* _parser;
    std::string _ip_address;
    Callback<void()> _socket_event;

    Radio _type;				//The type of radio being used
    std::string _mts_model;
    std::string _manufacturer_model;
    std::string _cid;			//context ID=1 for most radios. Verizon LTE LVW2&3 use cid 3.
    std::string _registration_cmd;
	
    DigitalIn* _vdd1_8;		//Maps to 1.8v from the radio.
    DigitalOut* _radio_pwr;	//Maps to the 3.8v power regulator that powers the radio and tiny9.
    DigitalOut* _3g_onoff;	//Maps to the tiny9 which controls the radio ON_OFF and HW_SHUTDOWN/RESET.
    DigitalIn* _radio_cts;	//Maps to the radio's cts signal
    DigitalOut* _radio_rts;	//Maps to the radio's rts signal
    DigitalIn* _radio_dcd;	//Maps to the radio's dcd signal
    DigitalIn* _radio_dsr;	//Maps to the radio's dsr signal
    DigitalIn* _radio_ri;	//Maps to the radio's ring indicator signal
    DigitalOut* _radio_dtr;	//Maps to the radio's dtr signal

    DigitalOut* _reset_line;	//Maps to the radio's reset signal 

private:
// Event thread for asynchronous received data(SRING) and socket/connection disconnect (NO CARRIER).
//    3GPP defines SRING and NO CARRIER URCs.	
	void handle_urc_event();	
    void SRING_URC();
	Thread event_thread;
	Mutex _mutex;

};

#endif // MTSCELLULARRADIO_H

