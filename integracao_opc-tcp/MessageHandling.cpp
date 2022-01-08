#include "MessageHandling.h"

#include <string>
using namespace std;
#pragma once
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
MessageHandling::MessageHandling(string RawMessage) : rawMessage(RawMessage) {
	ConvertMessageHeader();
}