#include "memutils.h"

void vm_flags_to_string(struct vm_area_struct *vma, char *output, int size)
{
	if (size < 7) {
		c_printk("string buffer doesn't have enough memory"
			 " to write memory protection "
			 "flags\n");
		return;
	}

	strcpy(output, "------");

	if (vma->vm_flags & VM_READ) {
		output[0] = 'r';
	}
	if (vma->vm_flags & VM_WRITE) {
		output[1] = 'w';
	}
	if (vma->vm_flags & VM_EXEC) {
		output[2] = 'x';
	}
	if (vma->vm_flags & VM_SHARED) {
		output[3] = 's';
	}
	if (vma->vm_flags & VM_GROWSDOWN) {
		output[4] = 'd';
	}
	if (vma->vm_flags & VM_GROWSUP) {
		output[5] = 'g';
	}

	output[6] = '\0';
}

int vm_flags_to_prot(struct vm_area_struct *vma)
{
	int prot;

	prot = PROT_NONE;

	if (vma->vm_flags & VM_READ) {
		prot |= PROT_READ;
	}
	if (vma->vm_flags & VM_WRITE) {
		prot |= PROT_WRITE;
	}
	if (vma->vm_flags & VM_EXEC) {
		prot |= PROT_EXEC;
	}
	if (vma->vm_flags & VM_GROWSDOWN) {
		prot |= PROT_GROWSDOWN;
	}
	if (vma->vm_flags & VM_GROWSUP) {
		prot |= PROT_GROWSUP;
	}

	return prot;
}

int prot_to_vm_flags(int prot)
{
	int flags;

	flags = VM_NONE;

	if (prot & PROT_READ) {
		flags |= VM_READ;
	}
	if (prot & PROT_WRITE) {
		flags |= VM_WRITE;
	}
	if (prot & PROT_EXEC) {
		flags |= VM_EXEC;
	}
	if (prot & PROT_GROWSDOWN) {
		flags |= VM_GROWSDOWN;
	}
	if (prot & PROT_GROWSUP) {
		flags |= VM_GROWSUP;
	}

	return flags;
}

// Find the part of virtual memory of the virtual address space range
// of the process that contains our virtual address.
int c_find_vma_from_task(struct task_struct *task,
			 struct vm_area_struct **vma_start,
			 unsigned long wanted_addr)
{
	struct mm_struct *mm;

	// Kernel thread?
	mm = get_task_mm(task);

	if (mm == NULL)
		goto out;

	*vma_start = mm->mmap;

	// Sometimes it can be null pointer as for systemdd process.
	if (*vma_start == NULL)
		goto out;

	// Loop through all the linkedlist of mapped memory areas until we find our
	// address.
	while (true) {
		// Waw we found it
		if (wanted_addr >= (*vma_start)->vm_start &&
		    wanted_addr < (*vma_start)->vm_end)
			break;

		// Go on the next mapped memory area
		*vma_start = (*vma_start)->vm_next;

		// Ouch we reached the end of the linked list, we didn't find anything.
		if (*vma_start == NULL)
			break;
	}

	return *vma_start == NULL ? 0 : 1;

out:
	return 0;
}

void c_print_vmas(struct task_struct *task)
{
	struct mm_struct *mm;
	struct vm_area_struct *vma_start;
	// Protect flags translated...
	char strflags[7];

	mm = get_task_mm(task);

	// Kernel thread?
	if (mm == NULL)
		mm = task->active_mm;

	if (mm == NULL)
		return;

	vma_start = mm->mmap;

	// Sometimes it can be null pointer as for systemdd process.
	if (vma_start != NULL) {
		// Loop through all the linkedlist of mapped memory areas until we find
		// our address.
		while (true) {
			vm_flags_to_string(vma_start, strflags,
					   sizeof(strflags));
			c_printk(
				"%s(%i); start: 0x%lX; end: 0x%lX; current flags: %s\n",
				task->comm, task->pid, vma_start->vm_start,
				vma_start->vm_end, strflags);

			// Go on the next mapped memory area
			vma_start = vma_start->vm_next;

			// Ouch we reached the end of the linked list, we didn't find
			// anything.
			if (vma_start == NULL)
				break;
		}
	}
}

struct task_struct *find_task_from_addr(unsigned long address)
{
	struct task_struct *task;
	struct vm_area_struct *vma;
	task = NULL;
	vma = NULL;

	for_each_process (task) {
		if (c_find_vma_from_task(task, &vma, address)) {
			break;
		}
	}

	return task;
}

