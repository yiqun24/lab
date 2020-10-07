#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_si(char *args){
        char* arg = strtok(NULL," ");
        int step;

        if(arg == NULL)
         {
            step = 1;
         }
         else
         {
           step = atoi(arg);
         }

         cpu_exec(step);
         return 0;
}
static int cmd_info(char *args){
         char* arg = strtok(NULL," ");
         
         if(strcmp(arg,"r") == 0)
        {
         printf("eax       0x%x         %d\n", cpu.eax,cpu.eax);
         printf("ecx       0x%x         %d\n", cpu.ecx,cpu.ecx);
         printf("edx       0x%x         %d\n", cpu.edx,cpu.edx);
         printf("ebx       0x%x         %d\n", cpu.ebx,cpu.ebx);
         printf("esp       0x%x         %d\n", cpu.esp,cpu.esp);
         printf("ebp       0x%x         %d\n", cpu.ebp,cpu.ebp);
         printf("esi       0x%x         %d\n", cpu.esi,cpu.esi);
         printf("edi       0x%x         %d\n", cpu.edi,cpu.edi);
        }
         return 0;
}

static int cmd_x(char *args)
{
     char* arg1 = strtok(NULL," ");
     char* arg2 = strtok(NULL," ");
     int length;
     swaddr_t address;
     if(arg2 == NULL)
     {
        length = 1;
        sscanf(arg1, "%x",&address);
     }
     else
     {
        length = atoi(arg1);
        sscanf(arg2, "%x",&address);
     }
      int i;
      printf("0x%x:   ",address);
      for(i = 0;i < length; i++)
     {
        printf("%02x " , swaddr_read(address+i,1));
     }
     printf("\n");
     return 0;
}

static int cmd_p(char *args)
{   
    bool success;
    uint32_t result = expr(args,&success);
    if(success)
     printf("0x%x     %d\n",result,result);
    else
     printf("Evaluate failure\n");
    return 0;
}
static int cmd_w(char *args)
{
   WP *p = new_wp();
   bool success;
   //char* !!!
   p->value = expr(args,&success);
   if(!success)
   assert(0);
   strcpy(p->exp,args);
   printf("watchpoint %d: %s\n",p->NO,p->exp);
   printf("the first value : %d\n",p->value);   
   return 0;
}
static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
        { "si", "Make the program executs [N] steps.It will output 20 steps at most at one time", cmd_si},
        { "info", "Output the value of all registers", cmd_info},
        { "x", "Output the data of [N] consecutive bytes in memory from the address you input", cmd_x},
        { "p", "Evaluate the expression", cmd_p},
        { "w", "Set watchpoint", cmd_w},
	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
