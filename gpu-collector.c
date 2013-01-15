#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>

struct gpu_info
{
	bool found;
	unsigned short device_id, vendor_id;
	size_t memory_bytes;
};

struct gpu_info gpu_info[2] = {{0}, {0}};

int main(int argc, char *argv[])
{
	int i;
	struct dirent *pci_device_dir;
	char path_buf[PATH_MAX];
	DIR *pci_devices = opendir("/sys/bus/pci/devices");
	if (!pci_devices)
	{
		printf("Failed to access sysfs\n");
		return 1;
	}
	while ((pci_device_dir = readdir(pci_devices)) != NULL)
	{
		DIR *pci_device;
		struct dirent *entry;
		unsigned int d_class = 0, d_vendor = 0, d_device = 0, d_memory = 0;

		if (pci_device_dir->d_name[0] == '.')
			continue;

		snprintf(path_buf, sizeof(path_buf), "/sys/bus/pci_devices/%s",
			pci_device_dir->d_name);
		printf("Entering %s\n", path_buf);
		pci_device = opendir(path_buf);

		if (!pci_device)
		{
			printf("Can't access %s, skipping\n", path_buf);
			continue;
		}

		while ((entry = readdir(pci_device)) != NULL)
		{
			if (!strcmp(entry->d_name, "class"))
			{
				FILE *f = fopen(entry->d_name, "r");
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
				FILE *f = fopen(entry->d_name, "r");
				if (!f)
					continue;
				fscanf(f, "0x%X", &d_vendor);
				fclose(f);
				continue;
			}
			else if (!strcmp(entry->d_name, "device"))
			{
				FILE *f = fopen(entry->d_name, "r");
				if (!f)
					continue;
				fscanf(f, "0x%X", &d_device);
				fclose(f);
				continue;
			}
		}
		printf("Read PCI device: vendor = %04x, device = %04x\n",
			d_vendor, d_device);
	}
	/*for (i = 0; i < sizeof(gpu_info) / sizeof(gpu_info[0]); ++i)
	{
		if (gpu_info[i].found)
		{
			printf("Found %s GPU:\n"
				"Vendor: 0x%04X\n",
				"Device: 0x%04X\n"
				"Memory: %z bytes\n",
				i == 0 ? "primary" : "secondary",
				vendor_id,
				device_id,
				memory_bytes);
		}
	}
	if (!gpu_info[0].found)
		printf("Could not find primary GPU\n");*/
	return 0;
}
