// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

int allocate()
{
	int i = 0;
	while(i < NumPhysPages)
	{
		if( machine->mem_free_list[i] == 0 )
		{
			machine->mem_free_list[i] = 1;
			return i;
		}
		i++;
	}
}

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('t', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = allocate();
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    //bzero(machine->mainMemory, size);
	for(i=0; i<numPages; i++)
	{
		bzero( &(machine->mainMemory[pageTable[i].physicalPage * PageSize]), PageSize );
	}

// then, copy in the code and data segments into memory
int no_pages,page_start,size_offset,src_addr;
    if (noffH.code.size > 0) {
    DEBUG('t', "Initializing code segment, at %d, size %d, from %d\n", 
			noffH.code.virtualAddr, noffH.code.size,noffH.code.inFileAddr);
            
        page_start=noffH.code.virtualAddr / PageSize;
        size_offset=noffH.code.virtualAddr % PageSize;
        src_addr=noffH.code.inFileAddr;
        if( size_offset !=0 )
        {
        DEBUG('t', "Initializing code segment (-1), at %x, size %d, from %d\n",
        (pageTable[page_start].physicalPage * PageSize)+size_offset, PageSize-size_offset,src_addr);
        	executable->ReadAt( &(machine->mainMemory[ (pageTable[page_start].physicalPage * PageSize)+size_offset ]),
			PageSize-size_offset, src_addr );
		src_addr+=(PageSize-size_offset);
		page_start++;
        }
            
        no_pages = (noffH.code.size-size_offset) / PageSize;
        i=0;
        DEBUG('t',"Code : Virtual Addr = %d, Page_start = %d , Size_offset = %d\n",noffH.code.virtualAddr,page_start,size_offset);
        while( i < no_pages )
        {        	
        	   DEBUG('t', "Initializing code segment %d, at %d, size %d, from %d\n",
        	   i,pageTable[page_start+i].physicalPage * PageSize, PageSize,src_addr+(i * PageSize));
        	  executable->ReadAt( &(machine->mainMemory[pageTable[page_start+i].physicalPage * PageSize]),
			PageSize, src_addr+(i * PageSize) );
		i++;
        }        
        if( (noffH.code.size-size_offset) % PageSize != 0)
        {
        	DEBUG('t', "Initializing code segment %d, at %d, size %d, from %d\n",i,
        	pageTable[page_start+i].physicalPage * PageSize, src_addr+(i * PageSize));
        	   executable->ReadAt( &(machine->mainMemory[pageTable[page_start+i].physicalPage * PageSize]),
			(noffH.code.size-size_offset) % PageSize, src_addr+(i * PageSize) );        
    	}
    }    
        
    if (noffH.initData.size > 0) {  
        DEBUG('t', "Initializing data segment, at %d, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
	page_start=noffH.initData.virtualAddr / PageSize;
        size_offset=noffH.initData.virtualAddr % PageSize;
        src_addr=noffH.initData.inFileAddr;
        if( size_offset !=0 )
        {
        	DEBUG('t', "Initializing data segment (-1), at %d, size %d\n",(pageTable[page_start].physicalPage * PageSize)+size_offset, PageSize-size_offset);
        	executable->ReadAt( &(machine->mainMemory[ (pageTable[page_start].physicalPage * PageSize)+size_offset ]),
			PageSize-size_offset, src_addr );
		src_addr+=(PageSize-size_offset);
		page_start++;
        }
            
        no_pages = (noffH.initData.size-size_offset) / PageSize;
        i=0;
        DEBUG('t',"DATA : Virtual Addr = %d, Page_start = %d , Size_offset = %d\n",noffH.initData.virtualAddr,page_start,size_offset);
        while( i < no_pages )
        {
        	DEBUG('t', "Initializing data segment %d, at 0x%d, size %d\n",i,pageTable[page_start+i].physicalPage * PageSize, PageSize);
        	  executable->ReadAt( &(machine->mainMemory[pageTable[page_start+i].physicalPage * PageSize]),
			PageSize, src_addr+(i * PageSize) );
		i++;
        }        
        if( (noffH.initData.size-size_offset) % PageSize != 0)
        {
        	DEBUG('t', "Initializing data segment %d, at 0x%d, size %d\n",i,
        	pageTable[page_start+i].physicalPage * PageSize, (noffH.initData.size-size_offset) % PageSize);
        	   executable->ReadAt( &(machine->mainMemory[pageTable[page_start+i].physicalPage * PageSize]),
			(noffH.initData.size-size_offset) % PageSize, src_addr+(i * PageSize) );	
    	}
    }

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
	int i;
	for(i=0; i<numPages; i++)
		machine->mem_free_list[pageTable[i].physicalPage] = 0;
   	delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('t', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
	pageTable = machine->pageTable;
  	numPages = machine->pageTableSize;
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
