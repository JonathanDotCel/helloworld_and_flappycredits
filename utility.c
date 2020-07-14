

#include "utility.h"



// Get int enable state from cop0r12
int InCriticalSection(){
	
	
	ulong returnVal;
	__asm__ volatile(
		"mfc0 %0,$12\n\t"
		"nop\n\t"
		: "=r"( returnVal )
		: // no inputs		
	);

	return !(returnVal & 0x01);
	
	
}


// Enter a critical section by disabling interrupts
int EnterCritical(){
	
	
	ulong oldVal = InCriticalSection();

	__asm__ volatile (  
		"li   $9, 0x1\n\t"		// li t1,0x01
		"not  $9\n\t"			// not t1
		"mfc0 $8, $12\n\t"		// mfc0 t0,$12
		"nop  \n\t"				// best opcode		
		"and  $8,$8,$9\n\t"		// t0 = t0 & t1 (mask it)
		"mtc0 $8, $12\n\t"		// send it back
		"nop"
		: // no outputs
		: // no inputs
		: "$8", "$9"
	);

	return oldVal;
	

}

// Exit critical by re-enabling interrupts
int ExitCritical(){
	
	
	__asm__ volatile (		
		"mfc0 $8, $12\n\t"
		"nop  \n\t"
		"ori  $8,$8,0x01\n\t"
		"mtc0 $8, $12\n\t"
		"nop"
		: //
		: //
		: "$8"
	);
	
}


unsigned long ResetEntryInt(){

	register int cmd __asm__("$9") = 0x18;
	__asm__ volatile("" : "=r"(cmd) : "r"(cmd));
	return ((int(*)(void))0xB0)();

}



// TODO:
void ResetGraph(){}
void InitHeap (unsigned long * a, unsigned long b){}
int StopCallback(void){}



#pragma GCC push options
#pragma GCC optimize ("-O0")
void Delay( int inLen ){

	// __TEST__
	int i = 0;
	for( i = 0; i < inLen; i++ ){		
		__asm__ volatile( "" : "=r"( i ) : "r"( i ) );
	}
	

}
#pragma GCC pop options



// E, J, or U
int IsPAL(){
	
	return ( *(char*)0xBFC7FF52 == 'E' );
	
}

void UnloadMe(){
	
	// Reset interrupt + mask
	*(ulong*)0xBF801070 = 0;
	*(ulong*)0xBF801074 = 0;
	
	PadStop();
	ResetGraph( 3 );
	StopCallback();

}


