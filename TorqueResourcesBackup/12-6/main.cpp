//
//  main.cpp
//  TorqueMinifier
//
//  Created by HiGuy Smith on 6/12/12.
//  Copyright (c) 2012 CouleeApps. All rights reserved.
//

/**

 @arg "-n"
   Don't strip newlines
 @arg "-l"
   Don't strip local variables
 @arg "-s"
   Don't strip spaces
 @arg "-c"
   Don't strip comments
 @arg "-i"
   Don't strip indents
 @arg "-h"
   Don't add header
 @arg "-t"
   Trace mode (log all actions)

 */

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>

using namespace std;

//Maximum lines
#define maxlength 4096
//Maximum local variables
#define maxvars 1024
//String that will *NEVER* be in code
#define deleteme "<<<<>>>>DELETE_ME<<<<>>>>DELETE_ME<<<<>>>>"

//Globals (just for "nobody") :3
static bool trace = false;
static bool newLines = true, locals = true, spaces = true, comments = true, indents = true, header = true;
static const char *file;
static string line[maxlength];
static int lines;

//http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

// trim from start
static inline string &ltrim(string &s) {
   s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
   return s;
}

// trim from end
static inline string &rtrim(string &s) {
   s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
   return s;
}

// trim from both ends
static inline string &trim(string &s) {
   return ltrim(rtrim(s));
}

static inline bool countbackslash(string line, int pos) {
   char charbefore = 'Q';
   if (pos != 0)
      charbefore = line[pos - 1];
   if (charbefore != '\\')
      return true;
   int backslashes = 1;
   pos --;
   while (line[pos - 1] == '\\' && pos > 0) {
      backslashes ++;
      pos --;
   }
   return !(backslashes % 2 == 1);
}

static inline long firstPos(string blob, const char *find, int start) {
   long lowest = blob.length();
   for (int i = 0; i < strlen(find); i ++) {
      char search = find[i];
      long pos = blob.find(search, start);
      if (pos == string::npos)
         continue;
      if (pos < lowest)
         lowest = pos;
   }
   return lowest;
}

static inline string getMinName(int num) {
   const char *letters = "abcdefghijklmnopqrstuvwxyz";
   string ret = "%";
   while (true) {
      int pos = num % 26;
      ret += letters[pos];
      num -= pos;
      num /= 26;
      if (num == 0)
         return ret;
   }
}

static inline string nilAll(string line, const char *nils) {
   bool inQuotes = false, inDoubleQuotes = false;
   for (int i = 0; i < line.length(); i ++) {
      if (trace && i % 300 == 0)
         printf("Nilling loop %d\n", i);
      char charbefore = 'Q';
      if (i != 0)
         charbefore = line[i - 1];
      char charon = line[i];
      char charafter = line[i + 1];
      if (!inQuotes && !inDoubleQuotes && charon != '\'' && charon != '\"' && strchr(nils, charon) != NULL) {
         if (charbefore == ' ') {
            line = line.substr(0, i - 1) + line.substr(i, line.length());
            i --;
         }
         if (charafter == ' ') {
            line = line.substr(0, i + 1) + line.substr(i + 2, line.length());
            i --;
         }
      }
      if (charon == '\'' && !inDoubleQuotes) {
         if (!inQuotes) {
            inQuotes = true;
            if (charbefore == ' ' && strchr(nils, charon) != NULL) {
               line = line.substr(0, i - 1) + line.substr(i, line.length());
               i --;
            }
         } else {
            if (countbackslash(line, i)) {
               inQuotes = false;
               if (charafter == ' ' && strchr(nils, charon) != NULL) {
                  line = line.substr(0, i + 1) + line.substr(i + 2, line.length());
                  i --;
               }
            }
         }
      }
      if (charon == '\"' && !inQuotes) {
         if (!inDoubleQuotes) {
            inDoubleQuotes = true;
            if (charbefore == ' ' && strchr(nils, charon) != NULL) {
               line = line.substr(0, i - 1) + line.substr(i, line.length());
               i --;
            }
         } else {
            if (countbackslash(line, i)) {
               inDoubleQuotes = false;
               if (charafter == ' ' && strchr(nils, charon) != NULL) {
                  line = line.substr(0, i + 1) + line.substr(i + 2, line.length());
                  i --;
               }
            }
         }
      }
   }
   return line;
}

