%{
#include <stdio.h>

#include <iostream>
#include <string>

#include "searchdata.h"

using namespace std;

int yylex(void);
void yyerror(char const *);
void logwhere(const char *);
class Expression;
static void qualify(Rcl::SearchDataClauseDist *, const string &);

string stemlang("english");

%}

%union {
    string *str;
    Rcl::SearchDataClauseSimple *cl;
    Rcl::SearchData *sd;
}

%type <cl> qualquote
%type <cl> fieldexpr
%type <cl> term
%type <sd> orchain
%type <sd> query

%left AND
%right OR

%token EQUALS
%token CONTAINS
%token SMALLEREQ
%token SMALLER
%token GREATEREQ
%token GREATER

%token <str> WORD
%token <str> QUOTED
%token <str> QUALIFIERS

%%

query: fieldexpr 
{
    Rcl::SearchData *sd = new Rcl::SearchData(Rcl::SCLT_AND, stemlang);
    sd->addClause($1);
    $$ = sd;
    cerr << "q: fieldexpr" << endl;
}
| query fieldexpr 
{
    cerr << "q: query fieldexpr" << endl;
    $1->addClause($2);
    $$ = $1;
}
| query AND fieldexpr
{
    cerr << "q: query AND fieldexpr" << endl;
    $1->addClause($3);
    $$ = $1;
}
| query AND orchain
{
    cerr << "q: query AND orchain";
    Rcl::SearchDataClauseSub *sub = 
        new Rcl::SearchDataClauseSub(RefCntr<Rcl::SearchData>($1));
    $1->addClause(sub);
    $$ = $1;
}
| query orchain
{
    cerr << "q: query orchain" << endl;
    Rcl::SearchDataClauseSub *sub = 
        new Rcl::SearchDataClauseSub(RefCntr<Rcl::SearchData>($1));
    $1->addClause(sub);
    $$ = $1;
}
| orchain
{
    cerr << "q: orchain" << endl;
    $$ = $1;
}
| '(' query ')' 
{
    cerr << "( query )" << endl;
    $$ = $2;
}
;

orchain: 
fieldexpr OR fieldexpr
{
    cerr << "orchain: fieldexpr[" << $1->gettext() << "] OR fieldexpr[" << 
        $3->gettext() << "]" << endl;
    Rcl::SearchData *sd = new Rcl::SearchData(Rcl::SCLT_OR, stemlang);
    sd->addClause($1);
    sd->addClause($3);
    $$ = sd;
}
| orchain OR fieldexpr
{
    cerr << "orchain: orchain OR fieldexpr[" << $3->gettext() << "]" << endl;
    $1->addClause($3);
    $$ = $1;
}
;

fieldexpr: term 
{
    //cerr << "simple fieldexpr: " << $1->gettext() << endl;
    $$ = $1;
}
| WORD EQUALS term 
{
    //cerr << *$1 << " = " << $3->gettext() << endl;
    $3->setfield(*$1);
    $3->setrel(Rcl::SearchDataClause::REL_EQUALS);
    $$ = $3;
}
| WORD CONTAINS term 
{
    //cerr << *$1 << " : " << $3->gettext() << endl;
    $3->setfield(*$1);
    $3->setrel(Rcl::SearchDataClause::REL_CONTAINS);
    $$ = $3;
}
| WORD SMALLER term 
{
    //cerr << *$1 << " < " << $3->gettext() << endl;
    $3->setfield(*$1);
    $3->setrel(Rcl::SearchDataClause::REL_LT);
    $$ = $3;
}
| WORD SMALLEREQ term 
{
    //cerr << *$1 << " <= " << $3->gettext() << endl;
    $3->setfield(*$1);
    $3->setrel(Rcl::SearchDataClause::REL_LTE);
    $$ = $3;
}
| WORD GREATER term 
{
    //cerr << *$1 << " > " << $3->gettext() << endl;
    $3->setfield(*$1);
    $3->setrel(Rcl::SearchDataClause::REL_GT);
    $$ = $3;
}
| WORD GREATEREQ term 
{
    //cerr << *$1 << " >= " << $3->gettext() << endl;
    $3->setfield(*$1);
    $3->setrel(Rcl::SearchDataClause::REL_GTE);
    $$ = $3;
}
| '-' fieldexpr 
{
    //cerr << "- fieldexpr[" << $2->gettext() << "]" << endl;
    $2->setexclude(true);
    $$ = $2;
}
;

