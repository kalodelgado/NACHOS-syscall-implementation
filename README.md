

#syscall_GetReg
1.Returns the value present int the register asked for.
This is a straightforward system call, we read value of register using machine::ReadRegister and return using machine::WriteRegister.


#syscall_GetPA
1.Getting Physical Address isn't a straight task.
We check for various conditions where it had to fail, and return -1 for them.

2.Otherwise we directly get the physical address, by using the machine->pageTable, PageSize, virtual page number and the virtualAddress.

#syscall_GetPID
1. A new static variable called pidCount is defined in NachOSThread class and a reference has been made in thread.h

2. The Constructor for the NachOSThread has been modified to increment pidCount and assign this pidCount to the current thread as it PID!.

3. We have assumed that there can be infinitely many PID's. No wrapping has been done over the number of processes possible. 

4. Now it is a simple task of calling for currentThread->pid

#syscall_GetPPID
1. In the NachOSThread constructor we assign the ppid of new thread being constructed as the pid of current thread. (if this procees has a pid=1 then ppid is set to be 0)

2. ppid also gets reassigned if  a parent exits.

3. Now it is a simple task of calling for currentThread->ppid.

#syscall_GetNumInstr
1. d


#syscall_Time
1. There exists a global variable totalTicks in the stats class(Available in stats.cc and stats.h).

2. Return stats->totalTicks

#syscall_Yield
1. YieldCPU() a memeber function of NachOSThread was able to do the required task.

2. Only important thing to do was to increment program counters before calling YieldCPU()

#syscall_Sleep
1. Generated a new List called SleepingQueue, to which all the sleeping processes are appended and are maintained with their primary key as their wakeUpTime.

2. TimerInterruptHandler has been redefined to handle the Sleeping Queue, so that the processes that have 'woken up', that is their wakeUpTime <= currentTime, are dequeued from SleepingQueue and enqued into the ReadyQueue. 



#syscall_Exec
1. First 'filename' was generated as it was generated in the Print_String.

2. Now using the filename we create an OpenFile instance.

3. It is allocated space using AddrSpace.

4. AddrSpace constructor was modified to take into account the total number of pages created till now.

5. Registers are initialised, and state is restored for new process

6. Return back calling Run();


#syscall_Fork
1. To begin with, the value of PC was incremented follwing which a new NachOSThread Object was created.

2. The parent pointer of this new NachOSThread was changed to point to the currentThread, which is the creator of this forked thread.

3. The new step was to create an Address Space, the constructor of AddrSpace class was overloaded to handle the case when we fork threads. 
    
    a. In this new constructor, the Page Table mapped virtual address to physical address with a function virtual address + totalPagesCount(Where totalPagesCount is a global which stores the total number of physical pages which have been alloted as of yet.)
    
    b. Next we duplicated the parent's physical memory exactly into the child's physical memory location, a simple copy in machine->mainMemory

4. Now the return value of child was changed to zero and it's state is saved (inspired from NachOSThread::SaveUserState)

5. The parent's return address is set to the child's pid and the child's stack is set to the function 'myFunction' which is defined in thread.h. This essentially mimics the behaviour a normal thread has on returning from a sleep. This is done to ensure that a newly forked thread behaves like any other woken thread.

6. After this the child is scheduled to run.

#syscall_Join
1. To begin with, the value of PC was incremented follwing which a new NachOSThread Object was created.

2. There is three possible states in which child can be found terminated, running, not found(i.e. child not yet made). Tckeled using an array 'Child_Status'.

3. If child is terminated or not yet made then simply return the return value or -1 respectively.

4. If child is running, set 'WaitingFor' flag of parent to its pid and add parent to waiting queue.

5. On termination of every process search waiting queue to see if any process is waiting for it termination. If yes then wake it up.   


#syscall_Exit
1. Incrment program counter.

2. For all child of the current process assign parent NULL and attach it to root(i.e. ppid=0).

3. Remove instance of the current thread from its parent's child_list. And set 'Child_Status' to 0 indicating termination of child(current thread) and store the return value.

4. See if parent process is waiting its termination then wake it by removing it from WaitingQueue and inserting in readyQueue(done in WaitingQueue->RemovePid(currentThread->getPID());)

5. At last call FinishThread on current process.    










