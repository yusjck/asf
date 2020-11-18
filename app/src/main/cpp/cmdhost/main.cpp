#include <dlfcn.h>
#include <unistd.h>
#include <errno.h>
#include <cstdio>

int main(int argc, char *argv[])
{
	if (argc != 3)
		return EINVAL;

	chdir(argv[2]);

	void *srvmod = dlopen(argv[1], RTLD_NOW);
	if (srvmod == NULL)
	{
		printf("%s\n", dlerror());
		return errno;
	}

	int (*run_service)();
	*(void **)&run_service = dlsym(srvmod, "run_service");
	if (run_service == NULL)
	{
		printf("%s\n", dlerror());
		return errno;
	}

	return run_service();
}
