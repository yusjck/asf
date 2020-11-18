#include <stdio.h>
#include <stdlib.h>
#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <elf.h>
#include <android/log.h>
#include <linux/uio.h>

#if defined(__i386__) || defined(__x86_64__)
#define pt_regs         user_regs_struct
#elif defined(__aarch64__)
#define pt_regs         user_pt_regs
#define uregs       	regs
#define ARM_pc      	pc
#define ARM_sp      	sp
#define ARM_cpsr	    pstate
#define ARM_lr		    regs[30]
#define ARM_r0		    regs[0]
#define PTRACE_GETREGS  PTRACE_GETREGSET
#define PTRACE_SETREGS  PTRACE_SETREGSET
#endif

#define ENABLE_DEBUG 1

#if ENABLE_DEBUG
#define  LOG_TAG "INJECT"
#define  LOGD(fmt, args...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, fmt, ##args)
#define DEBUG_PRINT(format,args...) \
	LOGD(format, ##args)
#else
#define DEBUG_PRINT(format,args...)
#endif

#define CPSR_T_MASK     ( 1u << 5 )

#if defined(__aarch64__) || defined(__x86_64__)
const char *libc_path = "/system/lib64/libc.so";
const char *linker_path = "/system/bin/linker64";
#else
const char *libc_path = "/system/lib/libc.so";
const char *linker_path = "/system/bin/linker";
#endif

int ptrace_getregs(pid_t pid, struct pt_regs *regs)
{
#if defined (__aarch64__)
	int regset = NT_PRSTATUS;
	struct iovec ioVec;
	ioVec.iov_base = regs;
	ioVec.iov_len = sizeof(*regs);

	if (ptrace(PTRACE_GETREGSET, pid, (void *)regset, &ioVec) < 0)
	{
		perror("ptrace_getregs: Can not get register values");
		return -1;
	}
#else
	if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0)
	{
		perror("ptrace_getregs: Can not get register values");
		return -1;
	}
#endif

	return 0;
}

int ptrace_setregs(pid_t pid, struct pt_regs *regs)
{
#if defined (__aarch64__)
	int regset = NT_PRSTATUS;
	struct iovec ioVec;
	ioVec.iov_base = regs;
	ioVec.iov_len = sizeof(*regs);

	if (ptrace(PTRACE_SETREGSET, pid, (void *)regset, &ioVec) < 0)
	{
		perror("ptrace_setregs: Can not get register values");
		return -1;
	}
#else
	if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0)
	{
		perror("ptrace_setregs: Can not set register values");
		return -1;
	}
#endif

	return 0;
}

int ptrace_continue(pid_t pid)
{
	if (ptrace(PTRACE_CONT, pid, NULL, 0) < 0)
	{
		perror("ptrace_cont");
		return -1;
	}

	return 0;
}

int ptrace_attach(pid_t pid)
{
	if (ptrace(PTRACE_ATTACH, pid, NULL, 0) < 0)
	{
		perror("ptrace_attach");
		return -1;
	}

	int status = 0;
	waitpid(pid, &status , WUNTRACED);

	return 0;
}

int ptrace_detach(pid_t pid)
{
	if (ptrace(PTRACE_DETACH, pid, NULL, 0) < 0)
	{
		perror("ptrace_detach");
		return -1;
	}

	return 0;
}

int ptrace_readdata(pid_t pid, uint8_t *src, uint8_t *buf, size_t size)
{
	size_t i, j, remain;
	uint8_t *laddr;

	union u {
		long val;
		char chars[sizeof(long)];
	} d;

	j = size / sizeof(long);
	remain = size % sizeof(long);

	laddr = buf;

	for (i = 0; i < j; i++)
	{
		d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);
		memcpy(laddr, d.chars, sizeof(long));
		src += sizeof(long);
		laddr += sizeof(long);
	}

	if (remain > 0)
	{
		d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);
		memcpy(laddr, d.chars, remain);
	}

	return 0;
}

