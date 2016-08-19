/* --------------------------------------------------------------------------
 * $RCSfile: EAList.cpp,v $
 * $Revision: 1.1 $
 * $Date: 1996/05/22 20:59:55 $
 * $Author: Bablok $
 * --------------------------------------------------------------------------
 * Synopsis:
 *
 * Implementation of class EAList. This class wraps the extended attributes API
 * of OS/2 and handles complete sets of file-EAs.
 *
 * An EAList is implemented as a key sorted set. This allows the conversion
 * of an EAList to a multivalued EA and back again.
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
 * $Log: EAList.cpp,v $
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
#ifndef __iostream_h
   #include <iostream.h>
#endif
#ifndef __math_h
   #include <math.h>
#endif

#ifndef EA_H
   #include "EA.hpp"
#endif
#ifndef EALIST_H
   #include "EAList.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////
// Copy constructor
//
EAList::EAList(const EAList& eaList) : EASet(EALIST_DEFAULT_SIZE),
                                                               mFEA2List(NULL) {
   if (eaList.mFEA2List) {
      ULONG length = *(ULONG*) eaList.mFEA2List;
      char *buffer = new char[length];
      if (!buffer) {
         IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
         ITHROW(exc);
      }
      memcpy(buffer,(char*) eaList.mFEA2List,length);
      mFEA2List = (FEA2LIST*) buffer;
   }
   addAllFrom(eaList);
}


///////////////////////////////////////////////////////////////////////////////
// Construct EAList from multi-valued EA
//
EAList::EAList(const IString& basename, const EA& ea) :
                                   EASet(EALIST_DEFAULT_SIZE), mFEA2List(NULL) {

   USHORT type,
          n      = ea.numValues(),     // this will throw an exception if not mv
          digits = log10(n) + 1;
   BYTE   flag   = ea.flag();
   char   *p     = (char*) ea.value() + 2*sizeof(USHORT);   // skip header

   // extract type if single-typed   -------------------------------------------

   if (ea.type() == EAT_MVST) {
      type = *(USHORT*) p;
      p += sizeof(USHORT);
      if (!EA::isLengthPreceded(type)) {
         IString excText(IMessageText(ERR_INVALID_TYPE,MSG_FILE));
         excText += " (" + EA::typeAsString(type) + ")";
         IInvalidRequest exc(excText,0,IException::recoverable);
         ITHROW(exc);
      }
   }

   // iterate through all entries   --------------------------------------------

   for (int i=1; i<=n; ++i) {
      if (ea.type() == EAT_MVMT) {
         type = *(USHORT*) p;
         p += sizeof(USHORT);
         if (!EA::isLengthPreceded(type)) {
            IString excText(IMessageText(ERR_INVALID_TYPE,MSG_FILE));
            excText += " (" + EA::typeAsString(type) + ")";
            IInvalidRequest exc(excText,0,IException::recoverable);
            ITHROW(exc);
         }
      }
      USHORT length = *(USHORT*) p;
      p += sizeof(USHORT);
      IString name = basename + IString('.') + IString(i).rightJustify(digits,0);
      add(EA(name,p,length,type,flag));
      p += length;
   }
}


///////////////////////////////////////////////////////////////////////////////
// Assignment
//
EAList& EAList::operator=(const EAList& eaList) {
   if (&eaList == this)
      return *this;

   if (mFEA2List)
      delete mFEA2List;
   if (eaList.mFEA2List) {
      ULONG length = *(ULONG*) eaList.mFEA2List;
      char *buffer = new char[length];
      if (!buffer) {
         IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
         ITHROW(exc);
      }
      memcpy(buffer,(char*) eaList.mFEA2List,length);
      mFEA2List = (FEA2LIST*) buffer;
   }  else
      mFEA2List = NULL;

   removeAll();
   addAllFrom(eaList);
   return *this;
}


///////////////////////////////////////////////////////////////////////////////
//  Read EAs from a given file. This function either reads all EAs of the
//  file, or only the EAs given in the list.
//
EAList& EAList::read(PVOID fileRef,Boolean isPathName,Boolean onlyEAsFromList) {

   EAOP2 eaBuffer;
   eaBuffer.oError = 0;

   if (onlyEAsFromList) {
      if (!numberOfElements())                                 // nothing to do!
         return *this;
      eaBuffer.fpGEA2List = createGEA2LIST();
      eaBuffer.fpFEA2List = createFEA2LISTBuffer(fileRef,isPathName);
   } else {
      DENA2 *eaEnumBuffer = queryDENA2(fileRef,isPathName);
      if (!eaEnumBuffer) {
         removeAll();
         return *this;
      }
      eaBuffer.fpGEA2List = createGEA2LIST(eaEnumBuffer);
      eaBuffer.fpFEA2List = createFEA2LISTBuffer(eaEnumBuffer);
      delete eaEnumBuffer;
   }

   APIRET rc;
   if (isPathName)
      rc = DosQueryPathInfo((PSZ)fileRef,FIL_QUERYEASFROMLIST,
                                                       &eaBuffer,sizeof(EAOP2));
   else
      rc = DosQueryFileInfo(*(HFILE*)fileRef,FIL_QUERYEASFROMLIST,
                                                       &eaBuffer,sizeof(EAOP2));
   delete eaBuffer.fpGEA2List;
   if (rc) {
      IString api;
      if (isPathName)
         api = "DosQueryPathInfo";
      else
         api = "DosQueryFileInfo";
      IException exc(ISystemErrorInfo(rc,api),rc,IException::recoverable);
      ITHROW(exc);
   }
   if (!onlyEAsFromList)
      removeAll();                         // this is save now
   return convert();                       // converts FEA2LIST to EAList
}


///////////////////////////////////////////////////////////////////////////////
//  Write EAs to a given file.
//
EAList& EAList::write(PVOID fileRef,Boolean isPathName,Boolean useFEA2List) {

   if (!numberOfElements())        // nothing to do!
      return *this;

   EAOP2 eaBuffer;
   eaBuffer.oError     = 0;
   eaBuffer.fpGEA2List = 0;

   if (useFEA2List)
      eaBuffer.fpFEA2List = mFEA2List;
   else
      eaBuffer.fpFEA2List = createFEA2LIST();

   APIRET rc;
   if (isPathName)
      rc = DosSetPathInfo((PSZ)fileRef,FIL_QUERYEASIZE,&eaBuffer,sizeof(EAOP2),
                                                                  DSPI_WRTTHRU);
   else
      rc = DosSetFileInfo(*(HFILE*)fileRef,FIL_QUERYEASIZE,
                                                       &eaBuffer,sizeof(EAOP2));
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
//  Remove EAs from a given file. This function either removes all EAs of the
//  file, or only the EAs given in the list.
//
EAList& EAList::remove(PVOID fileRef,Boolean isPathName,Boolean onlyEAsFromList) {

   EAList removeList;

   if (onlyEAsFromList)
      removeList = *this;
   else
      removeList.read(fileRef,isPathName,false);

   EAList::Cursor current(removeList);
   forCursor(current)
      removeList.elementAt(current).setValue("");

   removeList.write(fileRef,isPathName,false);
   return *this;
}


///////////////////////////////////////////////////////////////////////////////
//  Enumerate EAs. This function returns a pointer to a DENA2-structure.
//
DENA2* EAList::queryDENA2(PVOID fileRef, Boolean isPathName) const {

   // query EA size   ----------------------------------------------------------

   FILESTATUS4 filestatus4;
   APIRET rc;
   if (isPathName)
      rc = DosQueryPathInfo((PSZ)fileRef,FIL_QUERYEASIZE,&filestatus4,
                                                           sizeof(FILESTATUS4));
   else
      rc = DosQueryFileInfo(*(HFILE*)fileRef,FIL_QUERYEASIZE,&filestatus4,
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

   // Allocate necessary buffer   ----------------------------------------------

   char *buffer = new char[2*filestatus4.cbList];
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }

   // Query DENA2-structure   --------------------------------------------------

   ULONG count = -1;                                            // query all EAs
   if (isPathName)
      rc = DosEnumAttribute(ENUMEA_REFTYPE_PATH,fileRef,1,buffer,
                             2*filestatus4.cbList,&count,ENUMEA_LEVEL_NO_VALUE);
   else
      rc = DosEnumAttribute(ENUMEA_REFTYPE_FHANDLE,fileRef,1,buffer,
                             2*filestatus4.cbList,&count,ENUMEA_LEVEL_NO_VALUE);
   if (rc) {
      IException exc(ISystemErrorInfo(rc,"DosEnumAttribute"),
                                                    rc,IException::recoverable);
      ITHROW(exc);
   }

   if (!count) {
      delete buffer;
      return NULL;
   } else
      return (DENA2*) buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Create GEA2LIST-structure from DENA2. This structure contains all
//  necessary information to query EAs.
//
GEA2LIST* EAList::createGEA2LIST(DENA2* pDENA2) const {

   IASSERT(pDENA2 != 0);

   // Calculate size of buffer   -----------------------------------------------

   ULONG length = sizeof(ULONG);                    // cbList
   DENA2* p = pDENA2;
   while (1) {
      length += sizeof(GEA2) + p->cbName;
      length += 4-(length&3) & 3;                   // align on double word
      if (p->oNextEntryOffset)
         p = (DENA2*) ((char*) p + p->oNextEntryOffset);
      else
         break;
   }

   // Allocate buffer   --------------------------------------------------------

   char *buffer = new char[length];
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }

   // fill buffer   ------------------------------------------------------------

   memset(buffer,0,length);
   *(ULONG*) buffer = length;                 // cbList
   GEA2* pGEA2 = (GEA2*) (buffer+sizeof(ULONG));
   p = pDENA2;
   while (1) {
      pGEA2->cbName = p->cbName;
      memcpy(pGEA2->szName,p->szName,p->cbName);
      if (p->oNextEntryOffset) {
         length = sizeof(GEA2) + p->cbName;
         pGEA2->oNextEntryOffset = length + (4-(length&3) & 3);
         pGEA2 = (GEA2*) ((char*) pGEA2 + pGEA2->oNextEntryOffset);
         p     = (DENA2*) ((char*) p + p->oNextEntryOffset);
      } else
         break;
   }
   return (GEA2LIST*) buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Create FEA2LIST-buffer from DENA2-structure.
//
FEA2LIST* EAList::createFEA2LISTBuffer(DENA2* pDENA2) {

   IASSERT(pDENA2 != 0);

   // Calculate size of buffer   -----------------------------------------------

   ULONG length = sizeof(ULONG);                    // cbList
   while (1) {
      length += sizeof(FEA2) + pDENA2->cbName + pDENA2->cbValue;
      length += 4-(length&3) & 3;                   // align on double word
      if (pDENA2->oNextEntryOffset)
         pDENA2 = (DENA2*) ((char*) pDENA2 + pDENA2->oNextEntryOffset);
      else
         break;
   }

   // Allocate buffer   --------------------------------------------------------

   char *buffer = new char[length];
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }
   memset(buffer,0,length);
   *(ULONG*) buffer = length;
   if (mFEA2List)
      delete mFEA2List;
   mFEA2List = (FEA2LIST*) buffer;
   return (FEA2LIST*) buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Create GEA2LIST-structure from EAList. This structure contains all
//  necessary information to query EAs.
//
GEA2LIST* EAList::createGEA2LIST() const {

   IASSERT(numberOfElements());

   // Calculate size of buffer   -----------------------------------------------

   ULONG length = sizeof(ULONG);                    // cbList
   EAList::Cursor current(*this);
   forCursor(current) {
      if (current.element().name() == "") {
         IInvalidRequest exc(IMessageText(ERR_NO_EA_NAME,MSG_FILE),
                                                     0,IException::recoverable);
         ITHROW(exc);
      }
      length += sizeof(GEA2) + current.element().name().length();
      length += 4-(length&3) & 3;                   // align on double word
   }

   // Allocate buffer   --------------------------------------------------------

   char *buffer = new char[length];
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }

   // fill buffer   ------------------------------------------------------------

   memset(buffer,0,length);
   *(ULONG*) buffer = length;                 // cbList
   GEA2* pGEA2 = (GEA2*) (buffer+sizeof(ULONG));
   current.setToFirst();
   while (1) {
      pGEA2->cbName = current.element().name().length();
      memcpy(pGEA2->szName,(const char*)current.element().name(),pGEA2->cbName);
      current.setToNext();
      if (current.isValid()) {
         length = sizeof(GEA2) + pGEA2->cbName;
         pGEA2->oNextEntryOffset = length + (4-(length&3) & 3);
         pGEA2 = (GEA2*) ((char*) pGEA2 + pGEA2->oNextEntryOffset);
      } else
         break;
   }
   return (GEA2LIST*) buffer;
}


///////////////////////////////////////////////////////////////////////////////
//  Create FEA2LIST-buffer large enough for all EAs.
//
FEA2LIST* EAList::createFEA2LISTBuffer(PVOID fileRef,Boolean isPathName) {

   if (mFEA2List)
      delete mFEA2List;
   mFEA2List = anyElement().createFEA2LISTBuffer(fileRef,isPathName);
   return mFEA2List;
}


///////////////////////////////////////////////////////////////////////////////
//  Create FEA2LIST-buffer from given EAList.
//
FEA2LIST* EAList::createFEA2LISTBuffer() {

   IASSERT(numberOfElements());

   // Calculate size of buffer   -----------------------------------------------

   ULONG length = sizeof(ULONG);                     // cbList
   EAList::Cursor current(*this);
   forCursor(current) {
      const EA& ea = current.element();
      if (ea.name() == "") {
         IInvalidRequest exc(IMessageText(ERR_NO_EA_NAME,MSG_FILE),
                                                     0,IException::recoverable);
         ITHROW(exc);
      }
      length += sizeof(FEA2) + ea.name().length() + ea.value().length();
      if (ea.value().length()) {
         length += sizeof(USHORT);                   // first word is type
         if (ea.isLengthPreceded())                  // second word is length
            length += sizeof(USHORT);
      }
      length += 4-(length&3) & 3;                   // align on double word
   }

   // Allocate buffer   --------------------------------------------------------

   char *buffer = new char[length];
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }
   memset(buffer,0,length);
   *(ULONG*) buffer = length;
   if (mFEA2List)
      delete mFEA2List;
   mFEA2List = (FEA2LIST*) buffer;
   return mFEA2List;
}


///////////////////////////////////////////////////////////////////////////////
//  Convert FEA2LIST-structure to EAList
//
EAList& EAList::convert() {

    FEA2 *p = &(mFEA2List->list[0]);
    while (1) {
       EA ea(p);
       if (ea.value() != "")
          addOrReplaceElementWithKey(ea);
       else
          removeElementWithKey(ea.name());
       if (p->oNextEntryOffset)
          p = (FEA2*) ((char*) p + p->oNextEntryOffset);
       else
          break;
    }
    return *this;
}


///////////////////////////////////////////////////////////////////////////////
//  Create FEA2LIST from given EAList.
//
FEA2LIST* EAList::createFEA2LIST() {

   createFEA2LISTBuffer();
   FEA2* p = (FEA2*) ((char*)mFEA2List + sizeof(ULONG));

   EAList::Cursor current(*this);
   current.setToFirst();
   while (1) {
      size_t length = current.element().createFEA2((char*)p);
      current.setToNext();
      if (current.isValid()) {
         p->oNextEntryOffset = length + (4-(length&3) & 3);
         p = (FEA2*) ((char*) p + p->oNextEntryOffset);
      } else
         break;
   }
   return mFEA2List;
}


///////////////////////////////////////////////////////////////////////////////
// Set values of an EAList from a multi-valued EA
//
EAList& EAList::setValues(const EA& ea) {

   USHORT n;
   if ((n = ea.numValues()) != numberOfElements()) {
      IInvalidRequest exc(IMessageText(ERR_ELEMENT_COUNT,MSG_FILE),
                                                     0,IException::recoverable);
      ITHROW(exc);
   }

   USHORT type, digits = log10(n) + 1;
   BYTE   flag   = ea.flag();
   char   *p     = (char*) ea.value() + 2*sizeof(USHORT);   // skip header

   // extract type if single-typed   -------------------------------------------

   if (ea.type() == EAT_MVST) {
      type = *(USHORT*) p;
      p += sizeof(USHORT);
      if (!EA::isLengthPreceded(type)) {
         IString excText(IMessageText(ERR_INVALID_TYPE,MSG_FILE));
         excText += " (" + EA::typeAsString(type) + ")";
         IInvalidRequest exc(excText,0,IException::recoverable);
         ITHROW(exc);
      }
   }

   // iterate through all entries   --------------------------------------------

   EAList::Cursor current(*this);
   forCursor(current) {
      if (ea.type() == EAT_MVMT) {
         type = *(USHORT*) p;
         p += sizeof(USHORT);
         if (!EA::isLengthPreceded(type)) {
            IString excText(IMessageText(ERR_INVALID_TYPE,MSG_FILE));
            excText += " (" + EA::typeAsString(type) + ")";
            IInvalidRequest exc(excText,0,IException::recoverable);
            ITHROW(exc);
         }
      }
      USHORT length = *(USHORT*) p;
      p += sizeof(USHORT);
      elementAt(current).setValue(IString(p,length));
      elementAt(current).setType(type);
      elementAt(current).setFlag(flag);
      p += length;
   }
   return *this;
}


///////////////////////////////////////////////////////////////////////////////
// Write mFEA2List to ostream (ostream must be open in binary mode)
//
ostream& operator<<(ostream& out, EAList& eaList) {
   if (eaList.mFEA2List == NULL)
      eaList.createFEA2LIST();
   out.write((char*) eaList.mFEA2List,*(ULONG*) eaList.mFEA2List);
   return out;
}


///////////////////////////////////////////////////////////////////////////////
// Read mFEA2List from istream (istream must be open in binary mode) and
// convert to EAList
//
istream& operator>>(istream& in, EAList& eaList) {
   ULONG length;
   in.read((char*) &length,sizeof(ULONG));
   char *buffer = new char[length];
   if (!buffer) {
      IOutOfMemory exc(IMessageText(ERR_MEM_ALLOC_FAILED,MSG_FILE),0);
      ITHROW(exc);
   }
   *(ULONG*) buffer = length;
   in.read(buffer+sizeof(ULONG),length-sizeof(ULONG));

   eaList.removeAll();
   if (eaList.mFEA2List)
      delete eaList.mFEA2List;
   eaList.mFEA2List = (FEA2LIST*) buffer;
   eaList.convert();

   return in;
}
