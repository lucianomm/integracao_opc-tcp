#include "pch.h"
#include "CppUnitTest.h"
#include "../integracao_opc-tcp/MessageHandling.h"
#include "../integracao_opc-tcp/MessageHandling.cpp"
#include <string>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace integracaoopctcptests
{
	TEST_CLASS(integracaoopctcptests)
	{
	public:
		TEST_METHOD(MessageHeaderConversion)
		{
			int sequenceNumber = 000001;
			int messageCode = 101;
			string message =
				to_string(sequenceNumber) + '$' +
				to_string(messageCode);

			MessageHandling messageHandler(message);
			Assert::AreEqual(sequenceNumber, messageHandler.getSequenceNumber());
			Assert::AreEqual(messageCode, messageHandler.getMessageCode());
		}
		TEST_METHOD(SoftwareDataMessageConversion)
		{
			int sequenceNumber = 104556;
			int messageCode = 100;
			double ladleTemperature = 1435.0;
			double vaccumChamberTemperature = 1480.0;
			double gasInjectionPressure = 0002.0;
			double vaccumChamberPressure = 0010.0;
			string processDataMessageExample = 
				to_string(sequenceNumber) + '$' + 
				to_string(messageCode) + '$' +
				to_string(ladleTemperature) + '$' +
				to_string(vaccumChamberTemperature) + '$' +
				to_string(gasInjectionPressure) + '$' +
				to_string(vaccumChamberPressure);

			MessageHandling messageHandler(processDataMessageExample);
			Assert::AreEqual(sequenceNumber, messageHandler.getSequenceNumber());
			Assert::AreEqual(messageCode, messageHandler.getMessageCode());
			Assert::AreEqual(ladleTemperature, messageHandler.getLadleTemperature());
			Assert::AreEqual(vaccumChamberTemperature, messageHandler.getVaccumChamberTemperature());
			Assert::AreEqual(gasInjectionPressure, messageHandler.getGasInjectionPressure());
			Assert::AreEqual(vaccumChamberPressure, messageHandler.getVaccumChamberPressure());
		}
		TEST_METHOD(SetPointMessageConversion)
		{
			int sequenceNumber = 000047;
			int messageCode = 103;
			double gasInjectionPressureSP = 0010.0;
			double vaccumChamberTemperatureSP = 1450.0;
			int vaccumChamberPressureSP = 0002;
			string setpointsMessageExample =
				to_string(sequenceNumber) + '$' +
				to_string(messageCode) + '$' +
				to_string(gasInjectionPressureSP) + '$' + 
				to_string(vaccumChamberTemperatureSP) + '$' + 
				to_string(vaccumChamberPressureSP);

			MessageHandling messageHandler(setpointsMessageExample);
			Assert::AreEqual(sequenceNumber, messageHandler.getSequenceNumber());
			Assert::AreEqual(messageCode, messageHandler.getMessageCode());
			Assert::AreEqual(gasInjectionPressureSP, messageHandler.getGasInjectionPressureSP());
			Assert::AreEqual(vaccumChamberTemperatureSP, messageHandler.getVaccumChamberTemperatureSP());
			Assert::AreEqual(vaccumChamberPressureSP, messageHandler.getVaccumChamberPressureSP());
	};
}
