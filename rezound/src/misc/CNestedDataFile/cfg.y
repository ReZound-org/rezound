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

#include <math.h>

#include <stack>
#include <stdexcept>

#include "CNestedDataFile.h"

#include <istring>

struct RTokenPosition
{
	int first_line,last_line;
	int first_column,last_column;
	const char *text;
};
#define YYLTYPE RTokenPosition
#define YYLSP_NEEDED 1

stack<string> scopeStack;

void cfg_error(int line,const char *msg);
void cfg_error(const YYLTYPE &pos,const char *msg);
void cfg_error(const char *msg);

int cfg_lex();

void cfg_init();
void cfg_deinit();

void cfg_includeFile(const char *filename);

void checkForDupMember(int line,const char *key);

static const string getCurrentScope();

int myyynerrs=0;

#define VERIFY_TYPE(ap,a) 				\
	if(a->type!=CNestedDataFile::ktFloat)			\
		cfg_error(ap,"invalid operand; numeric type expected"); 	

#define BINARY_EXPR(r,ap,a,bp,b,o)	\
	VERIFY_TYPE(ap,a)		\
	VERIFY_TYPE(bp,b)		\
	r=a;				\
	r->floatValue = r->floatValue o b->floatValue;	 \
	delete b;


%}

%expect 1

%union cfg_parse_union
{
	char *				stringValue;
	double				floatValue;
	int 				intValue;
	CNestedDataFile::CVariant *	variant;
	vector<CNestedDataFile::CVariant> *variantList;
}

%token 	<stringValue> 	IDENT
%token	<floatValue>	LIT_NUMBER
%token	<stringValue> 	LIT_STRING

%token			TRUE
%token			FALSE
%token			INCLUDE

%token			NE
%token			EQ
%token			GE
%token			LE
%token			OR
%token			AND


%type	<variantList>	array_body;
%type	<variantList>	array_body2;

// numeric expression stuff
%type	<variant>	primary_expr
%type	<variant>	unary_expr
%type	<intValue>	unary_op
%type	<variant>	multiplicative_expr
%type	<variant>	additive_expr
%type	<variant>	relational_expr
%type	<variant>	equality_expr
%type	<variant>	logical_and_expr
%type	<variant>	logical_or_expr
%type	<variant>	conditional_expr
%type	<variant>	expr

%type	<stringValue>	qualified_ident

%start start


%%



start 
	: { cfg_init(); } scope_body
	{
		cfg_deinit();

		return(myyynerrs!=0);
	}
	;


scope
	: IDENT '{' 
	{ /* mid-rule action */

		checkForDupMember(@1.first_line,$1);

		scopeStack.push($1);

		free($1);

		// now continue parsing for new scope's body
	} scope_body '}' {
		scopeStack.pop();
	}
	;

scope_body
	: /* empty */
	| scope_body scope_body_item

	// ERROR CASES
	| error ';'
	{
		cfg_error(@1,"syntax error");
	}
	| error
	{
		cfg_error(@1,"syntax error");
	}
	;

scope_body_item
	: scope

	| IDENT '=' expr ';'
	{
		checkForDupMember(@1.first_line,$1);
		if($3->type==CNestedDataFile::ktFloat)
			CNestedDataFile::parseTree->createKey((getCurrentScope()+$1).c_str(),$3->floatValue);
		else if($3->type==CNestedDataFile::ktString)
			CNestedDataFile::parseTree->createKey((getCurrentScope()+$1).c_str(),$3->stringValue);
		else
			yyerror(@2,"assigning an invalid typed value to an identifier");

		delete $3;
		free($1);
	}
	| IDENT '[' ']' '=' '{' array_body '}'
	{
		checkForDupMember(@1.first_line,$1);
		CNestedDataFile::parseTree->createKey((getCurrentScope()+$1).c_str(),*$6);

		delete $6;
		free($1);
	}
	| INCLUDE LIT_STRING
	{
		cfg_includeFile($2);
		free($2);
	}
	| ';' // allow stray ';'s
	;

