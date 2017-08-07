#include "nemu.h"
#include <stdlib.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
int cmd_info_one(char *args);
uint32_t swaddr_read(swaddr_t addr,size_t len);

enum {
	NOTYPE = 256, NUMBER, VARIABLE, EQ, NQ, AND, OR, NOT, PLUS, MINUS, POWER, DIVIDE, OPENBAR, CLOSEBAR, UNARYMINUS, DEREF

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},				// spaces
	{"^[0-9]+|0x[a-f0-9]+", NUMBER},		    // number
	{"\\$[a-z]+",VARIABLE},         // variable
	{"==", EQ},	                    // equal
	{"!=", NQ},                     // not equal
	{"&&", AND},                    // and
	{"\\|\\|", OR},                 // or
	{"\\!",NOT},                    // not	
	{"\\+", PLUS},					// plus	
	{"\\-", MINUS},                 // minus
	{"\\*", POWER},                 // power
	{"\\/", DIVIDE},		        // divide
	{"\\(", OPENBAR},               // (
	{"\\)", CLOSEBAR}               // )
 }; 

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					
					case VARIABLE:
					case NUMBER:
					    if(substr_len>32)  assert(0);
						tokens[nr_token].type = rules[i].token_type;
						for(int i=0;i<substr_len;i++)
						{
							tokens[nr_token].str[i]=substr_start[i];
						}
						tokens[nr_token].str[i]='\0';
						nr_token++;
						break;
					case OR:
					    tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].str[0]='0'; // youxianji
						nr_token++;
						break;
					case AND:
					    tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].str[0]='1';
						nr_token++;
						break;
					case EQ:
					case NQ:
					    tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].str[0]='2';
						nr_token++;
						break;
					case PLUS:
					case MINUS:
						tokens[nr_token].type = rules[i].token_type;
						tokens[nr_token].str[0]='3';
						nr_token++;
						break;
					case POWER:
					case DIVIDE:
					    tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].str[0]='4';
						nr_token++;
						break;
					case NOT:
					case DEREF:
					case UNARYMINUS:
					    tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].str[0]='5';
						nr_token++;
						break;
					case OPENBAR:
					case CLOSEBAR:
					    tokens[nr_token].type=rules[i].token_type;
						nr_token++;
						break;
					
					default: panic("please implement me");
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

static int stack[1000];
static int stack_length;

static void init_stack()
{
    stack_length=0;
}

static void push(int value)
{
	if(stack_length>1000) return;	
	else
	{
        stack[stack_length]=value;
		stack_length++;
	}
}

static void pop()
{
    if(stack_length<=0) return;
	else 
	{
		stack_length--;
	}	
}

static bool check_bar()
{
	init_stack();
    for(int i=0;i<nr_token;i++)
	{
		if(tokens[i].type==OPENBAR)
		{
			push(tokens[i].type);
		}
		if(tokens[i].type==CLOSEBAR)
		{
			if(stack_length<=0) return false;
			pop();
		}
	}
	if(stack_length!=0) return false;
	return true;
}

static bool check_parentheses(int start,int end)
{
	init_stack();
    if(tokens[start].type!=OPENBAR || tokens[end].type!=CLOSEBAR) return false;
	else
	{
		for(int i=start;i<=end;i++)
		{
			if(tokens[i].type==OPENBAR)
			{
				if(stack_length>=1000) assert(0);
				else push(tokens[i].type);
			}
			if(tokens[i].type==CLOSEBAR)
			{
				if(stack_length==1&& i!=end) return false;
				else pop();
			}
		}
		return true;
	}
}

static bool is_op(int type)
{
	if(type==NUMBER || type==VARIABLE || type==OPENBAR ||type ==CLOSEBAR ||type== NOTYPE) return false;
	return true;
}

static int dominate_op(int start, int end)
{
	int m=8,n=0;
    for(int i=start;i<=end;i++)
	{
        if(tokens[i].type==OPENBAR)
		{
			while(tokens[i].type!=CLOSEBAR)
			i++;
		}
		if(is_op(tokens[i].type) && tokens[i].str[0]-'0'<=m)
		{
			n=i;
			m=tokens[i].str[0]-'0';
		}
	}
	return n;
}

static int str16to10(char *args)
{
	int sum=0;
	for(int i=2;i<strlen(args);i++)
	{
		if(args[i]>='0' && args[i]<=9)
		sum=sum*16+(args[i]-'0');
		if(args[i]>='a' && args[i]<='f')
		sum=sum*16+(args[i]-'a'+10);
	}
	return sum;
}

static int eval(int start, int end)
{
	if(start>end) assert(0);

	else if(start==end)
	{
		if(tokens[start].str[0]=='$')
		{
            return cmd_info_one(tokens[start].str);
		}
		else if(tokens[start].str[0]=='0' && tokens[start].str[1]=='x')
		{
			return str16to10(tokens[start].str);
		}
		return atoi(tokens[start].str);
	}

	else if(check_parentheses(start,end)==true)
	{
        return eval(start+1,end-1);
	}
	else
	{
        int op=dominate_op(start,end);
		if(tokens[op].type==NOT || tokens[op].type==DEREF || tokens[op].type==UNARYMINUS)
		{
			int r=eval(op+1,end);
			if(tokens[op].type==NOT) 
			    return !r;
			else if(tokens[op].type==UNARYMINUS) 
			    return -r;
			else 
			    return swaddr_read(r,1);
		}
		int val1=eval(start, op-1);
		int val2=eval(op+1,end);
		switch (tokens[op].type)
		{
			case OR: return val1||val2;
			case AND: return val1&&val2;
			case EQ: return val1==val2;
			case NQ: return val1!=val2;
			case PLUS: return val1+val2;
			case MINUS: return val1-val2;
			case POWER: return val1*val2;
			case DIVIDE:
			    if(val2==0) assert(0);
				else return val1/val2;
			default:assert(0);
		}
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}
    
	if(!check_bar()){
		*success = false;
		return 0;
	}
	/* TODO: Insert codes to evaluate the expression. */
    for(int i=0;i<nr_token;i++)
	{
		if(tokens[i].type==POWER && (i==0 || is_op(tokens[i].type)))
		   tokens[i].type=DEREF;
		if(tokens[i].type==MINUS && (i==0 || is_op(tokens[i].type)))
		   tokens[i].type=UNARYMINUS;
	}
	//panic("please implement me");
	//return 0;
	return eval(0,nr_token-1);
}

