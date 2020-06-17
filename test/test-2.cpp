#include "pch.h"

namespace test7828
{
	TEST_CLASS(testHello)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			Assert::AreEqual("hello", "hello");
		}
	};
}
