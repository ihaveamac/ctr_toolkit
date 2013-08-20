#include "lib.h"
#include "main.h"
#include "ncsd.h"

typedef enum
{
	MAJOR = 2,
	MINOR = 6
} AppVer;

void app_title(void);
void help(char *app_name);

int main(int argc, char *argv[])
{	
	//Filter Out Bad number of arguments
	if (argc < 2){
		printf("[!] Must Specify Arguments\n");
		help(argv[0]);
		return ARGC_FAIL;
	}
	
	ROM_CONTEXT *ctx = malloc(sizeof(ROM_CONTEXT));
	memset(ctx,0x0,sizeof(ROM_CONTEXT));
		
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
			help(argv[0]);
			free_buffers(ctx);
			return ARGC_FAIL;
		}
		else if(i == argc-1){
			ctx->romfile.arg_len = strlen(argv[i]);
			ctx->romfile.argument = malloc(ctx->romfile.arg_len+1);
			if(ctx->romfile.argument == NULL){
				printf("[!] MEM ERROR\n");
				return Fail;
			}
			memcpy(ctx->romfile.argument,argv[i],ctx->romfile.arg_len+1);
			FILE *rom = fopen(ctx->romfile.argument,"rb");
			if(rom == NULL){
				printf("[!] Failed to open '%s'\n",ctx->romfile.argument);
				return 1;
			}
			fclose(rom);
		}
		else if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--info") == 0)
			ctx->flags[info] = True;
		else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--restore") == 0)
			ctx->flags[restore] = True;
		else if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trim") == 0)
			ctx->flags[trim] = True;
		else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--supertrim") == 0){
			ctx->flags[trim] = True;
			ctx->flags[supertrim] = True;
		}
		else if(strcmp(argv[i], "-x") == 0 && ctx->flags[extract] == False && i < (argc - 1)){
			ctx->flags[extract] = True;
			ctx->outfile.arg_len = strlen(argv[i+1]);
			ctx->outfile.argument = malloc(ctx->outfile.arg_len);
			if(ctx->outfile.argument == NULL){
				printf("[!] MEM ERROR\n");
				return Fail;
			}
			memcpy(ctx->outfile.argument,argv[i+1],ctx->outfile.arg_len+1);
		}
		else if(strncmp(argv[i], "--extract=",10) == 0 && ctx->flags[extract] == False){
			ctx->flags[extract] = True;
			ctx->outfile.arg_len = strlen(argv[i]+10);
			ctx->outfile.argument = malloc(ctx->outfile.arg_len);
			if(ctx->outfile.argument == NULL){
				printf("[!] MEM ERROR\n");
				return Fail;
			}
			memcpy(ctx->outfile.argument,argv[i]+10,ctx->outfile.arg_len+1);
		}
	}
	
	if(ctx->flags[restore] == True && ctx->flags[trim] == True){
		printf("[!] You cannot trim and restore a ROM at the same time\n");
		help(argv[0]);
		return 1;
	}
		
	if(NCSDProcess(ctx) != 0)
		goto fail_cleanup;
	
	printf("[*] Completed Successfully\n");
	free_buffers(ctx);
	return 0;
fail_cleanup:
	printf("[!] Failed\n");
	free_buffers(ctx);
	return 1;
}

void free_buffers(ROM_CONTEXT *ctx)
{	
	//Freeing Arguments
	if(ctx->romfile.arg_len > 0)
		free(ctx->romfile.argument);
		
	if(ctx->outfile.arg_len > 0)
		free(ctx->outfile.argument);
	
	//Freeing ROM Data buffers
	if(ctx->ncsd_struct_malloc_flag)
		free(ctx->ncsd_struct);
	
	//Freeing Main context
	free(ctx);
}

void app_title(void)
{
	printf("CTR_Toolkit - ROM TOOL\n");
	printf("Version %d.%d (C) 3DSGuy 2013\n",MAJOR,MINOR);
}

void help(char *app_name)
{
	app_title();
	printf("Usage: %s [options] <rom filepath>\n", app_name);
	printf("OPTIONS                 Explanation\n");
	printf(" -h, --help             Print this help.\n");
	printf(" -i, --info             Print 3DS ROM Info\n");
	printf(" -r, --restore          Restore(Un-Trim) 3DS ROM File.\n");
	printf(" -t, --trim             Trim 3DS ROM File.\n");
	printf(" -s, --supertrim        Same as '-t' but removes the update partition\n");
	printf(" -x, --extract=         Extract NCSD Partitions\n");
}