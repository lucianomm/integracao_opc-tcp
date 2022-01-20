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
private:
	struct MessageDataStructure
	{
		int SequenceNumber;
		int MessageCode;
	};
	struct ProcessDataMessage
	{
		double LadleTemperature;
		double VaccumChamberTemperature;
		double GasInjectionPressure;
		double VaccumChamberPressure;
	};
	struct SetPointsMessage
	{
		double GasInjectionPressureSP;
		double VaccumChamberTemperatureSP;
		int VaccumChamberPressureSP;
	};
	bool isProcessDataMessage = false, isSetPointsMessage = false;
	ProcessDataMessage processDataMessage;
	MessageDataStructure messageHeader;
	SetPointsMessage setPointsMessage;
	std::string rawMessage;
	void ConvertMessageHeader();
	std::string ProcessDataMessageToString();
	std::string SetPointsMessagetoString();
	std::string ACKMessageToString();
	std::string MessageHeaderToString();
	std::string RealToString(double real);
	void ConvertProcessDataMessage();
	void ConvertSetPointsMessage();
public:
	int getSequenceNumber() { return messageHeader.SequenceNumber; }
	int getMessageCode() { return messageHeader.MessageCode; }
	double getLadleTemperature() { return processDataMessage.LadleTemperature; }
	double getVaccumChamberTemperature() { return processDataMessage.VaccumChamberTemperature; }
	double getGasInjectionPressure() { return processDataMessage.GasInjectionPressure; }
	double getVaccumChamberPressure() { return processDataMessage.VaccumChamberPressure; }
	double getGasInjectionPressureSP() { return setPointsMessage.GasInjectionPressureSP; }
	double getVaccumChamberTemperatureSP() { return setPointsMessage.VaccumChamberTemperatureSP; }
	int getVaccumChamberPressureSP() { return setPointsMessage.VaccumChamberPressureSP; }
	MessageHandling(std::string RawMessage);
	void UpdateMessageFromString(std::string RawMessage);
	void UpdateProcessData(int sequenceNumber, int messageCode, double ladleTemperature,
		double vaccumChamberTemperature, double gasInjectionPressure, double vaccumChamberPressure);
	void setProcessDataMessage(double setLadleTemperature,
							   double setVaccumChamberTemperature,
		                       double setGasInjectionPressure,
		                       double setVaccumChamberPressure){
		processDataMessage.LadleTemperature = setLadleTemperature;
		processDataMessage.VaccumChamberTemperature = setVaccumChamberTemperature;
		processDataMessage.GasInjectionPressure = setGasInjectionPressure;
		processDataMessage.VaccumChamberPressure = setVaccumChamberPressure;
	}
	std::string toString();
};

#endif

