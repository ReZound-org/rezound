%{

/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is not part of any particular application.
 * 
 * CNestedDataFile is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * CNestedDataFile is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#define isodigit(c) (c>='0' && c<='7')
#include <string.h>

#include <stack>

#include <istring>

#include "src/misc/CNestedDataFile/CNestedDataFile.h"
#include "src/misc/CNestedDataFile/cfg_types.h"

#include "cfg.tab.hpp"

using namespace std;

extern int mycfg_nerrs;
extern void cfg_includeFile(const char *filename);
extern void cfg_error(const CFG_LTYPE &pos,const char *msg);
extern void cfg_error(const char *msg);
extern stack<string> cfg_scopeStack;


/* some changes needed to be made to the implementation when flex changed how some things worked */
#ifndef YY_FLEX_SUBMINOR_VERSION
	#define YY_FLEX_SUBMINOR_VERSION 0
#endif
#define FLEX_VERSION (YY_FLEX_MAJOR_VERSION*100000+YY_FLEX_MINOR_VERSION*1000+YY_FLEX_SUBMINOR_VERSION)

bool started;
#if FLEX_VERSION<=205027 /*2.5.27*/
int yylineno; 
#endif

static stack<string> filenameStack;
static stack<int> linenoStack;
static stack<YY_BUFFER_STATE> inputBufferStack;

static char *quote(const char *input,int &length);

string currentFilename;

#define PRINTF
//#define PRINTF printf

#define YY_USER_ACTION  			\
	cfg_lloc.first_line=yylineno; 		\
	for(int t=0;t<yyleng;t++) 		\
		if(yytext[t]=='\n') 		\
			yylineno++; 		
	//printf("%s:%d: token: %s\n",currentFilename.c_str(),yylineno,yytext);fflush(stdout);


%}

%option prefix="cfg_"
%option never-interactive
%option stack

%x COMMENT


%%

\/\/.*$					{ /* comment skipped */ }

"/*"					{ yy_push_state(COMMENT); }
<COMMENT>[^*\n]*			{ /* eat anything that's not a '*' */ }
<COMMENT>"*"+[^*/\n]*			{ /* eat up '*'s not followed by '/'s */ }
<COMMENT>\n				{ /* handle multiple lines */ }
<COMMENT>"*"+"/"			{ yy_pop_state(); /* comment skipped */ }



(([0-9]*\.[0-9]+(e[\-+]?[0-9]+)?)|([0-9]+))		{ cfg_lval.stringValue=strdup(yytext); return LIT_NUMBER; }

([_0-9A-Za-z]|(\\[\x20-\xff]))+	{ 
		if(strcmp(yytext,"include")==0)
			return INCLUDE;
		else if(strcmp(yytext,"true")==0)
			return TRUE;
		else if(strcmp(yytext,"false")==0)
			return FALSE;
		else if(strcmp(yytext,"gettext")==0) 
			// doesn't actually do anything, except cause an entry for a string 
			// tagged with this keyword to be created in the rezound.pot file
			return GETTEXT;
		else
		{
			cfg_lval.stringValue=strdup(yytext); 

			// parse error if it contains CNestedDataFile::delim
			if(strstr(cfg_lval.stringValue,CNestedDataFile::delim.c_str())!=NULL)
			{
				cfg_error(cfg_lloc,("key name contains '"+CNestedDataFile::delim+"'").c_str());
				return 0;
			}
			

			// convert remove the '\' from '\char'
			for(size_t t=0;t<strlen(cfg_lval.stringValue);t++)
			{
				if(cfg_lval.stringValue[t]=='\\')
					memmove(cfg_lval.stringValue+t,cfg_lval.stringValue+t+1,strlen(cfg_lval.stringValue+t+1)+1);
			}

			return IDENT;
		}
	}
			


\"(\\.|[^\\"\n])*\"			{         
	int length;
	char *data=quote(yytext,length);
	cfg_lval.stringValue=data;
	return LIT_STRING;
}


[\ \n\t\v\b\r\f\a]+			{ /* Eat White Space */ }

