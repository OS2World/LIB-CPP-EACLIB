/* --------------------------------------------------------------------------
 * $RCSfile: EA.cpp,v $
 * $Revision: 1.1 $
 * $Date: 1996/05/22 20:59:55 $
 * $Author: Bablok $
 * --------------------------------------------------------------------------
 * Synopsis:
 *
 * Implementation of class EA. This class wraps the extended attributes API of
 * OS/2.
 *
 * This file is part of the EA classlib package.
 * Copyright Bernhard Bablok, 1996
 *
 * The EA classlib package is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You may use the classes in the package to any extend you wish. You are
 * allowed to change and copy the source of the classes, as long as you keep
 * the copyright notice intact and as long as you document the changes you made.
 *
 * You are not allowed to sell the EA classlib package or a modified version
 * thereof, but you may charge for costs of distribution media.
 *
 * --------------------------------------------------------------------------
 * Change-Log:
 *
 * $Log: EA.cpp,v $
 * Revision 1.1  1996/05/22 20:59:55  Bablok
 * Initial revision
 *
 * -------------------------------------------------------------------------- */

#ifndef _IEXCEPT_
   #include <iexcept.hpp>
#endif
#ifndef _IMSGTEXT_
   #include <imsgtext.hpp>
#endif

#ifndef EA_H
   #include "EA.hpp"
#endif
#ifndef EALIST_H
   #include "EAList.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////
//  Construct a multivalued EA from an EAList
//
EA::EA(const IString& name, const EAList& eaList, BYTE flag) : mName(name),
                                                                   mFlag(flag) {

   if (!eaList.numberOfElements()) {
      IInvalidRequest exc(IMessageText(ERR_EALIST_EMPTY,MSG_FILE),
                                                     0,IException::recoverable);
      ITHROW(exc);
   }
   mName.upperCase();

   // Calculate size of required buffer for mValue and check type   ------------

   EAList::Cursor current(eaList);
   Boolean isSingleTyped = true;

   current.setToFirst();
   USHORT  type   = current.element().mType,
           length = 0;

   forCursor(current) {
      const EA& ea = current.element();
      isSingleTyped = type == ea.mType;
      if (ea.isLengthPreceded())
         length += sizeof(USHORT);
      length += ea.mValue.length();
   }

   // Query code page information   --------------------------------------------

   ULONG codePage, codePageLength;
   APIRET rc = DosQueryCp(sizeof(codePage),&codePage,&codePageLength);

   if (rc == ERROR_CP_NOT_MOVED) {
      IException exc(ISystemErrorInfo(rc,"DosQueryCp"),
                                                    rc,IException::recoverable);
      ITHROW(exc);
   }

   // Allocate and fill buffer for mValue   ------------------------------------

   char *buffer, *p;
   if (isSingleTyped)
      length += 3*sizeof(USHORT);
   else
      length += 2*sizeof(USHORT) + eaList.numberOfElements()*sizeof(USHORT);
   buffer = new char[length];
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }
   p = buffer;
   *(USHORT*) p = (USHORT) codePage;
   p += sizeof(USHORT);
   *(USHORT*) p = (USHORT) eaList.numberOfElements();
   p += sizeof(USHORT);
   if (isSingleTyped) {
      *(USHORT*) p = eaList.anyElement().mType;
      p += sizeof(USHORT);
   }

   forCursor(current) {
      const EA& ea = current.element();
      if (!isSingleTyped) {
         *(USHORT*) p = ea.mType;
         p += sizeof(USHORT);
      }
      if (ea.isLengthPreceded()) {
         *(USHORT*) p = ea.mValue.length();
         p += sizeof(USHORT);
      }
      memcpy(p,(char*) ea.mValue,ea.mValue.length());
      p += ea.mValue.length();
   }

   // set mValue and clean up resources   --------------------------------------

   mValue = IString(buffer,length);
   if (isSingleTyped)
      mType  = EAT_MVST;
   else
      mType  = EAT_MVMT;
   delete buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Construct an EA from a FEA2-structure
//
EA::EA(FEA2* fea2) {
   mName = fea2->szName;
   mFlag = fea2->fEA;
   if (fea2->cbValue) {
      char* val = (char*) fea2 + sizeof(FEA2) + fea2->cbName;
      mType  = *(USHORT*) val;                        // first word is type
      if (isLengthPreceded())                         // second word is length
         mValue = IString(val+2*sizeof(USHORT),fea2->cbValue-2*sizeof(USHORT));
      else
         mValue = IString(val+sizeof(USHORT),fea2->cbValue - sizeof(USHORT));
   } else {
      mValue = "";
      mType  = EAT_ASCII;
   }
}


