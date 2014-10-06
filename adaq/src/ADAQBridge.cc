///////////////////////////////////////////////////////////////////////////////
//
// name: ADAQBridge.cc
// date: 06 Oct 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
//
// desc: 
//
///////////////////////////////////////////////////////////////////////////////

// C++
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>
#include <time.h>
using namespace std;

// Boost
#include <boost/assign/std/vector.hpp>
using namespace boost::assign;

// CAEN
extern "C" {
#include "CAENComm.h"
#include "CAENVMElib.h"
}

// ADAQ
#include "ADAQBridge.hh"


ADAQBridge::ADAQBridge(ZBoardType Type, int ID, uint32_t Address)
  : ADAQVBoard(Type, ID, Address)
{
  if(Type == zV1718){
    BoardName = "V1718";
    ConnectionName = "USB";
  }
}


ADAQBridge::~ADAQBridge()
{;}


int ADAQBridge::OpenLink()
{ return OpenLinkDirectly(); }


int ADAQBridge::OpenLinkDirectly()
{
  CommandStatus = -42;
  
  if(LinkEstablished){
    if(Verbose)
      cout << "ADAQBridge: Error opening direct USB link! Link is already open!\n"
	   << endl;
  }
  else
    CommandStatus = CAENVME_Init(cvV1718, 
				 0, 
				 0, 
				 &BoardHandle);
  
  if(CommandStatus == CAENComm_Success){

    LinkEstablished = true;

    if(Verbose)
      cout << "ADAQBridge[" << BoardID << "] : Direct USB link successfully established!\n"
	   << "--> Type    : " << BoardName << " (" << ConnectionName << ")\n"
	   << "--> Address : Not applicable; direct USB link\n"
	   << "--> User ID : " << BoardID << "\n"
	   << endl;
  }
  else
    if(Verbose and !LinkEstablished)
      cout << "ADAQBridge[" << BoardID << "] : Error opening link! Error code: " 
	   << CommandStatus << "\n"
	   << endl;

  return CommandStatus;
}


int ADAQBridge::OpenLinkViaDigitizer(uint32_t DigitizerHandle, 
				     bool DigitizerLinkEstablished)
{
  CommandStatus = -42;

  if(!DigitizerLinkEstablished){
    cout << "ADAQBridge[" << BoardID << "] : Error opening link! A link must be first established with the V1720 board!\n"
	 << endl;
    return CommandStatus;
  }

  // If the link is not currently valid then establish one!
  if(!LinkEstablished){
    
    // A link to the V1718 is technically established automatically
    // when a VME connections is established through a VME digitizer
    // board. Thus, access to the VME bridge board is _not_ provided
    // by a hex VME address nor the CAENComm_OpenDevice() function.
    // Rather, the handle to the V1718 must be retrieved in a slightly
    // round-about fashion via the CAENCOMM_Info function via a
    // digitizer handle rather than directly to the bridge board (as
    // shown above in the ADAQBridge::OpenLinkDirectory() method)
    
    CommandStatus = CAENComm_Info(DigitizerHandle,
				  CAENComm_VMELIB_handle, 
				  &BoardHandle);
  }
  else
    if(Verbose)
      cout << "ADAQBridge: Error opening link through V1720 digitizer! Link is already open!\n"
	   << endl;

  // Set the LinkEstablished bool to indicate that a valid link nto
  // the V1718 has been established and output if Verbose set
  if(CommandStatus == CAENComm_Success){

    LinkEstablished = true;

    if(Verbose)
      cout << "ADAQBridge[" << BoardID << "] : Link successfully established through V1720 digitizer!\n"
	   << "--> Type    : " << BoardName << " (" << ConnectionName << ")\n"
	   << "--> Address : Not applicable; set automatically via digitizer\n"
	   << "--> User ID : " << BoardID << "\n"
	   << endl;
  }
  else
    if(Verbose and !LinkEstablished)
      cout << "ADAQBridge[" << BoardID << "] : Error opening link! Error code: " 
	   << CommandStatus << "\n"
	   << endl;
  
  // Return success/failure 
  return CommandStatus;
}


int ADAQBridge::CloseLink()
{
  // There is not really a true VME link to the V1718 board when it is
  // placed in slot 0 (The VME controller slot), i.e. when it
  // functions as the VME/USB bridge. So we'll just pretend...
  CommandStatus = -42;

  if(LinkEstablished)
    CommandStatus = 0;
  else
    if(Verbose)
      cout << "ADAQBridge[" << BoardID << "] : Error closing link! Link is already closed!\n"
	   << endl;
  
  if(CommandStatus == CAENComm_Success){
    LinkEstablished = false;
    if(Verbose)
      cout << "ADAQBridge[" << BoardID << "] : Link successfully closed!\n"
	   << endl;
  }
  
  return CommandStatus;
}


int ADAQBridge::Initialize()
{;}


// Method to set a value to an individual register of the V1718. Note
// that V1718 register access is through CAENVME not CAENComm library
int ADAQBridge::SetRegisterValue(uint32_t addr32, uint32_t data32)
{
  CommandStatus = -42;

  // Ensure that each register proposed for writing is a register that
  // is valid for user writing to prevent screwing up V1718 firmware
  if(CheckRegisterForWriting(addr32))
    CommandStatus  = CAENVME_WriteRegister(BoardHandle, (CVRegisters)addr32, data32); 
  
  return CommandStatus;
}


// Method to get a value stored at an individual register of the V1718
// Note that V1718 register access is through CAENVME not CAENComm library
int ADAQBridge::GetRegisterValue(uint32_t addr32, uint32_t *data32)
{
  CommandStatus = CAENVME_ReadRegister(BoardHandle, (CVRegisters)addr32, data32);
  return CommandStatus;
}


// Method to check validity of V1718 register for writing
bool ADAQBridge::CheckRegisterForWriting(uint32_t addr32)
{ return true; }



int ADAQBridge::SetPulserSettings(PulserSettings *PS)
{
  CommandStatus = CAENVME_SetPulserConf(BoardHandle,
					(CVPulserSelect)PS->PulserToSet,
					PS->Period,
					PS->Width,
					(CVTimeUnits)PS->TimeUnit,
					PS->PulseNumber,
					(CVIOSources)PS->StartSource,
					(CVIOSources)PS->StopSource);
  return CommandStatus;
}


int ADAQBridge::SetPulserOutputSettings(PulserOutputSettings *POS)
{
  CommandStatus = CAENVME_SetOutputConf(BoardHandle,
					(CVOutputSelect)POS->OutputLine,
					(CVIOPolarity)POS->OutputPolarity,
					(CVLEDPolarity)POS->LEDPolarity,
					(CVIOSources)POS->Source);
  return CommandStatus;
}


// Method to start Pulser A or B
int ADAQBridge::StartPulser(uint32_t PulserToStart)
{
  CommandStatus = CAENVME_StartPulser(BoardHandle,
				      (CVPulserSelect)PulserToStart);
  return CommandStatus;
}


// Method to stop Pulser A or B
int ADAQBridge::StopPulser(uint32_t PulserToStop)
{
  CommandStatus = CAENVME_StopPulser(BoardHandle,
				     (CVPulserSelect)PulserToStop);
  return CommandStatus;
}