"!="					{ return NE; }
"=="					{ return EQ; }
">="					{ return GE; }
"<="					{ return LE; }
"||"					{ return OR; }
"&&"					{ return AND; }

. 					{ return *yytext; }




%%


/*
	This DFM recognizes ANSI-C string literals with the exception that

	\[0-9][0-9][0-9]		is not octal but base-10
	\o[0-9][0-9][0-9]		is octal
	\x[0-9a-fA-F][0-9a-fA-F]	is hex

	Also, when reading the number string after the escape sequence 
	it only reads up to 3 chars for base-10 and octal and only up to 2 for hex
*/
static char *quote(const char *input,int &length)
{
	int input_index=0; 
	int numbufLength=0;
	length=0;
	// this is okay because the string can only get shorted or stay same
	char *yytext=(char *)malloc(strlen(input)+1);
	char numbuf[1024];

	int state=0;
	bool done=false;
	while(!done)
	{
		if(input_index<0)
		{
			cfg_error("internal error: function quote()");
			goto done;
		}

		char c=input[input_index++];
		if(c==0)
			break;

		// make sure numbufLength is still less than 1024 ???

		switch(state)
		{
			case 0: // reading for the open quote
				if(c=='\"')
				{
					yytext[length++]='\"';
					state=1;
				}
				else
				{
					cfg_error("internal error: invalid quote string passed to quote()");
					goto done;
				}
				break;
			case 1: // reading for closing quote or \ for an escape sequence
				if(c=='\"')
				{
					yytext[length++]='\"';
					done=true;
				}
				else if(c=='\\')
					state=2;
				else
					yytext[length++]=c;

				break;
			
			case 2: // reading the char immediatly following the \ of an escape sequence
				if(isdigit(c))
				{
					numbufLength=0;
					numbuf[numbufLength++]=c;
					state=5;
				}
				else if(c=='x')
					state=3; // go read a hex number
				else if(c=='o')
					state=6; // go read an octal number
				else 
				{
					int replaceChar=-1;
					switch(c)
					{
					case 'n':	
						replaceChar='\n';
						break;
					case 't':
						replaceChar='\t';
						break;
					case 'v':
						replaceChar='\v';
						break;
					case 'b':
						replaceChar='\b';
						break;
					case 'r':
						replaceChar='\r';
						break;
					case 'f':
						replaceChar='\f';
						break;
					case 'a':
						replaceChar='\a';
						break;
					case '\\':
						replaceChar='\\';
						break;
					case '\?':
						replaceChar='\?';
						break;
					case '\'':
						replaceChar='\'';
						break;
					case '\"':
						replaceChar='\"';
						break;
					}

					if(replaceChar==-1)
					{
						cfg_error(("unknown escape sequence: \\"+string((char *)&c,1)).c_str());
						state=1;
					}
					else
					{
						yytext[length++]=replaceChar;
						state=1;
					}
				}
				
				
				break;

			case 3: // reading first digit of hex escape sequence
				if(isxdigit(c))
				{
					numbufLength=0;
					numbuf[numbufLength++]=c;
					state=4;
				}
				else
				{
					cfg_error(("unknown escape sequence: \\"+string((char *)&c,1)).c_str());
					state=1;
				}
				break;

			case 4: // reading second or more digit of a hex escape sequence
				if(numbufLength<2 && isxdigit(c))
				{
					numbuf[numbufLength++]=c;
				}
				else
				{
					numbuf[numbufLength]=0;
					int asciiVal=strtol(numbuf,NULL,16);
					if(asciiVal<0 || asciiVal>255)
						cfg_error(("hex value for escape sequence is out of range: \\x"+string(numbuf)).c_str());
					else
						yytext[length++]=(char)asciiVal;

					input_index--; // push back the char just read to be handled by state 1

					state=1;
				}
				break;

			case 5: // reading second or more digit of base10 escape sequence
				if(numbufLength<3 && isdigit(c))
				{
					numbuf[numbufLength++]=c;
				}
				else
				{
					numbuf[numbufLength]=0;
					int asciiVal=strtol(numbuf,NULL,10);
					if(asciiVal<0 || asciiVal>255)
						cfg_error(("value for escape sequence is out of range: \\"+string(numbuf)).c_str());
					else
						yytext[length++]=(char)asciiVal;

					input_index--; // push back the char just read to be handled by state 1

					state=1;
				}
				break;

			case 6: // reading first digit of an octal escape sequence
				if(isodigit(c))
				{
					numbufLength=0;
					numbuf[numbufLength++]=c;
					state=7;
				}
				else
				{
					cfg_error(("unknown escape sequence: \\"+string((char *)&c,1)).c_str());
					state=1;
				}
				break;

			case 7: // reading second or more digit of an octal escape sequence
				if(numbufLength<3 && isodigit(c))
				{
					numbuf[numbufLength++]=c;
				}
				else
				{
					numbuf[numbufLength]=0;
					int asciiVal=strtol(numbuf,NULL,8);
					if(asciiVal<0 || asciiVal>255)
						cfg_error(("octal value for escape sequence is out of range: \\o"+string(numbuf)).c_str());
					else
						yytext[length++]=(char)asciiVal;

					input_index--;// push back the char just read to be handled by state 1

					state=1;
				}
				break;

			default:
				cfg_error("internal error: unknown state in quote()");
				// probably should return a value that means STOP
				goto done;
		}
	}

	done:

	// HERE: yytext should be "..." and we shouldn't return the quotes

	length-=2;
	yytext[length+1]=0;
	memmove(yytext,yytext+1,length+1);
	return yytext;
}