array_body
	: /* empty */
	{
		$$=new vector<CNestedDataFile::CVariant>;
	}
	| array_body2
	{
		$$=$1;
	}
	;

array_body2
	: expr
	{
		$$=new vector<CNestedDataFile::CVariant>;

		if($1->type==CNestedDataFile::ktFloat)
			$$->push_back(CNestedDataFile::CVariant("",$1->floatValue));
		else if($1->type==CNestedDataFile::ktString)
			$$->push_back(CNestedDataFile::CVariant("",$1->stringValue));
		else
			yyerror(@1,"invalid typed value in array initializer");

		delete $1;
	}

	| array_body ',' expr
	{
		$$=$1;

		if($3->type==CNestedDataFile::ktFloat)
			$$->push_back(CNestedDataFile::CVariant("",$3->floatValue));
		else if($3->type==CNestedDataFile::ktString)
			$$->push_back(CNestedDataFile::CVariant("",$3->stringValue));
		else
			yyerror(@3,"invalid typed value in array initializer");

		delete $3;
	}

	// ERROR CASES
	| error
	{
		$$=new vector<CNestedDataFile::CVariant>;
	}
	| array_body ',' error
	{
		$$=$1;
	}
	;
	
	


primary_expr
	// literal values
	: LIT_NUMBER
	{
		$$=new CNestedDataFile::CVariant("",$1);
	}
	| LIT_STRING
	{
		$$=new CNestedDataFile::CVariant("",$1);
		free($1);
	}
	| TRUE
	{
		$$=new CNestedDataFile::CVariant("","true");
	}
	| FALSE
	{
		$$=new CNestedDataFile::CVariant("","false");
	}

	// symbol lookups
	| qualified_ident
	{
		CNestedDataFile::CVariant *value;
		if(!CNestedDataFile::parseTree->findVariantNode(value,$1,0,false,CNestedDataFile::parseTree->root))
		{
			cfg_error(@1,("symbol not found: '"+string($1)+"'").c_str());
			value=new CNestedDataFile::CVariant("",0.0);
		}
			
		switch(value->type)
		{
		case CNestedDataFile::ktString:
			$$=new CNestedDataFile::CVariant("",value->stringValue);
			break;
		case CNestedDataFile::ktFloat:
			$$=new CNestedDataFile::CVariant("",value->floatValue);
			break;
		case CNestedDataFile::ktScope:
			cfg_error(@1,("symbol resolves to a scope: '"+string($1)+"'").c_str());
			value=new CNestedDataFile::CVariant("",0.0);
			break;
		default:
			throw(runtime_error("cfg_parse -- unhandled variant type"));

		}

		free($1);
	}

	// parenthesis
	| '(' expr ')'
	{
		$$=$2;
	}


	// ERROR CASES
	| '(' error ')'
	{
		yyclearin;
		$$=new CNestedDataFile::CVariant("",0.0);
	}
	;


unary_expr
	: primary_expr { $$=$1; }
	
	| unary_op unary_expr
	{
		if($2->type!=CNestedDataFile::ktFloat)
			cfg_error(@2,"invalid operand");
		$$=$2;
		if($1=='+')
			$$->floatValue=+$$->floatValue;
		else if($1=='-')
			$$->floatValue=-$$->floatValue;
		else
			throw(runtime_error("cfg_parse -- internal error parsing unary_expr"));
	}
	;

unary_op
	: '+' { $$='+'; }
	| '-' { $$='-'; }
	;

multiplicative_expr
	: unary_expr { $$=$1; }
	| multiplicative_expr '*' unary_expr { BINARY_EXPR($$,@1,$1,@3,$3,*) }
	| multiplicative_expr '/' unary_expr { BINARY_EXPR($$,@1,$1,@3,$3,/) }
	| multiplicative_expr '%' unary_expr { VERIFY_TYPE(@1,$1) VERIFY_TYPE(@3,$3) $$=$1; $$->floatValue=fmod($$->floatValue,$3->floatValue); delete $3; }
	;