term: WORD 
{
    //cerr << "term[" << *$1 << "]" << endl;
    $$ = new Rcl::SearchDataClauseSimple(Rcl::SCLT_AND, *$1);
}
| qualquote 
{
    $$ = $1;
}
;

qualquote: QUOTED 
{
    cerr << "QUOTED[" << *$1 << "]" << endl;
    $$ = new Rcl::SearchDataClauseDist(Rcl::SCLT_PHRASE, *$1, 0);
}
| QUOTED QUALIFIERS 
{
    cerr << "QUOTED[" << *$1 << "] QUALIFIERS[" << *$2 << "]" << endl;
    Rcl::SearchDataClauseDist *cl = 
        new Rcl::SearchDataClauseDist(Rcl::SCLT_PHRASE, *$1, 0);
    qualify(cl, *$2);
    $$ = cl;
}
;


%%

#include <ctype.h>
#include <stack>

void yyerror (char const *s)
{
    cerr << s << endl;
}

void logwhere(const char *s)
{
    cerr << s << endl;
}

// Look for int at index, skip and return new index found? value.
static unsigned int qualGetInt(const string& q, unsigned int cur, int *pval)
{
    unsigned int ncur = cur;
    if (cur < q.size() - 1) {
        char *endptr;
        int val = strtol(&q[cur + 1], &endptr, 10);
        if (endptr != &q[cur + 1]) {
            ncur += endptr - &q[cur + 1];
            *pval = val;
        }
    }
    return ncur;
}

static void qualify(Rcl::SearchDataClauseDist *cl, const string& quals)
{
    cerr << "qualify(" << cl << ", " << quals << ")" << endl;
    for (unsigned int i = 0; i < quals.length(); i++) {
        //fprintf(stderr, "qual char %c\n", quals[i]);
        switch (quals[i]) {
        case 'b': 
            cl->setWeight(10.0);
            break;
        case 'c': break;
        case 'C': 
            cl->addModifier(Rcl::SearchDataClause::SDCM_CASESENS);
            break;
        case 'd': break;
        case 'D':  
            cl->addModifier(Rcl::SearchDataClause::SDCM_DIACSENS);
            break;
        case 'e': 
            cl->addModifier(Rcl::SearchDataClause::SDCM_CASESENS);
            cl->addModifier(Rcl::SearchDataClause::SDCM_DIACSENS);
            cl->addModifier(Rcl::SearchDataClause::SDCM_NOSTEMMING);
            break;
        case 'l': 
            cl->addModifier(Rcl::SearchDataClause::SDCM_NOSTEMMING);
            break;
        case 'L': break;
        case 'o':  
        {
            int slack = 10;
            i = qualGetInt(quals, i, &slack);
            cl->setslack(slack);
            //cerr << "set slack " << cl->getslack() << " done" << endl;
        }
        break;
        case 'p': 
            cl->setTp(Rcl::SCLT_NEAR);
            if (cl->getslack() == 0) {
                cl->setslack(10);
                //cerr << "set slack " << cl->getslack() << " done" << endl;
            }
            break;
        case '.':case '0':case '1':case '2':case '3':case '4':
        case '5':case '6':case '7':case '8':case '9':
        {
            int n = 0;
            float factor = 1.0;
            if (sscanf(&(quals[i]), "%f %n", &factor, &n)) {
                if (factor != 1.0) {
                    cl->setWeight(factor);
                }
            }
            if (n > 0)
                i += n - 1;
        }
        default:
            break;
        }
    }
}