void cfg_init()
{
	yylineno=0;
	mycfg_nerrs=0;

	while(!cfg_scopeStack.empty())
		cfg_scopeStack.pop();

	while(!filenameStack.empty())
		filenameStack.pop();

	while(!linenoStack.empty())
		linenoStack.pop();

	while(!inputBufferStack.empty())
		inputBufferStack.pop();

	started=false;

	if(YY_CURRENT_BUFFER)
	{
		yy_delete_buffer(YY_CURRENT_BUFFER);
#if FLEX_VERSION<205031 /*2.5.31*/
		YY_CURRENT_BUFFER=NULL;
#endif
	}

	if(CNestedDataFile::s2at_string.size()>0) // use s2at_string if it's not blank (meaning that string_to_anytype<vector>() is invoking the parser)
		yy_scan_string(CNestedDataFile::s2at_string.c_str());
	else
		cfg_includeFile(CNestedDataFile::initialFilename.c_str());

	started=true;

}

void cfg_deinit()
{
}


void cfg_includeFile(const char *filename)
{
	FILE *f=fopen(filename,"r");
	if(f==NULL)
		throw runtime_error(string(__func__)+" -- "+istring(yylineno)+": -- could not open file: "+string(filename));
	
	// read text from file into scanString
	#define READ_CHUNK_SIZE 256
	char *scanString=(char *)malloc(READ_CHUNK_SIZE+1);
	int length;
	int totalLength=0;
	while((length=fread(scanString+totalLength,1,READ_CHUNK_SIZE,f))>0)
	{
		totalLength+=length;
		scanString=(char *)realloc(scanString,totalLength+READ_CHUNK_SIZE+1);
	}

	scanString[totalLength]=0;
	
	fclose(f);

	if(started && YY_CURRENT_BUFFER!=NULL)
	{
		filenameStack.push(currentFilename);
		linenoStack.push(yylineno);
		inputBufferStack.push(YY_CURRENT_BUFFER);
	}

	yy_scan_string(scanString);

	free(scanString);

	yylineno=1;
	currentFilename=filename;
}

int yywrap(void)
{
	if(YY_CURRENT_BUFFER)
	{
		yy_delete_buffer(YY_CURRENT_BUFFER);
#if FLEX_VERSION<205031 /*2.5.31*/
		YY_CURRENT_BUFFER=NULL;
#endif
	}

	if(inputBufferStack.empty())
		return true;
	else
	{
		currentFilename=filenameStack.top(); filenameStack.pop();
		yylineno=linenoStack.top(); linenoStack.pop();
		yy_switch_to_buffer(inputBufferStack.top()); inputBufferStack.pop();
		return false;
	}
}