inline bool minify() {
   //Read in
   FILE *f = fopen(file, "r");
   if (f == NULL) {
      printf("Could not read\n");
      return false;
   }

   long chars = 0;

   char lineIn[maxlength];
   while (fgets(lineIn, maxlength, f) != NULL) {
      line[lines ++] = lineIn;
      if (indents)
         trim(line[lines - 1]);
      chars += strlen(lineIn);
      if (trace)
         printf("%s\n", line[lines - 1].c_str());
   }

   fclose(f);

   if (comments) {
      for (int i = 0; i < lines; i ++) {
         if (trace && line[i].compare("") != 0)
            printf("Comments for line %d\n", i);
         if (line[i].find("//") != string::npos) {
            bool inQuotes = false;
            bool inDoubleQuotes = false;
            for (int j = 0; j < line[i].find("//"); j ++) {
               char charon;
               charon = line[i][j];
               if (charon == '\'' && !inDoubleQuotes) {
                  if (!inQuotes)
                     inQuotes = true;
                  else if (countbackslash(line[i], j))
                     inQuotes = false;
               }
               if (charon == '\"' && !inQuotes) {
                  if (!inDoubleQuotes)
                     inDoubleQuotes = true;
                  else if (countbackslash(line[i], j))
                     inDoubleQuotes = false;
               }
            }
            if (!inQuotes && !inDoubleQuotes) {
               line[i].erase(line[i].find("//"), string::npos);
               if (line[i] == "") {
                  line[i] = deleteme;
               }
            }
         }
      }
   }

   if (locals) {
      string localMin[maxvars];
      string localNew[maxvars];
      int localCount = 0;
      for (int i = 0; i < lines; i ++) {
         long lastlocal = 0;
         long startpos;
         while ((startpos = line[i].find("%", lastlocal)) != string::npos) {
            string varblob = line[i].substr(startpos, line[i].length());
            lastlocal = startpos + 1;
            long endPos = firstPos(varblob, " ,./?;\"\'[]{}\\|-=_+()*&%$@#!", 1);
            string varname = varblob.substr(0, endPos);
            if (varname.compare("%") != 0) {
               bool found = false;
               int onPos = -1;
               for (int j = 0; j < maxvars; j ++) {
                  if (localMin[j].compare(varname) == 0) {
                     found = true;
                     onPos = j;
                  }
                  if (found)
                     break;
               }
               if (!found) {
                  localMin[localCount] = varname;
                  localNew[localCount] = getMinName(localCount);
                  if (trace)
                     printf("New Local: %s %s\n", varname.c_str(), localNew[localCount].c_str());
                  onPos = localCount;
                  localCount ++;
               } else {
                  if (trace)
                     printf("Old local: %s %s\n", varname.c_str(), localNew[onPos].c_str());
               }
               string out = line[i].substr(0, startpos);
               out += localNew[onPos];
               out += line[i].substr(endPos + startpos, line[i].length() - endPos - startpos);
               line[i] = out;
            }
         }
      }
   }

   //Spaces
   if (spaces) {
      for (int i = 0; i < lines; i ++) {
         line[i] = nilAll(line[i], " ,@;=(){}$!%+-/*[]<>:|&");
         if (trace && line[i].compare("") != 0)
            printf("%s\n", line[i].c_str());
      }
   }

   long finallen = 0;

   for (int i = 0; i < lines; i ++) {
      finallen += line[i].length();
      if (trace && line[i].compare("") != 0)
         printf("%s\n", line[i].c_str());
   }


   string fileExt = file;
   string filePath = fileExt.substr(0, fileExt.find_last_of("/")) + "/";
   fileExt = fileExt.substr(fileExt.find_last_of("/") + 1, fileExt.length());
   string fileBase = fileExt.substr(0, fileExt.find_last_of("."));
   fileExt = fileExt.substr(fileExt.find_last_of("."), fileExt.length());

   string head = "";
   long time = clock();
   if (trace)
      printf("It took %f seconds\n", (float)time / CLOCKS_PER_SEC);
   if (header) {
      stringstream ss (stringstream::in | stringstream::out);
      ss << (float)time / CLOCKS_PER_SEC;
      string timec = ss.str();
      stringstream sa (stringstream::in | stringstream::out);
      sa << (float)chars;
      string charc = sa.str();
      stringstream sb (stringstream::in | stringstream::out);
      sb << (float)finallen;
      string lenc = sb.str();
      head = "//-----------------------------------------------------------------------------\n// Original file: " + fileBase + fileExt + "\n// Script minified by Minifier 1.0 C++ by HiGuy\n//\n// Minification Stats:\n// Script minified in " + timec + " seconds\n// Script shrunken from " + charc + " characters to " + lenc + " characters\n//-----------------------------------------------------------------------------\n\n";
   }

   if (trace)
      printf("Header: %s\n", head.c_str());

   string minFile = filePath + fileBase + "_min" + fileExt;
   if (trace)
      printf("Minpath: %s\n", minFile.c_str());

   string final = "";
   for (int i = 0; i < lines; i ++) {
      if (newLines)
         final += " ";
      if (line[i].compare(deleteme) != 0) {
         final += line[i];
         if (!newLines)
            final += "\n";
      }
   }
   final = nilAll(final, " ,@;=(){}$!%+-/*[]<>:|&");
   if (header)
      final = head + final;

   FILE *out = fopen(minFile.c_str(), "w");
   if (out == NULL)
      return false;
   fputs(final.c_str(), out);
   fclose(out);

   return true;
}

inline bool parseArgs(int argc, const char *argv[]) {
   if (argc < 2)
      return false;
   file = argv[1];
   printf("Parsing %d arguments...\n", argc);
   for (int i = 2; i < argc; i ++) {
      printf("Argument: %s\n", argv[i]);
      if (strcmp(argv[i], "-n") == 0)
         newLines = false;
      if (strcmp(argv[i], "-l") == 0)
         locals = false;
      if (strcmp(argv[i], "-s") == 0)
         spaces = false;
      if (strcmp(argv[i], "-c") == 0)
         comments = false;
      if (strcmp(argv[i], "-i") == 0)
         indents = false;
      if (strcmp(argv[i], "-h") == 0)
         header = false;
      if (strcmp(argv[i], "-t") == 0)
         trace = true;
   }
   if (!comments && newLines) {
      printf("Newlines cannot be stripped with comments!\n");
      printf("Enabling newlines...\n");
      newLines = false;
   }
   return true;
}

int main (int argc, const char * argv[])
{
   if (!parseArgs(argc, argv))
      return false;
   return minify();
}

