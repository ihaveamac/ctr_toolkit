#include "lib.h"
#include "ncsd.h"

int NCSDProcess(ROM_CONTEXT *ctx)
{
	ctx->ncsd_struct = malloc(sizeof(NCSD_STRUCT));
	if(GetNCSDData(ctx) != 0)
		return Fail;
		
	u8 IsNotSuperTrim = (ctx->ncsd_struct->actual_rom_file_size != ctx->ncsd_struct->partition_data[7].offset && ctx->ncsd_struct->partition_data[7].active == True);
	u8 IsNotTrim = (ctx->ncsd_struct->actual_rom_file_size != ctx->ncsd_struct->used_rom_size);
	u8 IsNotFullSize = (ctx->ncsd_struct->actual_rom_file_size != ctx->ncsd_struct->rom_size);	
		
	if(IsNotSuperTrim && IsNotTrim && IsNotFullSize){
		printf("[!] ROM is malformed\n");
		return Fail;
	}
		
	if(ctx->flags[extract] == True){
		if(ExtractROMPartitions(ctx) != 0){
			return Fail;
		}
	}	
	if(ctx->flags[trim] == True){
		if(TrimROM(ctx) != 0)
			return Fail;
	}
	if(ctx->flags[restore] == True){
		if(RestoreROM(ctx) != 0)
			return Fail;
	}
	return 0;
}

int TrimROM(ROM_CONTEXT *ctx)
{
	printf("[+] Trimming ROM\n");
	u64 trim_size = ctx->ncsd_struct->used_rom_size;
	if(ctx->flags[supertrim] == True && ctx->ncsd_struct->partition_data[7].active == True)trim_size = ctx->ncsd_struct->partition_data[7].offset;
	if(TruncateFile_u64(ctx->romfile.argument,trim_size) != 0){
		printf("[!] Failed to trim ROM\n");
		return Fail;
	}
	return 0;
}

int RestoreROM(ROM_CONTEXT *ctx)
{
	printf("[+] Restoring ROM\n");
	if(ctx->ncsd_struct->actual_rom_file_size < ctx->ncsd_struct->used_rom_size){
		printf("[!] ROM Is Super Trimmed, it cannot be restored\n");
		return Fail;
	}
	if(TruncateFile_u64(ctx->romfile.argument,ctx->ncsd_struct->rom_size) != 0){
		printf("[!] Failed to Restore ROM\n");
		return Fail;
	}
	FILE *rom = fopen(ctx->romfile.argument,"rb+");
	fseek_64(rom,ctx->ncsd_struct->used_rom_size,SEEK_SET);
	WriteDummyBytes(rom,0xff,(ctx->ncsd_struct->rom_size - ctx->ncsd_struct->used_rom_size));
	fclose(rom);
	return 0;
}

void WriteDummyBytes(FILE *file, u8 dummy_byte, u64 len)
{
	
	u8 dummy_bytes[16];
	memset(&dummy_bytes,dummy_byte,16);
	for(u64 i = 0; i < len; i += 16){
		fwrite(&dummy_bytes,16,1,file);
	}
}

