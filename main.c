#include <stdio.h>

#include "gpu-collector.h"

void found_gpu(unsigned int vendor,
	unsigned int device, unsigned int memory_bytes)
{
	printf("Detected VGA device: vendor = %04x, device = %04x, memory = %d MiB\n",
		vendor, device, memory_bytes / (1024 * 1024));
}

int main(int argc, char *argv[])
{
	return gpuc_enumerate_gpus(found_gpu);
}
