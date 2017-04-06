/* MTSCellularRadio object definition
 *
*/

#ifndef MTSCELLULARRADIO_H
#define MTSCELLULARRADIO_H

#include "ATParser.h"

class MTSCellularRadio
{
public:
	MTSCellularRadio(PinName RADIO_TX, PinName RADIO_RX, PinName RADIO_RTS = NC, PinName RADIO_CTS = NC, PinName RADIO_DCD = NC,
		PinName RADIO_DSR = NC, PinName RADIO_DTR = NC, PinName RADIO_RI = NC, PinName Radio_Power = , PinName Radio_Reset = );


	// Class ping paramter constants
    static const unsigned int PINGDELAY = 3;	//Time to wait on each ping for a response before timimg out (seconds)
    static const unsigned int PINGNUM = 4;		//Number of pings to try on ping command

	// Enumeration for radio power.
	enum Power{
		OFF, ON, SLEEP, RESET
		};

    /// Enumeration for different cellular radio types.
    enum Radio {
        NA, MTSMC_H5, MTSMC_EV3, MTSMC_G3, MTSMC_C2, MTSMC_H5_IP, MTSMC_EV3_IP, MTSMC_C2_IP, MTSMC_LAT1, MTSMC_LVW2, MTSMC_LEU1
    };

    /// An enumeration of radio registration states with a cell tower.
    enum Registration {
        NOT_REGISTERED, REGISTERED, SEARCHING, DENIED, UNKNOWN, ROAMING
    };

	/** This structure contains the data for an SMS message.
	*/
    struct Sms {
        /// Message Phone Number
        std::string phoneNumber;
        /// Message Body
        std::string message;
        /// Message Timestamp
        std::string timestamp;
    };

	/** This structure contains the data for GPS position.
	*/
    struct gpsData {
        bool success;
        /// Format is ddmm.mmmm N/S. Where: dd - degrees 00..90; mm.mmmm - minutes 00.0000..59.9999; N/S: North/South.
        std::string latitude;
        /// Format is dddmm.mmmm E/W. Where: ddd - degrees 000..180; mm.mmmm - minutes 00.0000..59.9999; E/W: East/West.
        std::string longitude;
        /// Horizontal Diluition of Precision.
        float hdop;
        /// Altitude - mean-sea-level (geoid) in meters.
        float altitude;
        /// 0 or 1 - Invalid Fix; 2 - 2D fix; 3 - 3D fix.
        int fix;
        /// Format is ddd.mm - Course over Ground. Where: ddd - degrees 000..360; mm - minutes 00..59.
        std::string cog;
        /// Speed over ground (Km/hr).
        float kmhr;
        /// Speed over ground (knots).
        float knots;
        /// Total number of satellites in use.
        int satellites;
        /// Date and time in the format YY/MM/DD,HH:MM:SS.
        std::string timestamp;
    };
    
    /** Controls radio power.
    	* Before power is removed, the connection must be brought down.
    	*
    	* @returns true if successful, false if a failure was detected.
    	*/
    bool power(Power option){
   	};
    	
	/** Sets up the physical connection pins
	*   (CTS, RTS, DCD, DTR, DSR, RI and RESET)
	*/
    bool configureSignals(unsigned int CTS = NC, unsigned int RTS = NC, unsigned int DCD = NC, unsigned int DTR = NC,
    	unsigned int RESET = NC, unsigned int DSR = NC, unsigned int RI = NC);

    /** A method for testing command access to the radio.  This method sends the
	* command "AT" to the radio, which is a standard radio test to see if you
	* have command access to the radio.  The function returns when it receives
	* the expected response from the radio.
	*
	* @returns the standard AT Code enumeration.
	*/
    virtual Code test();

    /** A method for getting the signal strength of the radio. This method allows you to
	* get a value that maps to signal strength in dBm. Here 0-1 is Poor, 2-9 is Marginal,
	* 10-14 is Ok, 15-19 is Good, and 20+ is Excellent.  If you get a result of 99 the
	* signal strength is not known or not detectable.
	*
	* @returns an integer representing the signal strength.
	*/
    virtual int getSignalStrength();

    /** This method is used to check the registration state of the radio with the cell tower.
	* If not appropriatley registered with the tower you cannot make a cellular connection.
	*
	* @returns the registration state as an enumeration type.
	*/
    virtual Registration getRegistration();

    /** This method is used to set the radios APN if using a SIM card. Note that the APN
	* must be set correctly before you can make a data connection. The APN for your SIM
	* can be obtained by contacting your cellular service provider.
	*
	* @param the APN as a string.
	* @returns the standard AT Code enumeration.
	*/
    virtual Code setApn(const std::string& apn) = 0;

    /** This method is used to set the DNS which enables the use of URLs instead
	* of IP addresses when making a socket connection.
	*
	* @param the DNS server address as a string in form xxx.xxx.xxx.xxx.
	* @returns the standard AT Code enumeration.
	*/
    virtual Code setDns(const std::string& primary, const std::string& secondary = "0.0.0.0");

    //Cellular Radio Specific
    /** A method for sending a generic AT command to the radio. Note that you cannot
	* send commands and have a data connection at the same time.
	*
	* @param command the command to send to the radio without the escape character.
	* @param timeoutMillis the time in millis to wait for a response before returning.
	* @param esc escape character to add at the end of the command, defaults to
	* carriage return (CR).  Does not append any character if esc == 0.
	* @returns all data received from the radio after the command as a string.
	*/
    virtual std::string sendCommand(const std::string& command, unsigned int timeoutMillis, char esc = CR);