int ptrace_writedata(pid_t pid, uint8_t *dest, uint8_t *data, size_t size)
{
	size_t i, j, remain;
	uint8_t *laddr;

	union u {
		long val;
		char chars[sizeof(long)];
	} d;

	j = size / sizeof(long);
	remain = size % sizeof(long);

	laddr = data;

	for (i = 0; i < j; i++)
	{
		memcpy(d.chars, laddr, sizeof(long));
		ptrace(PTRACE_POKETEXT, pid, dest, (void *)d.val);
		dest += sizeof(long);
		laddr += sizeof(long);
	}

	if (remain > 0)
	{
		d.val = ptrace(PTRACE_PEEKTEXT, pid, dest, 0);
		memcpy(d.chars, laddr, remain);
		ptrace(PTRACE_POKETEXT, pid, dest, (void *)d.val);
	}

	return 0;
}

#if defined(__arm__) || defined(__aarch64__)
int ptrace_call(pid_t pid, uintptr_t addr, long *params, uint32_t num_params, struct pt_regs *regs)
{
	uint32_t i;

#if defined(__arm__)
	int num_param_registers = 4;
#elif defined(__aarch64__)
	int num_param_registers = 8;
#endif

	for (i = 0; i < num_params && i < num_param_registers; i ++)
	{
		regs->uregs[i] = params[i];
	}

	//
	// push remained params onto stack
	//
	if (i < num_params)
	{
		regs->ARM_sp -= (num_params - i) * sizeof(long) ;
		ptrace_writedata(pid, (uint8_t *)regs->ARM_sp, (uint8_t *)&params[i], (num_params - i) * sizeof(long));
	}

	regs->ARM_pc = addr;
	if (regs->ARM_pc & 1)
	{
		/* thumb */
		regs->ARM_pc &= (~1u);
		regs->ARM_cpsr |= CPSR_T_MASK;
	}
	else
	{
		/* arm */
		regs->ARM_cpsr &= ~CPSR_T_MASK;
	}

	regs->ARM_lr = 0;

	if (ptrace_setregs(pid, regs) == -1
			|| ptrace_continue(pid) == -1)
	{
		return -1;
	}

	int stat = 0;
	waitpid(pid, &stat, WUNTRACED);
	while (stat != 0xb7f)
	{
		if (ptrace_continue(pid) == -1)
		{
			return -1;
		}
		waitpid(pid, &stat, WUNTRACED);
	}

	return 0;
}

#elif defined(__i386__)
long ptrace_call(pid_t pid, uintptr_t addr, long *params, uint32_t num_params, struct user_regs_struct *regs)
{
	regs->esp -= (num_params) * sizeof(long) ;
	ptrace_writedata(pid, (uint8_t *)regs->esp, (uint8_t *)params, (num_params) * sizeof(long));

	long tmp_addr = 0x00;
	regs->esp -= sizeof(long);
	ptrace_writedata(pid, (uint8_t *)regs->esp, (uint8_t *)&tmp_addr, sizeof(tmp_addr));

	regs->eip = addr;

	if (ptrace_setregs(pid, regs) == -1
			|| ptrace_continue( pid) == -1)
	{
		return -1;
	}

	int stat = 0;
	waitpid(pid, &stat, WUNTRACED);
	while (stat != 0xb7f)
	{
		if (ptrace_continue(pid) == -1)
		{
			return -1;
		}
		waitpid(pid, &stat, WUNTRACED);
	}

	return 0;
}

#elif defined(__x86_64__)
long ptrace_call(pid_t pid, uintptr_t addr, long *params, uint32_t num_params, struct user_regs_struct *regs)
{
	if (num_params > 6)
	{
		regs->rsp -= (num_params - 6) * sizeof(long);
		ptrace_writedata(pid, (uint8_t *)regs->rsp, (uint8_t *)&params[6], (num_params - 6) * sizeof(long));
	}

	unsigned long *paramRegs[] = {&regs->rdi, &regs->rsi, &regs->rdx, &regs->rcx, &regs->r8, &regs->r9};
	for (uint32_t i = 0; i < num_params && i < 6; i++)
	{
		*paramRegs[i] = (unsigned long)params[i];
	}

	long tmp_addr = 0x00;
	regs->rsp -= sizeof(long);
	ptrace_writedata(pid, (uint8_t *)regs->rsp, (uint8_t *)&tmp_addr, sizeof(tmp_addr));

	regs->rip = addr;

	if (ptrace_setregs(pid, regs) == -1
			|| ptrace_continue( pid) == -1)
	{
		return -1;
	}

	int stat = 0;
	waitpid(pid, &stat, WUNTRACED);
	while (stat != 0xb7f)
	{
		if (ptrace_continue(pid) == -1)
		{
			return -1;
		}
		waitpid(pid, &stat, WUNTRACED);
	}

	return 0;
}
#else
#error "Not supported"
#endif

