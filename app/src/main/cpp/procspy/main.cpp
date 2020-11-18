#include <zconf.h>
#include <dlfcn.h>
#include "memassist.h"
#include "common.h"

MemAssist sc;

extern "C" int __attribute__ ((visibility ("default"))) main_entry(const char *str)
{
	LOGI("MemAssist: %s\n", str);
	if (strcmp(str, "Frist load") == 0)
	{
		std::string so_path = GetModulePath(-1, (void *)&GetModulePath);
		void *so = dlopen(so_path.c_str(), RTLD_NOW);
		int (*new_main_entry)(const char *str);
		*(void **)&new_main_entry = dlsym(so, "main_entry");
		new_main_entry("Second load");
		return 0;
	}
	sc.StartListen();
	return 0;
}

__attribute__((constructor)) void x_init()
{
	LOGI("MemAssist: so load");
}

__attribute__((destructor)) void x_fini()
{
	LOGI("MemAssist: so unload");
}
