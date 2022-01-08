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
		
		TEST_METHOD(SoftwareDataMessageConversion)
		{
			string message = "104556$100$1435.0$1480.0$0002.0$0010.0";
			MessageHandling massageHandler(message);
			Assert::AreEqual(104556, massageHandler.getSequenceNumber());
			Assert::AreEqual(100, massageHandler.getMessageCode());
			Assert::AreEqual(1435.0, massageHandler.getLadleTemperature());
			Assert::AreEqual(1480.0, massageHandler.getVaccumChamberTemperature());
			Assert::AreEqual(0002.0, massageHandler.getGasInjectionPressure());
			Assert::AreEqual(0010.0, massageHandler.getVaccumChamberPressure());
		}
	};
}
