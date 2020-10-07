#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

#define max_length 32
enum
{
  NOTYPE = 256,
  DECIMAL,
  HEX,
  REGISTER,
  EQ,
  NEQ,
  AND,
  OR,
  POINTER,
  MINUS
  /* TODO: Add more token types */

};

static struct rule
{
  char *regex;
  int token_type;
  int priority;
} rules[] = {

    /* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

    {"\\b[0-9]+\\b", DECIMAL, 0},         // decimal integer
    {"\\b0(x|X)[0-9a-fA-F]+\\b", HEX, 0}, //hexadecimal integer
    {"\\$[a-zA-Z]{2,3}", REGISTER, 0},    //register
    {" +", NOTYPE, 0},                    // spaces
    {"\\+", '+', 4},                      // plus
    {"\\*", '*', 5},                      // multiply
    {"-", '-', 4},
    {"/", '/', 5},
    {"\\(", '(', 7},
    {"\\)", ')', 7},
    {"==", EQ, 3},  // equal
    {"!=", NEQ, 3}, // not equal
    {"&&", AND, 2},
    {"\\|\\|", OR, 1},
    {"!", '!', 6}};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[max_length];
  int priority;
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

        switch (rules[i].token_type)
        {
        case NOTYPE:
          break;

        case REGISTER:
          tokens[nr_token].type = rules[i].token_type;
          tokens[nr_token].priority = rules[i].priority;
          strncpy(tokens[nr_token].str, substr_start + 1, substr_len - 1);
          tokens[nr_token].str[substr_len - 1] = '\0';
          nr_token++;
          break;
        default:
          tokens[nr_token].type = rules[i].token_type;
          tokens[nr_token].priority = rules[i].priority;
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0'; // add terminator
          nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parenthese(int p, int q)
{
  if (tokens[p].type != '(' || tokens[q].type != ')')
    return false;

  int n1 = 0, n2 = 0;

  for (; p < q; p++, q--)
  {
    if (tokens[p].type == ')' || tokens[q].type == '(')
      return false;
    if (tokens[p].type == '(')
      n1++;
    if (tokens[q].type == ')')
      n2++;
  }
  if (n1 == n2)
    return true;
  else
    return false;
}

int dominant_operator(int p, int q)
{
  int min_priority = 10;
  int op = p;
  int i;
  int cnt = 0;
  for (i = p; i <= q; i++)
  {
    if (tokens[i].priority == 0)
      continue;
    if (tokens[i].type == '(')
    {
      cnt++;
      continue;
    }
    if (tokens[i].type == ')')
    {
      cnt--;
      continue;
    }
    if (cnt != 0)
      continue;
    if (tokens[i].priority <= min_priority)
    {
      min_priority = tokens[i].priority;
      op = i;
    }
  }
  return op;
}
uint32_t eval(int p, int q)
{
  if (p > q)
    assert(0);

  else if (p == q)
  {
    uint32_t result = 0;
    if (tokens[p].type == DECIMAL)
      sscanf(tokens[p].str, "%d", &result);
    else if (tokens[p].type == HEX)
      sscanf(tokens[p].str, "%x", &result);
    else if (tokens[p].type == REGISTER)
    {
      int i;
      if (strlen(tokens[p].str) == 3)
      {
        for (i = R_EAX; i <= R_EDI; i++)
        {
          if (strcmp(tokens[p].str, regsl[i]) == 0)
          {
            result = reg_l(i);
            break;
          }
          if (i == R_EDI)
          {
            if (strcmp(tokens[p].str, "eip") == 0)
              result = cpu.eip;
            else
              assert(0);
          }
        }
      }
      if (strlen(tokens[p].str) == 2)
      {
        if (tokens[p].str[1] == 'x' || tokens[p].str[1] == 'p' || tokens[p].str[1] == 'i')
        {
          for (i = R_AX; i <= R_DI; i++)
            if (strcmp(tokens[p].str, regsw[i]) == 0)
            {
              result = reg_w(i);
              break;
            }
        }
        else if (tokens[p].str[1] == 'l' || tokens[p].str[1] == 'h')
        {
          for (i = R_AL; i <= R_BH; i++)
            if (strcmp(tokens[p].str, regsb[i]) == 0)
            {
              result = reg_b(i);
              break;
            }
        }
      }
    }
    return result;
  }
  else if (check_parenthese(p, q) == true)
    return eval(p + 1, q - 1);
  else
  {
    int op = dominant_operator(p, q);
    if (tokens[op].type == POINTER || tokens[op].type == MINUS || tokens[op].type == '!' || p == op)
    {
      uint32_t val = eval(p + 1, q); //?
      switch (tokens[p].type)        //?
      {
      case POINTER:
        return swaddr_read(val, 4); //?
      case MINUS:
        return -val;
      case '!':
        return !val;
      default:
        assert(0);
      }
    }
    uint32_t val1 = eval(p, op - 1);
    uint32_t val2 = eval(op + 1, q);

    switch (tokens[op].type)
    {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      return val1 / val2;
    case AND:
      return val1 && val2;
    case OR:
      return val1 || val2;
    case EQ:
      return val1 == val2;
    case NEQ:
      return val1 != val2;
    default:
      assert(0);
    }
  }
}
uint32_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }
  int i;
  for (i = 0; i < nr_token; i++)
  {
    if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != DECIMAL && tokens[i - 1].type != HEX && tokens[i - 1].type != REGISTER && tokens[i - 1].type != ')')))
    {
      tokens[i].type = POINTER;
      tokens[i].priority = 6;
    }
    if (tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != DECIMAL && tokens[i - 1].type != HEX && tokens[i - 1].type != REGISTER && tokens[i - 1].type != ')')))
    {
      tokens[i].type = MINUS;
      tokens[i].priority = 6;
    }
  }
  /* TODO: Insert codes to evaluate the expression. */
  *success = true;
  return eval(0, nr_token - 1);
}
