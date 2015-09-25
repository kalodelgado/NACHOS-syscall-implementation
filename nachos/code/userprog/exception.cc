// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "console.h"
#include "synch.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

static void ConvertIntToHex (unsigned v, Console *console)
{
   unsigned x;
   if (v == 0) return;
   ConvertIntToHex (v/16, console);
   x = v % 16;
   if (x < 10) {
      writeDone->P() ;
      console->PutChar('0'+x);
   }
   else {
      writeDone->P() ;
      console->PutChar('a'+x-10);
   }
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;
    unsigned printvalus;        // Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);;

    if ((which == SyscallException) && (type == syscall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == syscall_PrintInt)) {
       printval = machine->ReadRegister(4);
       if (printval == 0) {
	  writeDone->P() ;
          console->PutChar('0');
       }
       else {
          if (printval < 0) {
	     writeDone->P() ;
             console->PutChar('-');
             printval = -printval;
          }
          tempval = printval;
          exp=1;
          while (tempval != 0) {
             tempval = tempval/10;
             exp = exp*10;
          }
          exp = exp/10;
          while (exp > 0) {
	     writeDone->P() ;
             console->PutChar('0'+(printval/exp));
             printval = printval % exp;
             exp = exp/10;
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintChar)) {
	writeDone->P() ;
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintString)) {
       vaddr = machine->ReadRegister(4);
       machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
          writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          machine->ReadMem(vaddr, 1, &memval);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintIntHex)) {
       printvalus = (unsigned)machine->ReadRegister(4);
       writeDone->P() ;
       console->PutChar('0');
       writeDone->P() ;
       console->PutChar('x');
       if (printvalus == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          ConvertIntToHex (printvalus, console);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 

    else if((which == SyscallException) && (type == syscall_GetReg)){
       machine->WriteRegister(2,machine->ReadRegister(machine->ReadRegister(4)));
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4); 
    }

    else if((which == SyscallException) && (type == syscall_GetPA)){
      unsigned virtAddress = machine->ReadRegister(4);
      unsigned vpn = (unsigned)virtAddress/PageSize;

      
      //vpn --- Virtual Page Numer (See translation.cc line:210)

      //The virtual page number is not larger than the number of entries in the page table. 
      if(vpn > machine->pageTableSize){
        machine->WriteRegister(2,-1);     
      }

    //The page table entry has the valid field set to true indicating that the entry holds a valid physical page number.      
      if (!machine->pageTable[vpn].valid)
      {
        machine->WriteRegister(2,-1);
      }

      //The obtained physical page number (physicalPage field of the page table entry) is not larger than the number of physical pages.
      if (machine->pageTable[vpn].physicalPage > NumPhysPages)
      {
        machine->WriteRegister(2,-1);
      }

      else
      {
        machine->WriteRegister(2,((machine->pageTable[vpn].physicalPage)*PageSize) + (virtAddress%PageSize));
      }

      //Advance Program Counters
      machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
      machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
      machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

    else if((which == SyscallException) && (type == syscall_Time)){
      machine->WriteRegister(2,stats->totalTicks);
      //stats is a global variable of class Statistics(see stats.h) which has been declared in code/threads/System.h and system.cc. please refer
      //Refer to machine/interrupt.cc to see how stats->totalticks works! Line 155 to be precise :P

      //Advance Program Counters
      machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
      machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
      machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    
    }


    //Implementation of GetPID and GetPPID from thread.cc and thread.h? Confused :(
    else if ((which == SyscallException) && (type == syscall_GetPID)) {
        machine->WriteRegister(2, currentThread->getPID());

        // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    else if ((which == SyscallException) && (type == syscall_GetPPID)) {
        machine->WriteRegister(2, currentThread->getPPID());

        // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    //Hehehehe IKR its!

    else if((which == SyscallException) && (type == syscall_Yield)){
      // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        //Already present in thread.cc
        currentThread->YieldCPU();
    }

    else if((which == SyscallException) && (type == syscall_Sleep)){
      // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        int WaitTime = machine->ReadRegister(4);

        if(WaitTime==0){
          //This will stop all interuppts when we are handling the sleep of this function
          //Check interrupts.h for implementation
          currentThread->YieldCPU();
          //Yield it
          //Start Interrupts again
        }
        else{
          //SleepingQueue has been defined, freshly in system.cc as a global variable. system.cc : Line 23 
          //Its implementation is incomplete. Yet is sufficient for purpose of sleep
          //Implementation of SortedInsert is present in list.h
          SleepingQueue->SortedInsert((void *)currentThread, stats->totalTicks + WaitTime);
          IntStatus oldlevel=interrupt->SetLevel(IntOff);
          currentThread->PutThreadToSleep();
          interrupt->SetLevel(oldlevel); 
        }
    }

    else if((which == SyscallException) && (type == syscall_Fork)){
      // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        //Create a child
        NachOSThread *child = new NachOSThread("Forked Thread");
        
        //Initialize the child
        child->parent = currentThread;
        
        //Update Parameters in parent
        currentThread->child_PIDs[currentThread->child_Count]=child->getPID();
        currentThread->child_Count++;

        child->space=new AddrSpace(currentThread->space->getNumPages(),currentThread->space->getStartPhysPage());

        machine->WriteRegister(2,0);
        child->SaveUserState(); //LOL

        machine->WriteRegister(2,child->getPID());

        //Allocate a stack to child
        child->ThreadStackAllocate(&myFunction,0);
        //But how?

        //WHy you do this? I was told to do so in the assignment :( 
        machine->Run();
        //Well this thing apparently swithces back to user from kernel

        IntStatus oldLevel = interrupt->SetLevel(IntOff); // disable interrupts
        scheduler->ReadyToRun(child);
        (void) interrupt->SetLevel(oldLevel); // re-enable interrupts

    }
    else if((which == SyscallException) && (type == syscall_Exec)){
        //I assume there is something called filename :(
        //where do I find

        //refer to line 130 above to know how to read from memory
        char filename[100];
        int i=0;

        vaddr = machine->ReadRegister(4);
        machine->ReadMem(vaddr, 1, &memval);
        while ((*(char*)&memval) != '\0') {
            filename[i]  = (char)memval;
            ++i;
            vaddr++;
            machine->ReadMem(vaddr, 1, &memval);
        }
        filename[i]  = (char)memval;
        //This by far is weirdestthing that has been copied

        if (executable == NULL) {
          printf("Unable to open file %s\n", filename);
          return;
        }


        Openfile *executable = filesystem->Openfile(machine->ReadRegister(4));
        
        AddrSpace *space;
        space = new AddrSpace(executable);
        currentThread->space=space;

        space->InitRegisters();
        space->RestoreState();
        delete executable;
        machine->Run();
        ASSERT(FALSE);      
        // machine->Run never returns;
          // the address space exits
          // by doing the syscall "exit"
        //no more incrementing count :)
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
