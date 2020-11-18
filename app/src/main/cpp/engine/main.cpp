//
// Created by Jack on 2019/7/22.
//

#include <unistd.h>
#include "common.h"
#include "engineservice.h"
#include "../control/minitouch.h"

EngineService g_engineService;

__attribute__((constructor)) void x_init()
{
	LOGI("init minitouch\n");
	minitouch_init();
}

__attribute__((destructor)) void x_fini()
{
	LOGI("uninit minitouch\n");
	minitouch_uninit();
}

extern "C" int __attribute__ ((visibility ("default"))) start_service()
{
	char workDir[MAX_PATH];
	getcwd(workDir, sizeof(workDir));
	LOGI("working dir is %s\n", workDir);

	if (!g_engineService.Start())
	{
		LOGI("start service failed\n");
		return -1;
	}

	LOGI("engine service started");
	return 0;
}

extern "C" void __attribute__ ((visibility ("default"))) stop_service()
{
	g_engineService.Stop();
	LOGI("engine service stopped");
}

extern "C" int __attribute__ ((visibility ("default"))) run_service()
{
	if (start_service() != 0)
		return -1;
	while (getchar() != 'q')
		sleep(1);
	return 0;
}
