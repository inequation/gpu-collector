#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#if 0
#define debugf printf
#else
#define debugf (void)
#endif

struct gpu_info
{
	bool found;
	unsigned short device_id, vendor_id;
	size_t memory_bytes;
};

struct gpu_info gpu_info[2] = {{0}, {0}};

void found_gpu(unsigned int vendor,
	unsigned int device, unsigned int memory_bytes)
{
	printf("Detected VGA device: vendor = %04x, device = %04x, memory = %d kB\n",
		vendor, device, memory_bytes / 1024);
}

int enumerate_gpus(void (* found_callback)(unsigned int vendor,
	unsigned int device, unsigned int memory_bytes))
{
	int i;
	struct dirent *device_dir;
	char path_buf[PATH_MAX];
	DIR *pci_devices;

	pci_devices = opendir("/sys/bus/pci/devices");
	if (!pci_devices)
	{
		debugf("Failed to access sysfs\n");
		return 1;
	}
	while ((device_dir = readdir(pci_devices)) != NULL)
	{
		DIR *subdir;
		struct dirent *entry;
		unsigned int d_class = 0, d_vendor = 0, d_device = 0, d_memory = 0;

		// skip ".", ".." and dotfiles
		if (device_dir->d_name[0] == '.')
		{
			debugf("Dotfile, skipping\n");
			continue;
		}

		snprintf(path_buf, sizeof(path_buf), "/sys/bus/pci/devices/%s",
			device_dir->d_name);

		debugf("Entering %s\n", path_buf);
		subdir = opendir(path_buf);
		if (!subdir)
		{
			debugf("Can't access %s, skipping\n", path_buf);
			continue;
		}

		while ((entry = readdir(subdir)) != NULL)
		{

			snprintf(path_buf, sizeof(path_buf), "/sys/bus/pci/devices/%s/%s",
				device_dir->d_name, entry->d_name);
			if (!strcmp(entry->d_name, "class"))
			{
				FILE *f = fopen(path_buf, "r");
				if (!f)
					continue;
				fscanf(f, "0x%X", &d_class);
				if (d_class != 0x030000)
					d_class = 0;
				fclose(f);
				continue;
			}
			else if (!strcmp(entry->d_name, "vendor"))
			{
				FILE *f = fopen(path_buf, "r");
				if (!f)
					continue;
				fscanf(f, "0x%X", &d_vendor);
				fclose(f);
				continue;
			}
			else if (!strcmp(entry->d_name, "device"))
			{
				FILE *f = fopen(path_buf, "r");
				if (!f)
					continue;
				fscanf(f, "0x%X", &d_device);
				fclose(f);
				continue;
			}
		}
		if (!d_class)
			continue;
		debugf("Detected VGA device: vendor = %04x, device = %04x\n",
			d_vendor, d_device);
		found_callback(d_vendor, d_device, d_memory);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	return enumerate_gpus(found_gpu);
}