int scan_pattern(unsigned long start, unsigned long end, char *pattern, int len,
		 struct buffer_struct *buf)
{
	unsigned long iter;
	char *pattern_c;
	int pattern_byte;

	if (buf == NULL)
		return 0;

	while (start < end) {
		iter = start;

		for (pattern_c = pattern; pattern_c < pattern + len;
		     pattern_c += 3) {
			// If it's an unknown byte, we skip the byte.
			if (*pattern_c == '?' &&
			    *(char *)((unsigned long)pattern_c + 1) == '?') {
				iter++;
				continue;
			}

			pattern_byte = hex_char_to_byte(
				*pattern_c,
				*(char *)((unsigned long)pattern_c + 1));

			if (pattern_byte == 0x100) {
				c_printk("wrong pattern (%li): %s\n",
					 (unsigned long)pattern_c -
						 (unsigned long)pattern,
					 pattern);
				return 0;
			}

			if (pattern_byte != (int)(*(unsigned char *)iter))
				goto dontmatch;

			iter++;
		}

		c_printk("found: %lX with pattern\n%s\n", iter, pattern);

		if (buf->addr == NULL) {
			buf->size = sizeof(void *);
			buf->addr = kmalloc(buf->size, GFP_KERNEL);

			if (buf->addr == NULL) {
				c_printk("kmalloc failed: with pattern\n%s\n",
					 pattern);
				return 0;
			}

			*(unsigned long *)(buf->addr) = start;
		} else {
			buf->size += sizeof(void *);
			buf->addr = krealloc(buf->addr, buf->size, GFP_KERNEL);

			if (buf->addr == NULL) {
				c_printk("krealloc failed: with pattern \n%s\n",
					 pattern);

				return 0;
			}

			*(unsigned long *)(buf->addr - sizeof(void *)) = start;
		}

	dontmatch:
		start++;
	}

	if (buf->addr == NULL) {
		c_printk("didn't find pattern\n%s\n", pattern);
		return 0;
	} else {
		return 1;
	}
}

int scan_task(struct task_struct *task, char *pattern, int len,
	      struct buffer_struct *buf)
{
	struct vm_area_struct *vma;
	struct mm_struct *mm;
	int ret;
	void *copied_user_memory;

	copied_user_memory = NULL;

	mm = get_task_mm(task);

	// Kernel thread?
	if (mm == NULL)
		mm = task->active_mm;

	if (mm == NULL) {
		c_printk("task %s(%i) has no mm struct!\n", task->comm,
			 task->pid);
		return 0;
	}

	vma = mm->mmap;

	if (vma == NULL) {
		c_printk("task %s(%i) has no mmap struct!\n", task->comm,
			 task->pid);
		return 0;
	}

	while (true) {
		c_printk("vma scanning... %lX\n", vma->vm_start);

		copied_user_memory =
			kmalloc(vma->vm_end - vma->vm_start, GFP_KERNEL);

		if (!copy_from_user(copied_user_memory, (void *)vma->vm_start,
				    vma->vm_end - vma->vm_start)) {
			kfree(copied_user_memory);
			c_printk(
				"couldn't copy memory from task %s(%i) at %lX!\n",
				task->comm, task->pid, vma->vm_start);
			break;
		}

		ret = scan_pattern((unsigned long)copied_user_memory,
				   (unsigned long)copied_user_memory +
					   (vma->vm_end - vma->vm_start),
				   pattern, len, buf);

		if (ret)
			return ret;

		vma = vma->vm_next;

		if (vma == NULL)
			break;
	}

	kfree(copied_user_memory);
	return 0;
}

int scan_kernel(char *start, char *end, char *pattern, int len,
		struct buffer_struct *buf)
{
	unsigned long addr_start, addr_end;

	addr_start = kallsyms_lookup_name(start);
	addr_end = kallsyms_lookup_name(end);

	if (addr_start == 0 || addr_end == 0) {
		c_printk("wrong start - end addr: %lX - %lX\n", addr_start,
			 addr_end);
		return 0;
	}

	if (addr_start > addr_end) {
		unsigned long swap = addr_start;
		addr_start = addr_end;
		addr_end = swap;
	}

	c_printk("scanning with start - end addr: %lX - %lX\n", addr_start,
		 addr_end);

	return scan_pattern(addr_start, addr_end, pattern, len, buf);
}

