/* --------------------------------------------------------------------------
 * $RCSfile: eatool.cpp,v $
 * $Revision: 1.1 $
 * $Date: 1996/05/22 20:59:55 $
 * $Author: Bablok $
 * --------------------------------------------------------------------------
 * Synopsis:
 *
 * Simple commandline EA tool.
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
 * $Log: eatool.cpp,v $
 * Revision 1.1  1996/05/22 20:59:55  Bablok
 * Initial revision
 *
 * -------------------------------------------------------------------------- */

#include <fstream.h>
#include <iomanip.h>
#include <istring.hpp>
#include <iexcbase.hpp>
#include "EA.hpp"
#include "EAList.hpp"

#define INDENT_DELTA 3

void usage(const char* pgmName);
void dumpEA(const EA& ea, int indent);
void dumpEAList(const EAList& eaList, int indent);

int main(int argc, char *argv[]) {

   if (argc < 2 || argc > 5 || *argv[1] != '-' ||
       (*(argv[1]+1) != 'r' && *(argv[1]+1) != 'w' && *(argv[1]+1) != 'd') ||
                                        (*(argv[1]+1) == 'r' && argc >  4) ||
                                        (*(argv[1]+1) == 'w' && argc != 5) ||
                                        (*(argv[1]+1) == 'd' && argc >  4)   )
      usage(argv[0]);
   try {
      switch (*(argv[1]+1)) {
         case 'r':
            if (argc == 3) {
               EAList list(argv[2]);
               dumpEAList(list,0);
            } else {
               EA ea(argv[3]);
               ea.read(argv[2]);
               dumpEA(ea,0);
            }
            return 0;
         case 'w': {
            IString name(argv[3]), value(argv[4]);
            EA ea(name,value);
            ea.write(argv[2]);
            return 0;
         } case 'd':
            if (argc == 3) {
               EAList list;
               list.remove(argv[2],false);
            } else {
               EA ea(argv[3]);
               ea.remove(argv[2]);
            }
            return 0;
         default:
            usage(argv[0]);
           break;
      }
      return 0;
   }
   catch (IException& exc) {
      cerr << "Error:" << endl;
      for (int i=0; i<exc.textCount(); ++i)
         cerr << "   " << exc.text(i) << endl;
      return 3;
   }
}

///////////////////////////////////////////////////////////////////////////////
// dumpEA(): Formatted output of EA
//
void dumpEA(const EA& ea, int indent) {
   if (ea.value() == "")
      return;

   cout.setf(ios::right,ios::adjustfield);
   cout << setw(7+indent) << "name:  ";
   cout.setf(ios::left,ios::adjustfield);
   cout << ea.name() << endl;

   cout.setf(ios::right,ios::adjustfield);
   cout << setw(7+indent) << "type:  ";
   cout.setf(ios::left,ios::adjustfield);
   cout << ea.typeAsString();
   if (ea.flag() == FEA_NEEDEA)
      cout << " (EA is critical)";
   cout << endl;

   cout.setf(ios::right,ios::adjustfield);
   cout << setw(7+indent) << "value: ";
   cout.setf(ios::left,ios::adjustfield);
   if (ea.type() != EAT_MVMT && ea.type() != EAT_MVST)
      if (ea.type() == EAT_ASCII)
         cout << ea.value() << endl;
      else
         cout << IString::c2x(ea.value()) << endl;
   else {
      cout << endl;
      EAList list(ea.name(),ea);
      dumpEAList(list,indent+INDENT_DELTA);
   }
   return;
}


///////////////////////////////////////////////////////////////////////////////
// dumpEAList(): Formatted output of EAList
//
void dumpEAList(const EAList& eaList, int indent) {
   EAList::Cursor current(eaList);
   forCursor(current) {
      dumpEA(current.element(),indent);
      cout << endl;
   }
   return;
}

///////////////////////////////////////////////////////////////////////////////
// usage(): Show syntax
//
void usage(const char* pgmName) {
   cerr << "EATool (c) by Bernhard Bablok, 1996\n\n"
           "This program is distributed in the hope that it will be\n"
           "useful, but WITHOUT ANY WARRANTY; without even the implied\n"
           "warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"

           "Usage: " << pgmName << " -[rwd] fileName [eaName] [eaValue]\n"
           "\tr: Read   (all EAs or the EA with name eaName) \n"
           "\tw: Write  (sets value of eaName to eaValue)\n"
           "\td: Delete (all EAs or EA with name eaName)" << endl;
  exit(3);
}
