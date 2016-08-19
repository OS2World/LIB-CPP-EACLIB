/* --------------------------------------------------------------------------
 * $RCSfile: EA.hpp,v $
 * $Revision: 1.1 $
 * $Date: 1996/05/22 20:59:55 $
 * $Author: Bablok $
 * --------------------------------------------------------------------------
 * Synopsis:
 *
 * Interface for class EA. This class wraps the extended attributes API of
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
 * $Log: EA.hpp,v $
 * Revision 1.1  1996/05/22 20:59:55  Bablok
 * Initial revision
 *
 * -------------------------------------------------------------------------- */

#ifndef EA_H
  #define EA_H

  #ifndef INCL_BASE
     #define INCL_BASE
     #include <os2.h>
  #endif
  #ifndef _ISTRING_
     #include <istring.hpp>
  #endif

  #define MSG_FILE               "EALerr.msg"
  #define ERR_MEM_ALLOC_FAILED   1
  #define ERR_NO_EA_NAME         2
  #define ERR_EALIST_EMPTY       3
  #define ERR_NOT_MULTI_VALUED   4
  #define ERR_INVALID_TYPE       5
  #define ERR_ELEMENT_COUNT      6

  class EAList;

  class EA {

     friend class EAList;
     friend const IString& key(const EA& ea) {return ea.mName;}

     public:

        // constructors, destructor   ------------------------------------------

        EA(const IString& name, const void* buffer, unsigned long length,
                                          USHORT type=EAT_BINARY, BYTE flag=0) :
                                mName(name), mValue(buffer,length), mType(type),
                                                                   mFlag(flag) {
           mName.upperCase();
        }
        EA(const IString& name="", const IString& value="", BYTE flag=0) :
                     mName(name), mValue(value), mType(EAT_ASCII), mFlag(flag) {
           mName.upperCase();
        }
        EA(const IString& name, const EAList& EAList, BYTE flag=0);
        ~EA() {}

        // get/set functions   -------------------------------------------------

        const IString& value() const {return mValue;}
        EA& setValue(const IString& value) {
           mValue = value;
           return *this;
        }
        const IString& name() const {return mName;}
        EA& setName(const IString& name) {
           mName = name;
           return *this;
        }
        USHORT  type() const {return mType;}
        static IString typeAsString(USHORT type);
        IString typeAsString() const {
           return typeAsString(mType);
        }
        EA& setType(USHORT type) {
           mType = type;
           return *this;
        }
        BYTE   flag() const {return mFlag;}
        EA& setFlag(BYTE flag) {
           mFlag = flag;
           return *this;
        }

        USHORT numValues() const;
        USHORT codePage() const;

        // interaction with physical files   -----------------------------------

        EA& read(const char* pathName) {           // read EA from file
           return read((PVOID) pathName,true);
        }
        EA& read(HFILE fileHandle) {
           return read((PVOID) &fileHandle,false);
        }

        EA& write(const char* pathName) {          // write EA to file
           return write((PVOID) pathName,true);
        }
        EA& write(HFILE fileHandle) {
           return write((PVOID) &fileHandle,false);
        }

        EA& remove(const char* pathName) {         // delete EA from file
           return remove((PVOID) pathName,true);
        }
        EA& remove(HFILE fileHandle) {
           return remove((PVOID) &fileHandle,false);
        }

     private:

        // data members   ------------------------------------------------------

        IString mName, mValue;
        USHORT  mType;
        BYTE    mFlag;

        // auxiliary functions   -----------------------------------------------

        EA(FEA2* fea2);

        EA& read(PVOID fileRef, Boolean isPathName);
        EA& write(PVOID fileRef, Boolean isPathName);
        EA& remove(PVOID fileRef, Boolean isPathName);

        static Boolean isLengthPreceded(USHORT type);
        Boolean EA::isLengthPreceded() const {
           return isLengthPreceded(mType);
        }

        ULONG createFEA2(char *buffer) const;
        GEA2LIST* createGEA2LIST() const;
        FEA2LIST* createFEA2LIST() const;
        FEA2LIST* createFEA2LISTBuffer() const;
        FEA2LIST* createFEA2LISTBuffer(PVOID fileRef, Boolean isPathName) const;
     };
#endif
