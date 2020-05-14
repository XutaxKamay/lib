#ifndef COMMUNICATE_H
#define COMMUNICATE_H

extern spinlock_t g_spin_lock;
task_t** get_current_task_ptr(void);

communicate_error_t
communicate_read__read_struct(task_t* task,
                              uintptr_t address,
                              communicate_read_t* communicate_read);

communicate_error_t
communicate_read__write_struct(task_t* task,
                               uintptr_t address,
                               communicate_write_t* communicate_write);

communicate_error_t communicate_read__remote_mmap_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_mmap_t* communicate_remote_mmap);

communicate_error_t communicate_read__remote_munmap_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_munmap_t* communicate_remote_munmap);

communicate_error_t communicate_read__remote_clone_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_clone_t* communicate_remote_clone);

communicate_error_t communicate_process_cmd_read(task_t* task,
                                                 uintptr_t address);

communicate_error_t communicate_process_cmd_write(task_t* task,
                                                  uintptr_t address);

communicate_error_t
communicate_process_cmd_remote_mmap(task_t* task, uintptr_t address);

communicate_error_t
communicate_process_cmd_remote_munmap(task_t* task, uintptr_t address);

communicate_error_t
communicate_process_cmd_remote_clone(task_t* task, uintptr_t address);

#endif