int GetNCSDData(ROM_CONTEXT *ctx)
{
	FILE *rom = fopen(ctx->romfile.argument,"rb");
	if(ctx->ncsd_struct == NULL)
		return Fail;
	memset(ctx->ncsd_struct,0x0,sizeof(NCSD_STRUCT));
	
	ctx->ncsd_struct->actual_rom_file_size = GetFileSize_u64(ctx->romfile.argument);
	
	NCSD_HEADER header;
	CARD_INFO_HEADER card_info;
	DEV_CARD_INFO_HEADER dev_card_info;
	
	fseek(rom,0x0,SEEK_SET);
	fread(&ctx->ncsd_struct->signature,0x100,1,rom);
	fseek(rom,0x100,SEEK_SET);
	fread(&header,sizeof(NCSD_HEADER),1,rom);
	fseek(rom,0x200,SEEK_SET);
	fread(&card_info,sizeof(CARD_INFO_HEADER),1,rom);
	fseek(rom,0x1200,SEEK_SET);
	fread(&dev_card_info,sizeof(DEV_CARD_INFO_HEADER),1,rom);
	
	if(u8_to_u32(header.magic,BE) != NCSD_MAGIC){
		printf("[!] ROM File is corrupt\n");
		goto fail;
	}
	
	u32 media_size = ((header.partition_flags[6] + 1)*0x200);
	
	u64 ROM_SIZE_MEDIAS = u8_to_u32(header.rom_size,LITTLE_ENDIAN);
	ctx->ncsd_struct->rom_size = (ROM_SIZE_MEDIAS)*(media_size);
	u32 tmp = u8_to_u32(header.offsetsize_table[0].offset,LITTLE_ENDIAN);
	for(int i = 0; i < 8; i++){
		tmp += u8_to_u32(header.offsetsize_table[i].size,LITTLE_ENDIAN);
	}
	ctx->ncsd_struct->used_rom_size = tmp*media_size;
		
	
	for(int i = 0; i < 8; i++){
		ctx->ncsd_struct->partition_data[i].offset = u8_to_u32(header.offsetsize_table[i].offset,LITTLE_ENDIAN)*media_size;
		ctx->ncsd_struct->partition_data[i].size = u8_to_u32(header.offsetsize_table[i].size,LITTLE_ENDIAN)*media_size;
		if(ctx->ncsd_struct->partition_data[i].offset != 0 && ctx->ncsd_struct->partition_data[i].size != 0)
			ctx->ncsd_struct->partition_data[i].active = True;
		ctx->ncsd_struct->partition_data[i].title_id = u8_to_u64(header.partition_id_table[i],LITTLE_ENDIAN);
		ctx->ncsd_struct->partition_data[i].fs_type = header.partitions_fs_type[i];
		ctx->ncsd_struct->partition_data[i].crypto_type = header.partitions_crypto_type[i];
		
		u8 magic[4];
		fseek_64(rom,(ctx->ncsd_struct->partition_data[i].offset + 0x100),SEEK_SET);
		fread(&magic,4,1,rom);
		if(u8_to_u32(magic,BE) == NCCH_MAGIC){
			u8 flags[8];
			u8 flag_bool[8];
			fseek_64(rom,(ctx->ncsd_struct->partition_data[i].offset + 0x188),SEEK_SET);
			fread(&flags,8,1,rom);
			resolve_flag(flags[5],flag_bool);
			if(flag_bool[1] == False && flag_bool[0] == True){
				if(flag_bool[2] == False && flag_bool[3] == True)
					ctx->ncsd_struct->partition_data[i].content_type = CFA_Manual;
				else if(flag_bool[2] == True && flag_bool[3] == True)
					ctx->ncsd_struct->partition_data[i].content_type = CFA_DLPChild;
				else if(flag_bool[2] == True && flag_bool[3] == False)
					ctx->ncsd_struct->partition_data[i].content_type = CFA_Update;
				else
					ctx->ncsd_struct->partition_data[i].content_type = _unknown;
			}
			else if(flag_bool[1] == True)
				ctx->ncsd_struct->partition_data[i].content_type = CXI;
			else
				ctx->ncsd_struct->partition_data[i].content_type = _unknown;
			fseek_64(rom,(ctx->ncsd_struct->partition_data[i].offset + 0x150),SEEK_SET);
			fread(ctx->ncsd_struct->partition_data[i].product_code,16,1,rom);
		}
		else
			ctx->ncsd_struct->partition_data[i].content_type = _unknown;
	}
	
	u8 *zeros = malloc(0x30);
	memset(zeros,0,0x30);
	if(memcmp(&header.reserved,zeros,0x30) != 0)
		ctx->ncsd_struct->type = nand;
	else if(u8_to_u64(card_info.cver_title_id,LITTLE_ENDIAN) == 0){
		u8 stock_title_key[0x10] = {0x6E, 0xC7, 0x5F, 0xB2, 0xE2, 0xB4, 0x87, 0x46, 0x1E, 0xDD, 0xCB, 0xB8, 0x97, 0x11, 0x92, 0xBA};
		if(memcmp(dev_card_info.TitleKey,stock_title_key,0x10) == 0)
			ctx->ncsd_struct->type = dev_external_SDK;
		else
			ctx->ncsd_struct->type = dev_internal_SDK;
	}
	else
		ctx->ncsd_struct->type = retail;
	free(zeros);
		
	if(ctx->flags[info])
		PrintNCSDData(ctx->ncsd_struct,&header,&card_info,&dev_card_info);
	fclose(rom);
	return 0;
fail:
	fclose(rom);
	return Fail;

}

