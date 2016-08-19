/* --------------------------------------------------------------------------
 * $RCSfile: tdrive.cpp,v $
 * $Revision: 1.1 $
 * $Date: 1996/05/22 20:59:55 $
 * $Author: Bablok $
 * --------------------------------------------------------------------------
 * Synopsis:
 *
 * Test driver program for class EA and EAList.
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
 * $Log: tdrive.cpp,v $
 * Revision 1.1  1996/05/22 20:59:55  Bablok
 * Initial revision
 *
 * -------------------------------------------------------------------------- */

#include <fstream.h>
#include <istring.hpp>
#include <iexcbase.hpp>
#include "EA.hpp"
#include "EAList.hpp"

void dumpEA(const EA& ea);
void dumpEAList(const EAList& eaList);

int main(int argc, char *argv[]) {

   IString basename, name, value, file, dumpFile;
   fstream stream;
   EAList eaList;
   EA     ea("dummy",IString("dummy"));
   if (argc > 1)
      file = argv[1];

   cout << "Test driver for class EA:\n\n";
   while (1) {
      char answer;
      cout << "Select:\n\n"
              "1 filename       8 write  EA   b write  list\n"
              "2 create EA      9 remove EA   c remove list\n"
              "3 create EAList  a read   EA   d remove all\n"
              "4 EAList->EA                   e read list\n"
              "5 EA->EAList                   f read all\n"
              "6 print EA                     g dump to file\n"
              "7 print EAList                 h read from file\n"
              "q quit"
           << endl;
      cin >> answer; cin.ignore(80,'\n');
      try {
         switch (answer) {
            case 'q':
            case 'Q':
               return 0;
            case '1':                                     // set file
               cout << "Enter filename:";
               cin >> file;
               break;
            case '2':                                     // define EA
               cout << "Enter name of EA:";
               cin >> name;
               cout << "Enter value of EA:";
               value = IString::lineFrom(cin);
               ea.setName(name);
               ea.setValue(value);
               break;
            case '3':                                     // create EAList
               eaList.removeAll();
               while (1) {
                  cout << "Enter name of EA:";
                  name = IString::lineFrom(cin);
                  if (name == "")
                     break;
                  cout << "Enter value of EA:";
                  value = IString::lineFrom(cin);
                  eaList.addOrReplaceElementWithKey(EA(name,value));
               }
               break;
            case '4':
               ea = EA("list",eaList);
               break;
            case '5':
               cout << "Enter basename for EAList:";
               basename = IString::lineFrom(cin);
               eaList = EAList(basename,ea);
               break;
            case '6':
               dumpEA(ea);
               break;
            case '7':
               dumpEAList(eaList);
               break;
            case '8':
               ea.write(file);
               cout << "EA successfully written!" << endl;
               dumpEA(ea);
               break;
            case '9':
               ea.remove(file);
               cout << "EA successfully removed!" << endl;
               dumpEA(ea);
               break;
            case 'a':
               ea.read(file);
               cout << "EA successfully read!" << endl;
               dumpEA(ea);
               break;
            case 'b':
               eaList.write(file);
               cout << "EAList successfully written!" << endl;
               dumpEAList(eaList);
               break;
            case 'c':
               eaList.remove(file);
               cout << "EAs from list successfully removed!" << endl;
               dumpEAList(eaList);
               break;
            case 'd':
               eaList.remove(file,false);
               cout << "All EAs successfully removed!" << endl;
               dumpEAList(eaList);
               break;
            case 'e':
               eaList.read(file);
               cout << "EAs read from list:" << endl;
               dumpEAList(eaList);
               break;
            case 'f':
               eaList.read(file,false);
               cout << "All EAs from file:" << endl;
               dumpEAList(eaList);
               break;
            case 'g':
               cout << "Enter filename to dump to:" << endl;
               dumpFile = IString::lineFrom(cin);
               stream.open(dumpFile,ios::bin|ios::out);
               if (stream) {
                  stream << eaList;
                  stream.close();
                  cout << "EAList written to file " << dumpFile << endl;
               } else
                  cout << "Open failed for file " << dumpFile << endl;
               break;
            case 'h':
               cout << "Enter filename to read from:" << endl;
               dumpFile = IString::lineFrom(cin);
               stream.open(dumpFile,ios::bin|ios::in);
               if (stream) {
                  stream >> eaList;
                  stream.close();
                  cout << "EAList from file " << dumpFile << ":" << endl;
                  dumpEAList(eaList);
               } else
                  cout << "Open failed for file " << dumpFile << endl;
               break;
            default:
              break;
         }  // endswitch
      } catch (IException& exc) { }
   }  // end while(1)
}


void dumpEA(const EA& ea) {
   switch (ea.type()) {
      case EAT_ASCII:
         cout << ea.name() <<": >" << ea.value() << "<" << endl;
         break;
      case EAT_MVMT:
      case EAT_MVST:
          try {
             cout << ea.name() << " [" << endl;
             EAList list("value",ea);
             dumpEAList(list);
             cout << "]" << endl;
          }
          catch (IInvalidRequest& exc) {
            cout << ea.name() <<": >" << IString::c2x(ea.value()) << "<" << endl;
          }
         break;
      default:
         cout << ea.name() <<": >" << IString::c2x(ea.value()) << "<" << endl;
        break;
   }
}

void dumpEAList(const EAList& eaList) {
   EAList::Cursor current(eaList);
   forCursor(current)
      dumpEA(current.element());
}