additive_expr
	: multiplicative_expr { $$=$1; }
	| additive_expr '+' multiplicative_expr 
	{ 
		if($1->type==CNestedDataFile::ktFloat)
		{
			BINARY_EXPR($$,@1,$1,@3,$3,+) 
		}
		else if($1->type==CNestedDataFile::ktString)
		{
			if($3->type!=CNestedDataFile::ktString)
				yyerror(@3,"string type expected");
			$$=$1;
			$$->stringValue+=$3->stringValue;
			delete $3;
		}
		else
		{
			yyerror(@1,"invalid type");
			$$=$1;
			delete $3;
		}
	}
	| additive_expr '-' multiplicative_expr { BINARY_EXPR($$,@1,$1,@3,$3,-) }
	;


relational_expr
	: additive_expr { $$=$1; }
	| relational_expr LE additive_expr { BINARY_EXPR($$,@1,$1,@3,$3,<=) }
	| relational_expr GE additive_expr { BINARY_EXPR($$,@1,$1,@3,$3,>=) }
	| relational_expr '<' additive_expr { BINARY_EXPR($$,@1,$1,@3,$3,<) }
	| relational_expr '>' additive_expr { BINARY_EXPR($$,@1,$1,@3,$3,>) }
	;

equality_expr
	: relational_expr { $$=$1; }
	| equality_expr EQ relational_expr { BINARY_EXPR($$,@1,$1,@3,$3,==) }
	| equality_expr NE relational_expr { BINARY_EXPR($$,@1,$1,@3,$3,!=) }
	;
	
logical_and_expr
	: equality_expr { $$=$1; }
	| logical_and_expr AND equality_expr { BINARY_EXPR($$,@1,$1,@3,$3,&&) }
	;

logical_or_expr
	: logical_and_expr { $$=$1; }
	| logical_or_expr OR logical_and_expr { BINARY_EXPR($$,@1,$1,@3,$3,||) }
	;


conditional_expr
	: logical_or_expr { $$=$1; }
	| logical_or_expr '?' expr ':' conditional_expr { VERIFY_TYPE(@1,$1) $$=($1->floatValue ? ((delete $5),$3) : ((delete $3),$5)); delete $1; }
	;


expr
	: conditional_expr { $$=$1; }
	;



qualified_ident
	: IDENT
	{
		$$=$1;
	}
	| qualified_ident '|' IDENT
	{
		$$=(char *)realloc($$,strlen($$)+1+strlen($3)+1);
		strcat($$,CNestedDataFile::delimChar);
		strcat($$,$3);

		free($3);
	}
	;



%%

#include "cfg.lex.c"

void cfg_error(int line,const char *msg)
{
	fprintf(stderr,"%s:%d: %s\n",currentFilename.c_str(),line,msg);
	
	myyynerrs++;
	if(myyynerrs>25)
		throw(runtime_error("cfg_error -- more than 25 errors; bailing"));
}

void cfg_error(const RTokenPosition &pos,const char *msg)
{
	int line=pos.first_line;
	
	cfg_error(line,msg);
}

void cfg_error(const char *msg)
{
	cfg_error(yylloc.first_line,msg);
}

void checkForDupMember(int line,const char *key)
{
	if(CNestedDataFile::parseTree->keyExists((getCurrentScope()+key).c_str()))
		cfg_error(line,("duplicate scope name: "+(getCurrentScope()+string(key))).c_str());
}

static const string getCurrentScope()
{
	stack<string> copy=scopeStack;
	string s;
	while(!copy.empty())
	{
		s=copy.top()+s;
		s=CNestedDataFile::delimChar+s;

		copy.pop();
	}

	s+=CNestedDataFile::delimChar;

	s.erase(0,1);

	return(s);
}