int ExtractROMPartitions(ROM_CONTEXT *ctx)
{
	FILE *rom = fopen(ctx->romfile.argument,"rb");
	if(rom == NULL)
		return Fail;
	for(int i = 0; i < 8; i++){
		if(ctx->ncsd_struct->partition_data[i].active == True && ctx->ncsd_struct->partition_data[i].offset < ctx->ncsd_struct->actual_rom_file_size){
			char output[1024];
			memset(&output,0,1024);
			switch(ctx->ncsd_struct->partition_data[i].content_type){
				case CXI : snprintf(output,1024,"%s_%d_%s_APPDATA.cxi",ctx->outfile.argument,i,ctx->ncsd_struct->partition_data[i].product_code); break;
				case CFA_Manual : snprintf(output,1024,"%s_%d_%s_MANUAL.cfa",ctx->outfile.argument,i,ctx->ncsd_struct->partition_data[i].product_code); break;
				case CFA_DLPChild : snprintf(output,1024,"%s_%d_%s_DLP.cfa",ctx->outfile.argument,i,ctx->ncsd_struct->partition_data[i].product_code); break;
				case CFA_Update : snprintf(output,1024,"%s_%d_%s_UPDATEDATA.cfa",ctx->outfile.argument,i,ctx->ncsd_struct->partition_data[i].product_code); break;
				default: snprintf(output,1024,"%s_%d.bin",ctx->outfile.argument,i); break;
			}
			FILE *out = fopen(output,"wb");
			if(out == NULL){
				fclose(rom);
				return Fail;
			}
			printf("[+] Writing '%s'\n",output);
			if(ExportFileToFile(rom,out,ctx->ncsd_struct->partition_data[i].size,ctx->ncsd_struct->partition_data[i].offset,0) != 0){
				fclose(rom);
				fclose(out);
				return Fail;
			}
			fclose(out);
		}
	}
	fclose(rom);
	return 0;
}

void PrintNCSDData(NCSD_STRUCT *ctx, NCSD_HEADER *header, CARD_INFO_HEADER *card_info, DEV_CARD_INFO_HEADER *dev_card_info)
{
	printf("[+] NCSD Header Info\n");
	memdump(stdout,"Signature:              ",ctx->signature,0x100);
	printf("Magic:                  NCSD\n");
	switch(ctx->type){
		case retail : 
			printf("NCSD Type:              Retail/Production\n"); 
			printf("CVer Title ID:          %016llx\n",u8_to_u64(card_info->cver_title_id,LITTLE_ENDIAN));
			printf("CVer Title Ver:         v%d\n",u8_to_u16(card_info->cver_title_version,LITTLE_ENDIAN));
			char FW_STRING[10];
			GetMin3DSFW((char*)FW_STRING,card_info);
			printf("Min 3DS Firm:           %s\n",FW_STRING);
			break;
		case dev_internal_SDK :
			printf("NCSD Type:              Debug/Development\n");
			printf("SDK Type:               Nintendo Internal SDK\n");
			memdump(stdout,"Title Key:              ",dev_card_info->TitleKey,0x10);
			break;
		case dev_external_SDK :
			printf("NCSD Type:              Debug/Development\n");
			printf("SDK Type:               Nintendo 3RD Party SDK\n");
			memdump(stdout,"Title Key:              ",dev_card_info->TitleKey,0x10);
			break;
		case nand :
			printf("NCSD Type:              CTR NAND Dump\n");
	}	
	//char full_size_string[100];
	//char actual_size_string[100];
	GetCHIPFullSize(/*(char*)full_size_string,*/ctx->rom_size,ctx->type);
	GetROMUsedSize(/*(char*)actual_size_string,*/ctx->used_rom_size,ctx->actual_rom_file_size,ctx->type);
	//printf("%s\n",full_size_string);
	//printf("%s\n",actual_size_string);
	
	if(ctx->type != nand){
		printf("NCSD Title ID:          %016llx\n",u8_to_u64(header->title_id,LITTLE_ENDIAN));
		memdump(stdout,"ExHeader Hash:          ",header->exheader_hash,0x20);
		printf("AddHeader Size:         0x%x\n",u8_to_u32(header->additional_header_size,LITTLE_ENDIAN));
	}
	else{
		printf("Sector 0 Offset:        0x%x\n",u8_to_u32(header->sector_zero_offset,LITTLE_ENDIAN));
	}
	memdump(stdout,"Flags:                  ",header->partition_flags,8);
	printf("\n");
	
	printf("[+] NCSD Partitions\n");
	int firm_count = 0;
	for(int i = 0; i < 8; i++){
		if(ctx->partition_data[i].active == True){
			printf("Partition %d\n",i);
			if(ctx->partition_data[i].content_type != _unknown){
				printf(" Title ID:              %016llx\n",ctx->partition_data[i].title_id);
				printf(" Product Code:          %s\n",ctx->partition_data[i].product_code);
				printf(" Content Type:          ");
				switch(ctx->partition_data[i].content_type){
					case _unknown : printf("Unknown\n"); break;
					case CXI : printf("Application\n"); break;
					case CFA_Manual : printf("Electronic Manual\n"); break;
					case CFA_DLPChild : printf("Download Play Child\n"); break;
					case CFA_Update : printf("Software Update Partition\n"); break;
				}
			}
			else if(ctx->type == nand){
				switch(ctx->partition_data[i].fs_type){
					case 0x4: printf(" Partition Name:        AGB_FIRM Savegame\n"); break;
					case 0x3: 
						printf(" Partition Name:        firm%d (FIRMWARE)\n",firm_count);
						firm_count++;
						break;
					case 0x1: printf(" Partition Name:        CTR-NAND\n");
				}
			}
			printf(" FS Type:               %x\n",ctx->partition_data[i].fs_type);
			printf(" Crypto Type:           %x\n",ctx->partition_data[i].crypto_type);
			printf(" Offset:                0x%x\n",ctx->partition_data[i].offset);
			printf(" Size:                  0x%x",ctx->partition_data[i].size);
			//if(ctx->partition_data[i].size > GB) printf(" (%d GB)\n",ctx->partition_data[i].size/GB);
			if(ctx->partition_data[i].size > MB) printf(" (%d MB)\n",ctx->partition_data[i].size/MB);
			else printf(" (%d KB)\n",ctx->partition_data[i].size/KB);
			printf("\n");
		}
	}
}