///////////////////////////////////////////////////////////////////////////////
//  Read an EA from a given file. If the given EA does not exist, the value
//  is set to "".
//
EA& EA::read(PVOID fileRef, Boolean isPathName) {

   if (mName == "") {
      IInvalidRequest exc(IMessageText(ERR_NO_EA_NAME,MSG_FILE),
                                                     0,IException::recoverable);
      ITHROW(exc);
   }

   // set up EA list and query EA   --------------------------------------------

   EAOP2 eaBuffer;
   eaBuffer.fpGEA2List = createGEA2LIST();
   eaBuffer.fpFEA2List = createFEA2LISTBuffer(fileRef,isPathName);
   eaBuffer.oError     = 0;
   APIRET rc;
   if (isPathName)
      rc = DosQueryPathInfo((PSZ)fileRef,FIL_QUERYEASFROMLIST,&eaBuffer,
                                                                 sizeof(EAOP2));
   else
      rc = DosQueryFileInfo(*(HFILE*)fileRef,FIL_QUERYEASFROMLIST,&eaBuffer,
                                                                 sizeof(EAOP2));
   if (rc) {
      delete eaBuffer.fpGEA2List;
      delete eaBuffer.fpFEA2List;
      IString api;
      if (isPathName)
         api = "DosQueryPathInfo";
      else
         api = "DosQueryFileInfo";
      IException exc(ISystemErrorInfo(rc,api),rc,IException::recoverable);
      ITHROW(exc);
   }

   // convert result to EA   ---------------------------------------------------

   *this = EA(&eaBuffer.fpFEA2List->list[0]);
   delete eaBuffer.fpGEA2List;
   delete eaBuffer.fpFEA2List;
   return *this;
}


///////////////////////////////////////////////////////////////////////////////
//  Write an EA to the given file.
//
EA& EA::write(PVOID fileRef, Boolean isPathName) {

   if (mName == "") {
      IInvalidRequest exc(IMessageText(ERR_NO_EA_NAME,MSG_FILE),
                                                     0,IException::recoverable);
      ITHROW(exc);
   }

   EAOP2 eaBuffer;
   eaBuffer.fpGEA2List = 0;
   eaBuffer.fpFEA2List = createFEA2LIST();
   eaBuffer.oError     = 0;
   APIRET rc;
   if (isPathName)
      rc = DosSetPathInfo((PSZ)fileRef,FIL_QUERYEASIZE,&eaBuffer,sizeof(EAOP2),
                                                                  DSPI_WRTTHRU);
   else
      rc = DosSetFileInfo(*(HFILE*)fileRef,FIL_QUERYEASIZE,
                                                       &eaBuffer,sizeof(EAOP2));
   delete eaBuffer.fpFEA2List;
   if (rc) {
      IString api;
      if (isPathName)
         api = "DosSetPathInfo";
      else
         api = "DosSetFileInfo";
      IException exc(ISystemErrorInfo(rc,api),rc,IException::recoverable);
      ITHROW(exc);
   }
   return *this;
}


///////////////////////////////////////////////////////////////////////////////
//  Remove an EA from the given file.
//
EA& EA::remove(PVOID fileRef, Boolean isPathName) {
   EA rem(*this);
   rem.mValue = "";
   rem.write(fileRef,isPathName);
   return *this;
}


///////////////////////////////////////////////////////////////////////////////
//  Determine if data-area of EA is preceded by length (static version)
//
Boolean EA::isLengthPreceded(USHORT type) {
   switch (type) {
      case EAT_BINARY:
      case EAT_ASCII:
      case EAT_BITMAP:
      case EAT_METAFILE:
      case EAT_ICON:
      case EAT_EA:
         return true;
      default:
         return false;
   }
}


///////////////////////////////////////////////////////////////////////////////
//  Convert mName to GEA2LIST-structure
//
GEA2LIST* EA::createGEA2LIST() const {
   size_t length = sizeof(GEA2LIST) + mName.length();
   char *buffer = new char[length];
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }
   char *p = buffer;

   memset(p,0,length);
   *(ULONG*) p = length;                             // cbList
   p += 2*sizeof(ULONG);
   *(BYTE*) p = mName.length();                      // cbName
   p += sizeof(BYTE);
   memcpy(p,(char*) mName,mName.length());           // mName
   return (GEA2LIST*) buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Convert mName,mValue,mType to FEA2LIST-structure
