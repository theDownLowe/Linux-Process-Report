// Trevor N. Lowe
// University of Washington, Tacoma
// Winter 2017
// TCSS 422: OS
// Assignment 2

// ----- OUTLINE -----

// Creates a Linux Kernel Module that generates a report describing the running processes on a Linux system.
//	- The module traverses the list of running processes, and introspects info about them.

// Uses the Linux Kernel Linked LIst functions to:
//	1) Iterate through the master process list
//	2) Drill down into a process's list of child processes.

// Generates and provide report output /proc


// ----- CODE -----

#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

// Stores information about a process.
struct Process {
	int proc_id;		// ID # of proc
	char* proc_name;	// Name of proc
	int numChild;		// # of children
	int fchild_id;		// PID of proc's 1st child
	char* fchild_name;	// Name of proc's 1st child
} procList[500];

int unrunnable;			// # of unrunnable procs
int runnable;			// # of runnable procs
int stopped;			// # of stopped procs
int procNum;			// # of procs


// Computes the list of processes.
void compList(void) {

	struct task_struct *task;

	unrunnable = 0;
	runnable = 0;
	stopped = 0;
	procNum = 0;

	// Loops through each process
	for_each_process(task) {
		struct task_struct *child;
		struct list_head *list;
		long t_state = 0;
		int firstChild = 1;			

		// Determine the state
		t_state = (long)task->state;
		if (t_state == -1) {	// Unrunnable
			unrunnable++;
		} else if (t_state == 0) {// Runnable
			runnable++;
		} else {		// Stopped
			stopped++;
		}

		// Determine PID
		procList[procNum].proc_id = (int)task->pid;	

		// Determine name
		procList[procNum].proc_name = (char*)task->comm;

		// Determine first child pid and name, and number of children for a process
		list_for_each(list, &task->children) {
			child = list_entry(list, struct task_struct, sibling);
			if (firstChild == 1) {
				procList[procNum].fchild_id = (int)child->pid;
				procList[procNum].fchild_name = (char*)child->comm;
				firstChild = 0;
			}

			procList[procNum].numChild++;
		}

		procNum++;
	}
}

// Generates a report of the processes from compList()
static int genReport_show(struct seq_file *m, void*v) {

	int i;

	seq_printf(m, "PROCESS REPORTER:\n");
	seq_printf(m, "Unrunnable:%d\n", unrunnable);
	seq_printf(m, "Runnable:%d\n", runnable); 
	seq_printf(m, "Stopped:%d\n", stopped);
	
	for (i = 0; i < procNum; i++) {
		if (procList[i].numChild > 0) { // Proc hasChild
			seq_printf(m, "Process ID=%d Name=%s number_of_children=%d first_child_pid=%d first_child_name=%s\n", procList[i].proc_id, procList[i].proc_name, procList[i].numChild, procList[i].fchild_id, procList[i].fchild_name);
		} else {
			seq_printf(m, "Process ID=%d Name=%s *No Children\n", procList[i].proc_id, procList[i].proc_name);
		}
	}

	return 0;

}

static int genReport_open(struct inode *inode, struct file *file) {
	return single_open(file, genReport_show, NULL);
}

static const struct file_operations proc_report_fops = {
	.owner = THIS_MODULE,
	.open = genReport_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int proc_init(void) {
	
	printk(KERN_INFO "procReport: Kernel module initialized\n");

	compList();
	proc_create("proc_report", 0, NULL, &proc_report_fops);

	return 0;
}

void proc_cleanup(void) {

	printk(KERN_INFO "procReport: Performing cleanup of module\n");
	remove_proc_entry("proc_report", NULL);
}

MODULE_LICENSE("GPL");
module_init(proc_init);
module_exit(proc_cleanup);
