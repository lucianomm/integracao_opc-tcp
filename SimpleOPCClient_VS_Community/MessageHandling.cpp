#include "MessageHandling.h"
#include <string>
#include <sstream>
#include <iomanip>
using namespace std;
#pragma once
#define SEQUENCE_FIELD_SIZE 6
#define REAL_FIELD_SIZE 6
#define PROCESS_DATA_MESSAGE_CODE 100
#define SETPOINTS_MESSAGE_CODE 103
void MessageHandling::ConvertMessageHeader() {

	messageHeader.SequenceNumber = stoi(rawMessage.substr(0, rawMessage.find('$')));
	rawMessage.erase(0, rawMessage.find('$') + 1);

	messageHeader.MessageCode = stoi(rawMessage.substr(0, rawMessage.find('$')));
	rawMessage.erase(0, rawMessage.find('$') + 1);

	return;
}
void MessageHandling::ConvertSetPointsMessage() {
	setPointsMessage.GasInjectionPressureSP = stod(rawMessage.substr(0, rawMessage.find('$')));
	rawMessage.erase(0, rawMessage.find('$') + 1);

	setPointsMessage.VaccumChamberTemperatureSP = stod(rawMessage.substr(0, rawMessage.find('$')));
	rawMessage.erase(0, rawMessage.find('$') + 1);

	setPointsMessage.VaccumChamberPressureSP = stoi(rawMessage.substr(0, rawMessage.find('$')));
	rawMessage.erase(0, rawMessage.find('$') + 1);

	return;
}
void MessageHandling::ConvertProcessDataMessage() {
	processDataMessage.LadleTemperature = stod(rawMessage.substr(0, rawMessage.find('$')));
	rawMessage.erase(0, rawMessage.find('$') + 1);

	processDataMessage.VaccumChamberTemperature = stod(rawMessage.substr(0, rawMessage.find('$')));
	rawMessage.erase(0, rawMessage.find('$') + 1);

	processDataMessage.GasInjectionPressure = stod(rawMessage.substr(0, rawMessage.find('$')));
	rawMessage.erase(0, rawMessage.find('$') + 1);

	processDataMessage.VaccumChamberPressure = stod(rawMessage.substr(0, rawMessage.find('$')));
	rawMessage.erase(0, rawMessage.find('$') + 1);

	return;
}

//Constructor for Raw message string
MessageHandling::MessageHandling(string RawMessage) : rawMessage(RawMessage) {
	ConvertMessageHeader();
	if (messageHeader.MessageCode == PROCESS_DATA_MESSAGE_CODE) {
		isProcessDataMessage = true;
		ConvertProcessDataMessage();
	}
	else if(messageHeader.MessageCode == SETPOINTS_MESSAGE_CODE)
	{
		isSetPointsMessage = true;
		ConvertSetPointsMessage();
	}
}

void MessageHandling::UpdateMessageFromString(string RawMessage){
	rawMessage = RawMessage;
	ConvertMessageHeader();
	if (messageHeader.MessageCode == PROCESS_DATA_MESSAGE_CODE) {
		isProcessDataMessage = true;
		ConvertProcessDataMessage();
	}
	else if (messageHeader.MessageCode == SETPOINTS_MESSAGE_CODE)
	{
		isSetPointsMessage = true;
		ConvertSetPointsMessage();
	}
}

void MessageHandling::UpdateProcessData(int sequenceNumber, int messageCode, double ladleTemperature,
	double vaccumChamberTemperature, double gasInjectionPressure, double vaccumChamberPressure) 
{
	messageHeader.SequenceNumber = sequenceNumber;

	messageHeader.MessageCode = messageCode;

	processDataMessage.LadleTemperature = ladleTemperature;

	processDataMessage.VaccumChamberTemperature = vaccumChamberTemperature;

	processDataMessage.GasInjectionPressure = gasInjectionPressure;

	processDataMessage.VaccumChamberPressure = vaccumChamberPressure;
}

string MessageHandling::MessageHeaderToString() {
	string sequenceNumber = to_string(messageHeader.SequenceNumber);
	return string(SEQUENCE_FIELD_SIZE - sequenceNumber.length(), '0').append(sequenceNumber) + '$' +
		to_string(messageHeader.MessageCode);

}

string MessageHandling::RealToString(double real) {
	stringstream stream;
	stream << fixed << setprecision(1) << real;
	string result = stream.str();
	return string(REAL_FIELD_SIZE - result.length(), '0').append(result);
}

string MessageHandling::ProcessDataMessageToString() {
	string sequenceNumber = to_string(messageHeader.SequenceNumber);
	string message =
		MessageHeaderToString() + '$' +
		RealToString(processDataMessage.LadleTemperature) + '$' +
		RealToString(processDataMessage.VaccumChamberTemperature) + '$' +
		RealToString(processDataMessage.GasInjectionPressure) + '$' +
		RealToString(processDataMessage.VaccumChamberPressure);
	return message;
}

string MessageHandling::SetPointsMessagetoString() {
	string sequenceNumber = to_string(messageHeader.SequenceNumber);
	string message =
		MessageHeaderToString() + '$' +
		RealToString(setPointsMessage.GasInjectionPressureSP) + '$' +
		RealToString(setPointsMessage.VaccumChamberTemperatureSP) + '$' +
		to_string(setPointsMessage.VaccumChamberPressureSP);
	return message;
}

string MessageHandling::ACKMessageToString() {
	string message =
		to_string(messageHeader.SequenceNumber) + '$' +
		to_string(messageHeader.MessageCode);
	return message;
}

string MessageHandling::toString() {
	if (isProcessDataMessage) return ProcessDataMessageToString();
	else if (isSetPointsMessage) return SetPointsMessagetoString();
	else return ACKMessageToString();
}