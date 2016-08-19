/* --------------------------------------------------------------------------
 * $RCSfile: EAList.hpp,v $
 * $Revision: 1.1 $
 * $Date: 1996/05/22 20:59:55 $
 * $Author: Bablok $
 * --------------------------------------------------------------------------
 * Synopsis:
 *
 * Interface for class EAList. This class wraps the extended attributes API of
 * OS/2 and handles complete sets of file-EAs.
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
 * $Log: EAList.hpp,v $
 * Revision 1.1  1996/05/22 20:59:55  Bablok
 * Initial revision
 *
 * -------------------------------------------------------------------------- */

#ifndef EALIST_H
  #define EALIST_H

  #ifndef INCL_BASE
     #define INCL_BASE
     #include <os2.h>
  #endif
  #ifndef _IKSSET_H
     #include <iksset.h>
  #endif
  #ifndef _ISTRING_
     #include <istring.hpp>
  #endif
  #ifndef EA_H
     #include "EA.hpp"
  #endif

  #define EALIST_DEFAULT_SIZE   20

  class ostream;
  class istream;

  class EAOps : public IStdMemOps, public IStdAsOps<EA> {
     public:
        IString const& key(EA const& ea) const {return ea.name();}
        class KeyOps {
           public:
              long compare(const IString& string1, const IString& string2) const {
                 return strcmpi(string1,string2);
              }
        } keyOps;
  };

  typedef IGKeySortedSet<EA,IString,EAOps> EASet;

  class EAList : public EASet{

    public:

       // constructors, destructor   -------------------------------------------

       EAList() : EASet(EALIST_DEFAULT_SIZE), mFEA2List(NULL) {}
       EAList(const EAList& eaList);
       EAList(const IString& basename, const EA& ea);
       EAList(const char* pathName) : EASet(EALIST_DEFAULT_SIZE),
                                                              mFEA2List(NULL)  {
          read((PVOID)pathName,true,false);
       }
       EAList(HFILE fileHandle) : EASet(EALIST_DEFAULT_SIZE), mFEA2List(NULL)  {
          read((PVOID) &fileHandle,false,false);
       }
       ~EAList() {
           if (mFEA2List)
              delete mFEA2List;
       }

       // get/set functions   --------------------------------------------------

       EAList& setValues(const EA& mvEA);

       // interaction with physical files   ------------------------------------

       EAList& read(const char* pathName, Boolean onlyEAsFromList=true) {
          return read((PVOID)pathName,true,onlyEAsFromList);
       }
       EAList& read(HFILE fileHandle, Boolean onlyEAsFromList=true) {
          return read((PVOID) &fileHandle,false,onlyEAsFromList);
       }

       EAList& write(const char* pathName, Boolean useFEA2List=false) {
          return write((PVOID)pathName,true,useFEA2List);
       }
       EAList& write(HFILE fileHandle, Boolean useFEA2List=false) {
          return write((PVOID) &fileHandle,false,useFEA2List);
       }

       EAList& remove(const char* pathName, Boolean onlyEAsFromList=true) {
          return remove((PVOID)pathName,true,onlyEAsFromList);
       }
       EAList& remove(HFILE fileHandle, Boolean onlyEAsFromList=true) {
          return remove((PVOID) &fileHandle,false,onlyEAsFromList);
       }

       // operators   ----------------------------------------------------------

       EAList& operator=(const EAList& eaList);
       friend ostream& operator<<(ostream& out, EAList& eaList);
       friend istream& operator>>(istream& in, EAList& eaList);

    private:

       // data members   -------------------------------------------------------

       FEA2LIST *mFEA2List;

       // auxiliary functions   ------------------------------------------------

       EAList& read(PVOID fileRef,Boolean isPathName,Boolean onlyEAsFromList);
       EAList& write(PVOID fileRef,Boolean isPathName,Boolean useFEA2List);
       EAList& remove(PVOID fileRef,Boolean isPathName,Boolean onlyEAsFromList);

       EAList&   convert();                              // FEA2LIST -> EAList
       GEA2LIST* createGEA2LIST() const;                 // read list
       GEA2LIST* createGEA2LIST(DENA2* pDENA2) const;    // read all
       FEA2LIST* createFEA2LIST();                       // write
       FEA2LIST* createFEA2LISTBuffer();                 // write
       FEA2LIST* createFEA2LISTBuffer(PVOID fileRef,     // read list
                                 Boolean isPathName);
       FEA2LIST* createFEA2LISTBuffer(DENA2* pDENA2);    // read all
       DENA2*    queryDENA2(PVOID fileRef,               // read all
                            Boolean isPathName) const;
    };
#endif
