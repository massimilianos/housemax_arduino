#ifndef domusalberti_h
#define domusalberti_h

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

class domusalberti
{
	public:
		int RELAY_ON;
		int RELAY_OFF;

		int RELAY_1_PIN;
		int RELAY_2_PIN;
		int RELAY_3_PIN;
		int RELAY_4_PIN;

		int DHT11_PIN;
	
		void SetupSamplePorts();
		void WaitForRequest(EthernetClient client, int bufferSize, char* buffer);
		void ParseReceivedRequest(char* cmd, char* buffer, char* param1, char* param2);
		void PerformRequestedCommands(char* cmd, int controlloManuale);

	private:
		void RemoteControlloManuale();
		void RemoteDigitalWrite();
		void RemoteAnalogRead();
		void RemoteTempRead();
		void scriviDati(EthernetClient client, float temperatura, float umidita);
		void PrintString(char* label, char* str);
		void PrintNumber(char* label, int number);
};
#endif