static stack<int> returns;
static string input;
static unsigned int index;

int GETCHAR()
{
    if (!returns.empty()) {
        int c = returns.top();
        returns.pop();
        return c;
    }
    if (index < input.size())
        return input[index++];
    return 0;
}
static void UNGETCHAR(int c)
{
    returns.push(c);
}

// Simpler to let the quoted string reader store qualifiers in there,
// because their nature is determined by the absence of white space
// after the closing dquote. e.g "some term"abc. We could avoid this
// by making white space a token
static string qualifiers;

// specialstartchars are special only at the beginning of a token
// (e.g. doctor-who is a term, not 2 terms separated by '-')
static string specialstartchars("-");
// specialinchars are special everywhere except inside a quoted string
static string specialinchars(":=<>()");
static string whites(" \t\n\r");

// Called with the first dquote already read
static int parseString()
{
    string* value = new string();
    qualifiers.clear();
    int c;
    while ((c = GETCHAR())) {
        switch (c) {
        case '\\':
            /* Escape: get next char */
            c = GETCHAR();
            if (c == 0) {
                value->push_back(c);
                goto out;
            }
            value->push_back(c);
            break;
        case '"':
            /* End of string. Look for qualifiers */
            while ((c = GETCHAR()) && whites.find_first_of(c) == string::npos)
                qualifiers.push_back(c);
            goto out;
        default:
            value->push_back(c);
        }
    }
out:
    //cerr << "GOT QUOTED ["<<value<<"] quals [" << qualifiers << "]" << endl;
    yylval.str = value;
    return QUOTED;
}


int yylex(void)
{
//    cerr << "yylex: input [" << input.substr(index) << "]" << endl;

    if (!qualifiers.empty()) {
        yylval.str = new string();
        yylval.str->swap(qualifiers);
        return QUALIFIERS;
    }

    int c;

    /* Skip white space.  */
    while ((c = GETCHAR ()) && whites.find_first_of(c) != string::npos)
        continue;

    if (c == 0)
        return 0;

    if (specialstartchars.find_first_of(c) != string::npos) {
        //cerr << "yylex: return " << c << endl;
        return c;
    }

    // field-term relations
    switch (c) {
    case '=': return EQUALS;
    case ':': return CONTAINS;
    case '<': {
        int c1 = GETCHAR();
        if (c1 == '=') {
            return SMALLEREQ;
        } else {
            UNGETCHAR(c);
            return SMALLER;
        }
    }
    case '>': {
        int c1 = GETCHAR();
        if (c1 == '=') {
            return GREATEREQ;
        } else {
            UNGETCHAR(c);
            return GREATER;
        }
    }
    case '(': case ')':
        return c;
    }
        
    if (c == '"')
        return parseString();

    UNGETCHAR(c);

    // Other chars start a term or field name or reserved word
    string* word = new string();
    while ((c = GETCHAR())) {
        if (whites.find_first_of(c) != string::npos) {
            //cerr << "Word broken by whitespace" << endl;
            break;
        } else if (specialinchars.find_first_of(c) != string::npos) {
            //cerr << "Word broken by special char" << endl;
            UNGETCHAR(c);
            break;
        } else if (c == 0) {
            //cerr << "Word broken by EOF" << endl;
            break;
        } else {
            word->push_back(c);
        }
    }
    
    if (!word->compare("AND") || !word->compare("&&")) {
        delete word;
        return AND;
    } else if (!word->compare("OR") || !word->compare("||")) {
        delete word;
        return OR;
    }

//    cerr << "Got word [" << word << "]" << endl;
    yylval.str = word;
    return WORD;
}

int main (int argc, const char *argv[])
{
    argc--;argv++;
    if (argc == 0)
        return 1;
    while (argc--) {
        input += *argv++;
        input += " ";
    }

    index = 0;
    returns = stack<int>();

    return yyparse();
}
