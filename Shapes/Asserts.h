#pragma once
#include <iostream>
#include <cstdlib>

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