void *get_module_base(pid_t pid, const char *module_name)
{
	FILE *fp;
	long addr = 0;
	char *pch;
	char filename[32];
	char line[1024];

	if (pid < 0)
	{
		/* self process */
		snprintf(filename, sizeof(filename), "/proc/self/maps");
	}
	else
	{
		snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
	}

	fp = fopen(filename, "r");
	if (fp != NULL)
	{
		while (fgets(line, sizeof(line), fp))
		{
			if (strstr(line, module_name))
			{
				pch = strtok( line, "-" );
				addr = strtoul( pch, NULL, 16 );

				if (addr == 0x8000)
					addr = 0;

				break;
			}
		}

		fclose(fp) ;
	}

	return (void *)addr;
}

void *get_remote_addr(pid_t target_pid, const char *module_name, void *local_addr)
{
	void *local_handle, *remote_handle;

	local_handle = get_module_base(-1, module_name);
	remote_handle = get_module_base(target_pid, module_name);

	DEBUG_PRINT("[+] get_remote_addr: local[%lx], remote[%lx]\n", (unsigned long)local_handle, (unsigned long)remote_handle);

	return (void *)((uintptr_t)local_addr + (uintptr_t)remote_handle - (uintptr_t)local_handle);
}

int find_pid_of(const char *process_name)
{
	int id;
	pid_t pid = -1;
	DIR *dir;
	FILE *fp;
	char filename[32];
	char cmdline[256];

	struct dirent *entry;

	if (process_name == NULL)
		return -1;

	dir = opendir("/proc");
	if (dir == NULL)
		return -1;

	while((entry = readdir(dir)) != NULL)
	{
		id = atoi(entry->d_name);
		if (id != 0)
		{
			sprintf(filename, "/proc/%d/cmdline", id);
			fp = fopen(filename, "r");
			if (fp)
			{
				fgets(cmdline, sizeof(cmdline), fp);
				fclose(fp);

				if (strcmp(process_name, cmdline) == 0)
				{
					/* process found */
					pid = id;
					break;
				}
			}
		}
	}

	closedir(dir);
	return pid;
}

long ptrace_retval(struct pt_regs * regs)
{
#if defined(__arm__) || defined(__aarch64__)
	return regs->ARM_r0;
#elif defined(__i386__)
	return regs->eax;
#elif defined(__x86_64__)
	return regs->rax;
#else
#error "Not supported"
#endif
}

long ptrace_ip(struct pt_regs * regs)
{
#if defined(__arm__) || defined(__aarch64__)
	return regs->ARM_pc;
#elif defined(__i386__)
	return regs->eip;
#elif defined(__x86_64__)
	return regs->rip;
#else
#error "Not supported"
#endif
}

int ptrace_call_wrapper(pid_t target_pid, const char *func_name, void *func_addr, long *parameters, uint32_t param_num, struct pt_regs *regs)
{
	DEBUG_PRINT("[+] Calling %s in target process.\n", func_name);
	if (ptrace_call(target_pid, (uintptr_t)func_addr, parameters, param_num, regs) == -1)
		return -1;

	if (ptrace_getregs(target_pid, regs) == -1)
		return -1;
	DEBUG_PRINT("[+] Target process returned from %s, return value=%lx, pc=%lx \n",
			func_name, (unsigned long)ptrace_retval(regs), (unsigned long)ptrace_ip(regs));
	return 0;
}