unsigned long map_base_task(struct task_struct *task)
{
	struct mm_struct *mm;

	if (task == NULL) {
		c_printk("wrong task struct\n");
		return 0;
	}

	mm = get_task_mm(task);

	// Kernel thread?
	if (mm == NULL)
		mm = task->active_mm;

	if (mm == NULL) {
		c_printk("couldn't find base address of %s(%i)\n", task->comm,
			 task->pid);
		return 0;
	}

	c_printk("found base address of %s(%i) (%lX)\n", task->comm, task->pid,
		 mm->mmap_base);

	return mm->mmap_base;
}

unsigned long kernel_offset(void)
{
	// 0xffffffff81000000 is the usual base address of text section.
	return kallsyms_lookup_name("_text") - 0xffffffff81000000;
}

void change_vm_flags(struct vm_area_struct *vma, int new_flags, int *old_flags)
{
	int all_prot_to_flags;

	all_prot_to_flags =
		VM_READ | VM_WRITE | VM_EXEC | VM_GROWSDOWN | VM_GROWSUP;

	if (old_flags != NULL) {
		*old_flags = vm_flags_to_prot(vma);
	}

	// Do not delete special vm flags.
	vma->vm_flags &= ~all_prot_to_flags;
	vma->vm_flags |= prot_to_vm_flags(new_flags);
}

void remote_mprotect(pid_t pid, uintptr_t address, int new_flags,
		     int *old_flags)
{
	struct task_struct *task;
	struct vm_area_struct *vma_start;

	task = pid_task(find_vpid(pid), PIDTYPE_PID);

	vma_start = NULL;

	if (c_find_vma_from_task(task, &vma_start, address)) {
		change_vm_flags(vma_start, new_flags, old_flags);
	}
}

struct vm_area_struct *vm_area_alloc(struct mm_struct *mm)
{
	typedef struct vm_area_struct *(*vm_area_alloc_t)(struct mm_struct *);
	static vm_area_alloc_t p_vm_area_alloc = NULL;

	if (p_vm_area_alloc == NULL) {
		p_vm_area_alloc =
			(vm_area_alloc_t)kallsyms_lookup_name("vm_area_alloc");
	}

	if (p_vm_area_alloc == NULL) {
		c_printk("couldn't find vm_area_alloc\n");
		return NULL;
	}

	return p_vm_area_alloc(mm);
}

void vm_area_free(struct vm_area_struct *vma)
{
	typedef void (*vm_area_free_t)(struct vm_area_struct *);
	static vm_area_free_t p_vm_area_free = NULL;

	if (p_vm_area_free == NULL) {
		p_vm_area_free =
			(vm_area_free_t)kallsyms_lookup_name("vm_area_free");
	}

	if (p_vm_area_free == NULL) {
		c_printk("couldn't find vm_area_free\n");
		return;
	}

	p_vm_area_free(vma);
}

int insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vma)
{
	typedef int (*insert_vm_struct_t)(struct mm_struct *,
					  struct vm_area_struct *);

	static insert_vm_struct_t p_insert_vm_struct = NULL;

	if (p_insert_vm_struct == NULL) {
		p_insert_vm_struct = (insert_vm_struct_t)kallsyms_lookup_name(
			"insert_vm_struct");
	}

	if (p_insert_vm_struct == NULL) {
		c_printk("couldn't find insert_vm_struct\n");
		return -1;
	}

	return p_insert_vm_struct(mm, vma);
}

void remote_mmap(pid_t pid, uintptr_t address, int prot)
{
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	task = pid_task(find_vpid(pid), PIDTYPE_PID);

	if (task == NULL) {
		c_printk("couldn't find task %i\n", pid);
		return;
	}

	mm = get_task_mm(task);

	// Kernel thread?
	if (mm == NULL)
		mm = task->active_mm;

	if (mm == NULL) {
		c_printk("couldn't find mm from task %i\n", pid);
		return;
	}

	vma = vm_area_alloc(mm);

	if (vma == NULL) {
		c_printk("couldn't allocate vma from task %i\n", pid);
		return;
	}

	vma->vm_start = address;
	vma->vm_end = address + PAGE_SIZE;
	vma->vm_flags = prot_to_vm_flags(prot) | mm->def_flags;
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	vma->vm_ops = NULL;
	vma->vm_private_data = NULL;

	// down_write(&current->mm->mmap_sem);

	if (insert_vm_struct(mm, vma) < 0) {
		// up_write(&current->mm->mmap_sem);
		vm_area_free(vma);
		c_printk("couldn't insert vma in mm struct from task %i\n",
			 pid);
		return;
	}

	// up_write(&current->mm->mmap_sem);
}