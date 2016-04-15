#pragma once
#include <iostream>
#include <cstdlib>

/*!
Fonction that checks if the condition cond holds true, print out OK or KO and exit program if critical boolean is passed
*/
int test_assert(bool cond, char* msg, bool critical = false)
{
	if (cond)
	{
		std::cout << msg << " ...... OK !" << std::endl;
		return 1;
	}
	else
	{
		std::cout << msg << "...... KO !" << std::endl;
		if (critical)
		{
			std::abort();
		}
		return 0;
	}
}