int inject_remote_process(pid_t target_pid, const char *library_path, const char *function_name, const char *param)
{
	int ret = -1;
	void *mmap_addr, *munmap_addr, *dlopen_addr, *dlsym_addr, *dlclose_addr, *dlerror_addr;
	void *dlhandle;
	long map_base = 0;
	long sohandle, main_entry_addr;
	struct pt_regs regs, original_regs;
	long parameters[10];

	DEBUG_PRINT("[+] Injecting process: %d\n", target_pid);

	if (ptrace_attach(target_pid) == -1)
		goto exit0;

	if (ptrace_getregs(target_pid, &regs) == -1)
		goto exit0;

	/* save original registers */
	memcpy(&original_regs, &regs, sizeof(regs));

	dlhandle = dlopen(libc_path, RTLD_LAZY);
	mmap_addr = dlsym(dlhandle, "mmap");
	munmap_addr = dlsym(dlhandle, "munmap");
	dlclose(dlhandle);

	mmap_addr = get_remote_addr(target_pid, libc_path, mmap_addr);
	munmap_addr = get_remote_addr(target_pid, libc_path, munmap_addr);
	DEBUG_PRINT("[+] Remote mmap address: %lx, munmap address: %lx\n", (unsigned long)mmap_addr, (unsigned long)munmap_addr);

	/* call mmap */
	parameters[0] = 0;  // addr
	parameters[1] = 0x4000; // size
	parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;  // prot
	parameters[3] = MAP_ANONYMOUS | MAP_PRIVATE; // flags
	parameters[4] = 0; //fd
	parameters[5] = 0; //offset

	if (ptrace_call_wrapper(target_pid, "mmap", mmap_addr, parameters, 6, &regs) == -1)
		goto exit1;

	map_base = ptrace_retval(&regs);

	dlopen_addr = (void *)dlopen;
	dlsym_addr = (void *)dlsym;
	dlclose_addr = (void *)dlclose;
	dlerror_addr = (void *)dlerror;

	dlopen_addr = get_remote_addr( target_pid, linker_path, dlopen_addr );
	dlsym_addr = get_remote_addr( target_pid, linker_path, dlsym_addr );
	dlclose_addr = get_remote_addr( target_pid, linker_path, dlclose_addr );
	dlerror_addr = get_remote_addr( target_pid, linker_path, dlerror_addr );

	DEBUG_PRINT("[+] Get imports: dlopen: %lx, dlsym: %lx, dlclose: %lx, dlerror: %lx\n",
			(unsigned long)dlopen_addr, (unsigned long)dlsym_addr, (unsigned long)dlclose_addr, (unsigned long)dlerror_addr);

	ptrace_writedata(target_pid, (uint8_t *)map_base, (uint8_t *)library_path, strlen(library_path) + 1);

	parameters[0] = map_base;
	parameters[1] = RTLD_NOW | RTLD_GLOBAL;

	if (ptrace_call_wrapper(target_pid, "dlopen", dlopen_addr, parameters, 2, &regs) == -1)
		goto exit2;

	sohandle = ptrace_retval(&regs);
	if (sohandle == NULL)
		goto exit4;

#define FUNCTION_NAME_ADDR_OFFSET       0x100
	ptrace_writedata(target_pid, (uint8_t *)map_base + FUNCTION_NAME_ADDR_OFFSET, (uint8_t *)function_name, strlen(function_name) + 1);
	parameters[0] = sohandle;
	parameters[1] = map_base + FUNCTION_NAME_ADDR_OFFSET;

	if (ptrace_call_wrapper(target_pid, "dlsym", dlsym_addr, parameters, 2, &regs) == -1)
		goto exit3;

	main_entry_addr = ptrace_retval(&regs);
	if (main_entry_addr == NULL)
		goto exit4;

#define FUNCTION_PARAM_ADDR_OFFSET      0x200
	ptrace_writedata(target_pid, (uint8_t *)map_base + FUNCTION_PARAM_ADDR_OFFSET, (uint8_t *)param, strlen(param) + 1);
	parameters[0] = map_base + FUNCTION_PARAM_ADDR_OFFSET;

	if (ptrace_call_wrapper(target_pid, function_name, (void *)main_entry_addr, parameters, 1, &regs) == -1)
		goto exit3;

	ret = 0;

exit3:
	parameters[0] = sohandle;
	ptrace_call_wrapper(target_pid, "dlclose", dlclose_addr, parameters, 1, &regs);

exit2:
	parameters[0] = map_base;
	parameters[1] = 0x4000;
	ptrace_call_wrapper(target_pid, "munmap", munmap_addr, parameters, 2, &regs);

exit1:
	/* restore */
	ptrace_setregs(target_pid, &original_regs);
	ptrace_detach(target_pid);

exit0:
	return ret;

exit4:
	if (ptrace_call_wrapper(target_pid, "dlerror", dlerror_addr, NULL, 0, &regs) != -1)
	{
		long addr = ptrace_retval(&regs);
		if (addr)
		{
			char err[100] = {0};
			ptrace_readdata(target_pid, (uint8_t *)addr, (uint8_t *)err, 99);
			DEBUG_PRINT("[-] Inject failed, function_name=%s, err=%s\n", function_name, err);
		}
	}
	goto exit3;
}