//
FEA2LIST* EA::createFEA2LIST() const {

   FEA2LIST *buffer = createFEA2LISTBuffer();
   char *p = (char*) buffer;
   p += sizeof(ULONG);                               // skip cbList
   createFEA2(p);
   return buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Convert mName,mValue,mType to FEA2-structure
//
ULONG EA::createFEA2(char *buffer) const {

   char *p = buffer + sizeof(ULONG);            // skip oNextEntryOffset

   *(BYTE*) p = mFlag;                          // fEA
   p += sizeof(BYTE);
   *(BYTE*) p = mName.length();                 // cbName
   p += sizeof(BYTE);
   *(USHORT*) p = mValue.length();              // cbValue
   USHORT *cbValuePtr = (USHORT*) p;
   p += sizeof(USHORT);
   memcpy(p,(char*)mName,mName.length());       // szName
   p += mName.length() + sizeof(char);
   if (mValue.length()) {
      *(USHORT*) p = mType;                     // EA data type
      *cbValuePtr += sizeof(USHORT);            // increase cbValue
      p += sizeof(USHORT);
      if (isLengthPreceded()) {
         *(USHORT*) p = mValue.length();        // length of data
         *cbValuePtr += sizeof(USHORT);         // increase cbValue
         p += sizeof(USHORT);
      }
      memcpy(p,(char*) mValue,mValue.length());
      p += mValue.length();
   }

   return p - buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Allocate FEA2LIST data area for get operation
//
FEA2LIST* EA::createFEA2LISTBuffer(PVOID fileRef, Boolean isPathName) const {

   // query EA size   ----------------------------------------------------------

   FILESTATUS4 status;
   APIRET rc;
   if (isPathName)
      rc = DosQueryPathInfo((PSZ)fileRef,FIL_QUERYEASIZE,&status,
                                                           sizeof(FILESTATUS4));
   else
      rc = DosQueryFileInfo(*(HFILE*)fileRef,FIL_QUERYEASIZE,&status,
                                                           sizeof(FILESTATUS4));
   if (rc) {
      IString api;
      if (isPathName)
         api = "DosQueryPathInfo";
      else
         api = "DosQueryFileInfo";
      IException exc(ISystemErrorInfo(rc,api),rc,IException::recoverable);
      ITHROW(exc);
   }

   unsigned long bufferSize = sizeof(ULONG)+2*status.cbList+mName.length()+1;
   char *buffer = new char[bufferSize];                      // crude, but quick
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }
   memset(buffer,0,bufferSize);
   *(ULONG*) buffer = bufferSize;
   return (FEA2LIST*) buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Allocate FEA2LIST data area for put operation
//
FEA2LIST* EA::createFEA2LISTBuffer() const {

   size_t length = sizeof(FEA2LIST) +
                   mName.length()   +                // length of szName
                   mValue.length();                  // length of value
   if (mValue.length()) {
      length += sizeof(USHORT);                      // first word is type
      if (isLengthPreceded())                        // second word is length
         length += sizeof(USHORT);
   }
   char* buffer = new char[length];
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }

   memset(buffer,0,length);
   *(ULONG*) buffer = length;
   return (FEA2LIST*) buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Return type of EA as string
//
IString EA::typeAsString(USHORT type) {
   switch (type) {
      case EAT_BINARY:
         return "Binary";
      case EAT_ASCII:
         return "ASCII";
      case EAT_BITMAP:
         return "Bitmap";
      case EAT_METAFILE:
         return "Metafile";
      case EAT_ICON:
         return "Icon";
      case EAT_EA:
         return "EA";
      case EAT_MVMT:
         return "MVMT";
      case EAT_MVST:
         return "MVST";
      case EAT_ASN1:
         return "ASN.1";
      default:
         return IString("Unknown(") + IString::c2x(type) + IString(')');
   }
}


///////////////////////////////////////////////////////////////////////////////
//  Return number of values in a multi-valued EA
//
USHORT EA::numValues() const {

   if (mType != EAT_MVMT && mType != EAT_MVST) {
      IInvalidRequest exc(IMessageText(ERR_NOT_MULTI_VALUED,MSG_FILE),
                                                     0,IException::recoverable);
      ITHROW(exc);
   }
   return *(USHORT*) ((char*) mValue+sizeof(USHORT));
}


///////////////////////////////////////////////////////////////////////////////
//  Return code page from a multi-valued EA
//
USHORT EA::codePage() const {

   if (mType != EAT_MVMT && mType != EAT_MVST) {
      IInvalidRequest exc(IMessageText(ERR_NOT_MULTI_VALUED,MSG_FILE),
                                                     0,IException::recoverable);
      ITHROW(exc);
   }
   return *(USHORT*) ((char*) mValue);
}