void GetCHIPFullSize(/*char *string,*/u64 size, int type)
{
	char string[100];
	u64 UnitSizesBytes[3] = {GB,MB,KB};
	u64 UnitSizesBits[3] = {GB/8,MB/8,KB/8};
	char Str_UnitSizesBytes[3][3] = {"GB","MB","KB"};
	char Str_UnitSizesBits[3][5] = {"Gbit","Mbit","Kbit"};
	char ChipName[2][20] = {"ROM Cart Size: ","NAND Chip Size:"};
	u8 ByteIndex = 0;
	u8 ChipIndex = 0;
	if(type == nand) ChipIndex = 1;
	while(size < UnitSizesBytes[ByteIndex]){
		if(ByteIndex == 2) break;
		ByteIndex++;
	}
	
	sprintf(string,"%s         %lld %s",ChipName[ChipIndex],(size/UnitSizesBytes[ByteIndex]),Str_UnitSizesBytes[ByteIndex]);
	if((size/UnitSizesBits[ByteIndex]) >= UnitSizesBytes[ByteIndex+1])
		ByteIndex--;
	sprintf(string,"%s (%lld %s)",string,(size/UnitSizesBits[ByteIndex]),Str_UnitSizesBits[ByteIndex]);

	printf("%s\n",string);
	return;
}
void GetROMUsedSize(/*char *string,*/u64 expected_size, u64 actual_size, int type)
{
	char string[100];
	u64 UnitSizesBytes[2] = {MB,KB};
	char Str_UnitSizesBytes[2][3] = {"MB","KB"};
	char ChipName[2][20] = {"ROM Used Size:  ","NAND Image Size:"};
	u8 ByteIndex = 0;
	u8 ChipIndex = 0;
	u8 SuperTrimmed = False;
	if(actual_size < expected_size) SuperTrimmed = True; 
	if(type == nand) ChipIndex = 1;
	if(actual_size < UnitSizesBytes[ByteIndex]) ByteIndex = 1;
	
	sprintf(string,"%s        %lld %s",ChipName[ChipIndex],(actual_size/UnitSizesBytes[ByteIndex]),Str_UnitSizesBytes[ByteIndex]);
	switch(SuperTrimmed){
		case True: sprintf(string,"%s (0x%llx bytes)(SuperTrimmed)",string,actual_size); break;
		default: sprintf(string,"%s (0x%llx bytes)",string,actual_size); break;
	}
	printf("%s\n",string);
	return;
}

void GetMin3DSFW(char *FW_STRING, CARD_INFO_HEADER *card_info)
{
	u8 MAJOR = 0;
	u8 MINOR = 0;
	u8 BUILD = 0;
	char REGION_CHAR = 'X';

	u16 CVer_ver = u8_to_u16(card_info->cver_title_version,LE);
	u32 CVer_UID = u8_to_u32(card_info->cver_title_id,LE);
		
	switch(CVer_UID){
		case EUR_ROM : REGION_CHAR = 'E'; break;
		case JPN_ROM : REGION_CHAR = 'J'; break;
		case USA_ROM : REGION_CHAR = 'U'; break;
		case CHN_ROM : REGION_CHAR = 'C'; break;
		case KOR_ROM : REGION_CHAR = 'K'; break;
		case TWN_ROM : REGION_CHAR = 'T'; break;
	}
	
	
	switch(CVer_ver){
		case 3088 : MAJOR = 3; MINOR = 0; BUILD = 0; break;
		default : MAJOR = CVer_ver/1024; MINOR = (CVer_ver - 1024*(CVer_ver/1024))/0x10; break;//This tends to work 98% of the time, use above for manual overides
	}
	sprintf(FW_STRING,"%d.%d.%d-X%c",MAJOR,MINOR,BUILD,REGION_CHAR);
}
