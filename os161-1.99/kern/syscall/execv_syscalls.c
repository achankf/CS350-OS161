#include <types.h>
#include <syscall.h>
#include <proc.h>
#include <limits.h>
#include <lib.h>
#include <thread.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <uio.h>
#include <clock.h>
#include <vfs.h>
#include <test.h>
#include <sfs.h>
#include <current.h>
#include <vm.h>
#include <copyinout.h>
#include <mips/trapframe.h>
#include <addrspace.h>
#include <synch.h>
#include "opt-synchprobs.h"
#include "opt-sfs.h"
#include "opt-net.h"

int sys_execv(const char *program, char **args)
{
	int argc = 0;	
	char *kargs[256]; // max 256 args
	char kprogram[256]; // max program path length
	size_t got;
	int result;
	struct addrspace *old_as;

	for (int i = 0; i < 256; i++)
		kargs[i] = NULL;

	old_as = curproc_getas();
	// we should not be a new process
	KASSERT(old_as != NULL);


/*
	result = copyin((const_userptr_t) args, kargs, (sizeof (char**)));
	if(result)
	{
		kprintf("copyin arugment pointer failed\n");
		return result;
	}

*/	result = copyinstr((const_userptr_t)program, kprogram, 256, &got); 
	if(result)
	{
		DEBUG(DB_EXEC, "copyinstr program name failed\n");
		return result;
	}
	if (got == 256) // we don't know how long the original was
		return ENAMETOOLONG;

	result = 0; // no problems (yet)
	for(int i = 0; i < 256; i++)
	{
		const_userptr_t arg_ptr;
		result = copyin((const_userptr_t) (args + i), &arg_ptr, sizeof(char*));
		if (result)
		{
			DEBUG(DB_EXEC,"pointer to arg %d was bad\n", i);
			break;
		}

		if(arg_ptr == NULL)
			break;
		argc++;

		char tmp_arg[256];
		result = copyinstr(arg_ptr, tmp_arg, 256, &got);
		if (result) {
			DEBUG(DB_EXEC,"couldn't copy in arg %d\n", i);
			break;
		}
	
		int length = strlen(tmp_arg);
		kargs[i] = kmalloc(length+1);
		if (kargs[i] == NULL)
		{
			DEBUG(DB_EXEC,"couldn't kmalloc\n");
			result = ENOMEM;
			break;
		}
		strcpy(kargs[i], tmp_arg);
	}

	if (result) // something failed
	{
		for (int i = 0; i < 256; i++)
		{
			if (kargs[i] != NULL)
				kfree(kargs[i]);
		}
		return result;
	}

	as_deactivate();
	curproc_setas(NULL);

	result = runprogram(kprogram, argc, kargs, old_as, true);

	curproc_setas(old_as);
	as_activate();

	DEBUG(DB_EXEC,"runprogram failed\n");
	return result;
}