    /** A method for sending a basic AT command to the radio. A basic AT command is
	* one that simply has a response of either OK or ERROR without any other information.
	* Note that you cannot send commands and have a data connection at the same time.
	*
	* @param command the command to send to the radio without the escape character.
	* @param timeoutMillis the time in millis to wait for a response before returning.
	* @param esc escape character to add at the end of the command, defaults to
	* carriage return (CR).
	* @returns the standard Code enumeration.
	*/
    virtual Code sendBasicCommand(const std::string& command, unsigned int timeoutMillis, char esc = CR);

    /** A static method for getting a string representation for the Registration
	* enumeration.
	*
	* @param code a Registration enumeration.
	* @returns the enumeration name as a string.
	*/
    static std::string getRegistrationNames(Registration registration);

    /** A static method for getting a string representation for the Radio
	* enumeration.
	*
	* @param type a Radio enumeration.
	* @returns the enumeration name as a string.
	*/
    static std::string getRadioNames(Radio radio);
    
    /** A method for changing the echo commands from radio.
	* @param state Echo mode is off (an argument of 1 turns echos off, anything else turns echo on)
	* @returns standard Code enumeration
	*/
    virtual Code echo(bool state);

	/** Gets the device IP
	* @returns string containing the IP address
	*/
	virtual std::string getDeviceIP();

	/** Get the device IMEI or MEID (whichever is available)
	* @returns string containing the IMEI for GSM, the MEID for CDMA, or an empty string
	* if it failed to parse the number.
	*/
	std::string getEquipmentIdentifier();

	/** Get string representation of radio type
	* @returns string containing the radio type (MTSMC-H5, etc)
	*/
	std::string getRadioType();

	/** PPP connect command.
	* Connects the radio to the cellular network.
	*
	* @returns true if PPP connection to the network succeeded,
	* false if the PPP connection failed.
	*/
	virtual bool connect();
    
	/** PPP disconnect command.
	* Disconnects from the PPP network, and will also close active socket
	* connection if open. 
	*/
	virtual void disconnect();	    		

	/** Checks if the radio is connected to the cell network.
	* Checks antenna signal, cell tower registration, and context activation
	* before finally pinging (4 pings, 32 bytes each) to confirm PPP connection
	* to network. Will return true if there is an open socket connection as well.
	*
	* @returns true if there is a PPP connection to the cell network, false
	* if there is no PPP connection to the cell network.
	*/
	virtual bool isConnected();

	/** Pings specified DNS or IP address
	 * Google DNS server used as default ping address
	 * @returns true if ping received alive response else false
	 */
	virtual bool ping(const std::string& address = "8.8.8.8");	

	/** This method is used to send an SMS message. Note that you cannot send an
	* SMS message and have a data connection open at the same time.
	*
	* @param phoneNumber the phone number to send the message to as a string.
	* @param message the text message to be sent.
	* @returns the standard AT Code enumeration.
	*/
	virtual Code sendSMS(const std::string& phoneNumber, const std::string& message);

	/** This method is used to send an SMS message. Note that you cannot send an
	* SMS message and have a data connection open at the same time.
	*
	* @param sms an Sms struct that contains all SMS transaction information.
	* @returns the standard AT Code enumeration.
	*/
	virtual Code sendSMS(const Sms& sms);

	/** This method retrieves all of the SMS messages currently available for
	* this phone number.
	*
	* @returns a vector of existing SMS messages each as an Sms struct.
	*/
	virtual std::vector<Cellular::Sms> getReceivedSms();

	/** This method can be used to remove/delete all received SMS messages
	* even if they have never been retrieved or read.
	*
	* @returns the standard AT Code enumeration.
	*/
	virtual Code deleteAllReceivedSms();

	/** This method can be used to remove/delete all received SMS messages
	* that have been retrieved by the user through the getReceivedSms method.
	* Messages that have not been retrieved yet will be unaffected.
	*
	* @returns the standard AT Code enumeration.
	*/
	virtual Code deleteOnlyReceivedReadSms();	

	/** Enables GPS.
	* @returns true if GPS is enabled, false if GPS is not supported.
	*/
	virtual bool GPSenable();

	/** Disables GPS.
	* @returns true if GPS is disabled, false if GPS does not disable.
	*/
	virtual bool GPSdisable();

	/** Checks if GPS is enabled.
	* @returns true if GPS is enabled, false if GPS is disabled.
	*/
	virtual bool GPSenabled();
		
	/** Get GPS position.
	* @returns a structure containing the GPS data field information.
	*/
	virtual gpsData GPSgetPosition();

	/** Check for GPS fix.
	* @returns true if there is a fix and false otherwise.
	*/
	virtual bool GPSgotFix();	


private:
	BufferedSerial _serial;
	ATParser _parser;

	std::string apn;		//APN setting for the radio. Access Point Name
    Radio type;				//The type of radio being used	

	DigitalIn* radio_cts;	//Maps to the radio's cts signal
	DigitalOut* radio_rts;	//Maps to the radio's rts signal
    DigitalIn* radio_dcd;	//Maps to the radio's dcd signal
    DigitalIn* radio_dsr;	//Maps to the radio's dsr signal
    DigitalIn* radio_ri;	//Maps to the radio's ring indicator signal
    DigitalOut* radio_dtr;	//Maps to the radio's dtr signal

    DigitalOut* resetLine;	//Maps to the radio's reset signal 
}

#endif // MTSCELLULARRADIO_H

