// Minimal test for core module to satisfy Makefile during development
#include <stdio.h>
#include "../core/core.h"

int main(void)
{
	printf("test_core: basic smoke test\n");
	connection_manager_print_all();
	return 0;
}
