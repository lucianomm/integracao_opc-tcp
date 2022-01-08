#include "MessageHandling.h"

#include <string>
using namespace std;
#pragma once
MessageHandling::Message MessageHandling::ConvertMessage(string RawMessage) {
		MessageHandling::Message convertedMessage;
		string substring = RawMessage.substr(0, RawMessage.find('$'));
		convertedMessage.SequenceNumber = stoi(substring);
		RawMessage.erase(0, RawMessage.find('$')+1);

		convertedMessage.MessageCode = stoi(RawMessage.substr(0, RawMessage.find('$')));
		RawMessage.erase(0, RawMessage.find('$') + 1);

		convertedMessage.LadleTemperature = stod(RawMessage.substr(0, RawMessage.find('$')));
		RawMessage.erase(0, RawMessage.find('$') + 1);

		convertedMessage.VaccumChamberTemperature = stod(RawMessage.substr(0, RawMessage.find('$')));
		RawMessage.erase(0, RawMessage.find('$') + 1);

		convertedMessage.GasInjectionPressure = stod(RawMessage.substr(0, RawMessage.find('$')));
		RawMessage.erase(0, RawMessage.find('$') + 1);

		convertedMessage.VaccumChamperPressure = stod(RawMessage.substr(0, RawMessage.find('$')));
		RawMessage.erase(0, RawMessage.find('$') + 1);

		return convertedMessage;
	}
int MessageHandling::getSequenceNumber() {
	return message.SequenceNumber;
}
int MessageHandling::getMessageCode() {
	return message.MessageCode;
}
double MessageHandling::getLadleTemperature() {
	return message.LadleTemperature;
}
double MessageHandling::getVaccumChamberTemperature() {
	return message.VaccumChamberTemperature;
}
double MessageHandling::getGasInjectionPressure() {
	return message.GasInjectionPressure;
}
double MessageHandling::getVaccumChamberPressure() {
	return message.VaccumChamperPressure;
}
MessageHandling::MessageHandling(string RawMessage) : message(ConvertMessage(RawMessage)) {
}