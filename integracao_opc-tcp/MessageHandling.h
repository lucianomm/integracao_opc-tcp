/*
Each message has fields separated by $

Message structure should follow this schematic:

--- Software App -> Process Computer ---

	--- Message with software data ---

		field		description						Type(Example)			Corresponding part item in OPC
		- 1		- Message sequence number			int (N N N N N N)
		- 2		- Message code						int (always '100')
		- 3		- Ladle temperature	(K)				REAL(N N N N . N)		Random.Real4
		- 4		- Vaccum chamber temperature (K)	REAL(N N N N . N)		Saw-toothed Waves.Real4
		- 5		- Gas injection pressure (mm Hg)	REAL(N N N N . N)		Triangle Waves.Real4
		- 6		- Vaccum chamber pressure (mm Hg)	REAL(N N N N . N)		Square Waves.Real4

	--- Message requesting set-points ---

		field		description						Type(Example)
		- 1		- Message sequence number			int (N N N N N N)
		- 2		- Message code						int (always '102')

	--- ACK messages ---

		field		description						Type(Example)
		- 1		- Message sequence number			int (N N N N N N)
		- 2		- Message code						int (always '104')
*/
#include <string>
#ifndef MESSAGEHANDLING_H
#define MESSAGEHANDLING_H

class MessageHandling {
public:
	struct Message
	{
		int SequenceNumber;
		int MessageCode;
		double LadleTemperature;
		double VaccumChamberTemperature;
		double GasInjectionPressure;
		double VaccumChamperPressure;
	};
	int getSequenceNumber();
	int getMessageCode();
	double getLadleTemperature();
	double getVaccumChamberTemperature();
	double getGasInjectionPressure();
	double getVaccumChamberPressure();
	MessageHandling(std::string RawMessage);
private:
	Message message;
	Message ConvertMessage(std::string RawMessage);
};

#endif

