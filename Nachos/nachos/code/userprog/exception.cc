// exception.cc 
//      Entry point into the Nachos kernel from user programs.
//      There are two kinds of things that can cause control to
//      transfer back to here from user code:
//
//      syscall -- The user code explicitly requests to call a procedure
//      in the Nachos kernel.  Right now, the only function we support is
//      "Halt".
//
//      exceptions -- The user code does something that the CPU can't handle.
//      For instance, accessing memory that doesn't exist, arithmetic errors,
//      etc.  
//
//      Interrupts (which can also cause control to transfer from user
//      code into the Nachos kernel) are handled elsewhere.
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
#include "consoledriver.h"

//----------------------------------------------------------------------
// UpdatePC : Increments the Program Counter register in order to resume
// the user program immediately after the "syscall" instruction.
//----------------------------------------------------------------------
static void
UpdatePC ()
{
    int pc = machine->ReadRegister (PCReg);
    machine->WriteRegister (PrevPCReg, pc);
    pc = machine->ReadRegister (NextPCReg);
    machine->WriteRegister (PCReg, pc);
    pc += 4;
    machine->WriteRegister (NextPCReg, pc);
}

int copyStringFromMachine(int from, char *to, unsigned size)
{
    unsigned i=0;
    int result;
    while((i<size)&&(machine->ReadMem(from+i, 1, &result)))
    {
        *(to+i) = (char)result;
        i++;
    }
    *(to+i) = '\0';
    return i;
}

int copyStringToMachine(char* from, int to, unsigned size)
{
	unsigned i=0;
	while(i<size && from[i] != '\0')
	{
		machine->WriteMem(to+i, 1, from[i]);
		i++;
	}
	machine->WriteMem(to+i, 1, '\0');
    return i;
}


//----------------------------------------------------------------------
// ExceptionHandler
//      Entry point into the Nachos kernel.  Called when a user program
//      is executing, and either does a syscall, or generates an addressing
//      or arithmetic exception.
//
//      For system calls, the following is the calling convention:
//
//      system call code -- r2
//              arg1 -- r4
//              arg2 -- r5
//              arg3 -- r6
//              arg4 -- r7
//
//      The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//      "which" is the kind of exception.  The list of possible exceptions 
//      are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler (ExceptionType which)
{
    int type = machine->ReadRegister (2);
    int address = machine->registers[BadVAddrReg];

    switch (which)
      {
	case SyscallException:
          {
	    switch (type)
	      {
		case SC_Halt:
		  {
		    DEBUG ('s', "Shutdown, initiated by user program.\n");
		    interrupt->Halt ();
		    break;

		  }
		case SC_PutChar:
		  {
			int ch = machine->ReadRegister(4); //recuperation du registre 4 = arg1
			DEBUG('s',"PutChar\n");
			consoledriver->PutChar(ch);
			break;
		  }

		case SC_PutString:
		{
			char *to = new char[MAX_STRING_SIZE+1]; // +1 pour le \0
			int ch = machine->ReadRegister(4); //recuperation string
			int res = 0;
			while(res != ch)
			{
				res = res + copyStringFromMachine(ch+res, to, MAX_STRING_SIZE); //copie string mips -> linux
				DEBUG('s', "PutString\n");
				consoledriver->PutString(to);
			}
			delete [] to;
			break;
		}
		case SC_GetChar:
		{
			machine->WriteRegister(2, consoledriver->GetChar());
			int res = machine->ReadRegister(2);

			if(res!=-1)
				printf("%c\n", res);

			break;
		}
		case SC_GetString:
		{
			char *buff = new char[MAX_STRING_SIZE+1];
			int to = machine->ReadRegister(4);
			int n = machine->ReadRegister(5);
			consoledriver->GetString(buff, n);
			copyStringToMachine(buff, to, n);	
			delete [] buff;
			break;
		}
		case SC_Exit:
		{
			printf("Fin programme");
			interrupt->Halt (); //halt par defaut mtn
			break;
		}
		default:
		  {
		    printf("Unimplemented system call %d\n", type);
		    ASSERT(FALSE); //partie 6 assertion failed
			//pas d'appel system halt ducoup on va dans le case default du switch
		  }
	      }

	    // Do not forget to increment the pc before returning!
	    UpdatePC ();
	    break;
	  }

	case PageFaultException:
	  if (!address) {
	    printf("NULL dereference at PC %x!\n", machine->registers[PCReg]);
	    ASSERT (FALSE);
	  } else {
	    printf ("Page Fault at address %x at PC %x\n", address, machine->registers[PCReg]);
	    ASSERT (FALSE);	// For now
	  }
	  break;

	case ReadOnlyException:
	  printf ("Read-Only at address %x at PC %x\n", address, machine->registers[PCReg]);
	  ASSERT (FALSE);	// For now
	  break;

	case BusErrorException:
	  printf ("Invalid physical address at address %x at PC %x\n", address, machine->registers[PCReg]);
	  ASSERT (FALSE);	// For now
	  break;

	case AddressErrorException:
	  printf ("Invalid address %x at PC %x\n", address, machine->registers[PCReg]);
	  ASSERT (FALSE);	// For now
	  break;

	case OverflowException:
	  printf ("Overflow at PC %x\n", machine->registers[PCReg]);
	  ASSERT (FALSE);	// For now
	  break;

	case IllegalInstrException:
	  printf ("Illegal instruction at PC %x\n", machine->registers[PCReg]);
	  ASSERT (FALSE);	// For now
	  break;

	default:
	  printf ("Unexpected user mode exception %d %d %x at PC %x\n", which, type, address, machine->registers[PCReg]);
	  ASSERT (FALSE);
	  break;
      }
}
