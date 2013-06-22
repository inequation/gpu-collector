#ifndef GPU_COLLECTOR_H
#define GPU_COLLECTOR_H

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#if 0
#define gpuc_debugf printf
#else
#define gpuc_debugf (void)
#endif

static int gpuc_get_default_X_display()
{
	int display = 0;
	char *display_env = getenv("DISPLAY");
	gpuc_debugf("$DISPLAY = %s\n", display_env);
	sscanf(display_env, ":%d", &display);
	return display;
}

static void gpuc_parse_memory_from_log(FILE *f, const char *format, unsigned int *memory, unsigned int read_multiplier)
{
	unsigned int match;
	int scan_result;
	if (!f)
		return;
	while ((scan_result = fscanf(f, format, &match)) != EOF)
	{
		if (scan_result)
		{
			gpuc_debugf("Got match: %d\n", match);
			match *= read_multiplier;
			if (*memory < match)
				*memory = match;
		}
		else
		{
			// skip a token, try again
			fscanf(f, "%*s");
		}
	}
}

static int gpuc_enumerate_gpus(void (* found_callback)(unsigned int vendor,
	unsigned int device, unsigned int memory_bytes))
{
	int i;
	struct dirent *device_dir;
	char path_buf[PATH_MAX];
	DIR *pci_devices;

	pci_devices = opendir("/sys/bus/pci/devices");
	if (!pci_devices)
	{
		gpuc_debugf("Failed to access sysfs\n");
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
			gpuc_debugf("Dotfile, skipping\n");
			continue;
		}

		snprintf(path_buf, sizeof(path_buf), "/sys/bus/pci/devices/%s",
			device_dir->d_name);

		gpuc_debugf("Entering %s\n", path_buf);
		subdir = opendir(path_buf);
		if (!subdir)
		{
			gpuc_debugf("Can't access %s, skipping\n", path_buf);
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
			else if (!strcmp(entry->d_name, "resource"))
			{
				void *begin = NULL, *end = NULL;
				unsigned int region_length;
				FILE *f = fopen(path_buf, "r");
				if (!f)
					continue;
				// this actually only works for GPUs that steal system RAM,
				// such as the Intel GMA; we can safely assume that the
				// longest memory region mapped to the device is used as VRAM
				while (fscanf(f, "%p %p %*p", &begin, &end) == 2)
				{
					region_length = 1 + (unsigned int)(end - begin);
					if (d_memory < region_length)
						d_memory = region_length;
				}
				fclose(f);
				continue;
			}
		}
		if (!d_class)
			continue;
		gpuc_debugf("Detected VGA device: vendor = %04x, device = %04x\n",
			d_vendor, d_device);
		switch (d_vendor)
		{
			case 0x10de:	// NVIDIA
			{
				// X11 module prints VRAM size to log
				FILE *f;				
				snprintf(path_buf, sizeof(path_buf), "/var/log/Xorg.%d.log",
					gpuc_get_default_X_display());
				f = fopen(path_buf, "r");
				gpuc_parse_memory_from_log(f, " NVIDIA(%*d): Memory: %d kBytes",
					&d_memory, 1024);
				if (f)
					fclose(f);
				break;
			}
			case 0x1002:	// ATI/AMD
			{
				// drm kernel module prints VRAM size to dmesg
				FILE *p = popen("dmesg", "r");
				gpuc_parse_memory_from_log(p, " [drm] Detected VRAM RAM=%dM",
					&d_memory, 1024 * 1024);
				if (p)
					pclose(p);
				break;
			}
			case 0x8086:	// Intel
			{
				// the stolen memory reading is fine on Intel
			}
			default:
				gpuc_debugf("Can't detect actual VRAM quantity for this GPU "
					"vendor, falling back on DMA memory range");
				break;
		}
		found_callback(d_vendor, d_device, d_memory);
	}

	return 0;
}

#endif // GPU_COLLECTOR_H