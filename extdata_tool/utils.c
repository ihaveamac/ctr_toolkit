/**
Copyright 2013 3DSGuy

This file is part of make_cdn_cia.

make_cdn_cia is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

make_cdn_cia is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with make_cdn_cia.  If not, see <http://www.gnu.org/licenses/>.
**/
#include "lib.h"

void char_to_int_array(unsigned char destination[], char source[], int size, int endianness, int base)
{	
	char tmp[size][2];
    unsigned char *byte_array = malloc(size*sizeof(unsigned char));
	memset(byte_array, 0, size);
	memset(destination, 0, size);
	memset(tmp, 0, size*2);
    
    for (int i = 0; i < size; i ++){
		tmp[i][0] = source[(i*2)];
        tmp[i][1] = source[((i*2)+1)];
		tmp[i][2] = '\0';
        byte_array[i] = (unsigned char)strtol(tmp[i], NULL, base);
    }
	for (int i = 0; i < size; i++){
        switch (endianness){
        	case(BIG_ENDIAN):
        	destination[i] = byte_array[i];
        	break;
        	case(LITTLE_ENDIAN):
        	destination[i] = byte_array[((size-1)-i)];
        	break;
        }
    }
	free(byte_array);
}

void print_product_code(u8 *product_code)
{
	for(int i = 0; i < 0x10; i++){
		if(product_code[i] == '\0')
			return;
		putchar(product_code[i]);
	}
}

int makedir(const char* dir)
{
#ifdef _WIN32
	return _mkdir(dir);
#else
	return mkdir(dir, 0777);
#endif
}

char *getcwdir(char *buffer,int maxlen)
{
#ifdef _WIN32
	return _getcwd(buffer,maxlen);
#else
	return getcwd(buffer,maxlen);
#endif
}

void resolve_flag(unsigned char flag, unsigned char *flag_bool)
{
	unsigned char bit_mask[8] = {0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};
	for(int i = 0; i < 8; i++){
		if (flag >= bit_mask[i]){
			flag_bool[7-i] = TRUE;
			flag -= bit_mask[i];
		}
		else
			flag_bool[7-i] = FALSE;
	}
}

void resolve_flag_u16(u16 flag, unsigned char *flag_bool)
{
	u16 bit_mask[16] = {0x8000,0x4000,0x2000,0x1000,0x800,0x400,0x200,0x100,0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};
	for(int i = 0; i < 16; i++){
		if (flag >= bit_mask[i]){
			flag_bool[15-i] = TRUE;
			flag -= bit_mask[i];
		}
		else
			flag_bool[15-i] = FALSE;
	}
}

void endian_strcpy(unsigned char destination[], unsigned char source[], int size, int endianness)
{ 
    for (int i = 0; i < size; i++){
        switch (endianness){
            case(BIG_ENDIAN):
                destination[i] = source[i];
                break;
            case(LITTLE_ENDIAN):
                destination[i] = source[((size-1)-i)];
                break;
        }
    }
}

void u8_hex_print_be(u8 *array, int len)
{
	for(int i = 0; i < len; i++)
		printf("%02x",array[i]);
}

void u8_hex_print_le(u8 *array, int len)
{
	for(int i = 0; i < len; i++)
		printf("%02x",array[len - i - 1]);
}

u32 align_value(u32 value, u32 alignment)
{
	u32 tmp = value;
	while(tmp > alignment)
		tmp -= alignment;
	return (value + (alignment - tmp));
}

u64 u8_to_u64(u8 *value, u8 endianness)
{
	u64 u64_return = 0;
	switch(endianness){
		case(BIG_ENDIAN): 
			u64_return |= (u64)value[7]<<0;
			u64_return |= (u64)value[6]<<8;
			u64_return |= (u64)value[5]<<16;
			u64_return |= (u64)value[4]<<24;
			u64_return |= (u64)value[3]<<32;
			u64_return |= (u64)value[2]<<40;
			u64_return |= (u64)value[1]<<48;
			u64_return |= (u64)value[0]<<56;
			break;
			//return (value[7]<<0) | (value[6]<<8) | (value[5]<<16) | (value[4]<<24) | (value[3]<<32) | (value[2]<<40) | (value[1]<<48) | (value[0]<<56);
		case(LITTLE_ENDIAN): 
			u64_return |= (u64)value[0]<<0;
			u64_return |= (u64)value[1]<<8;
			u64_return |= (u64)value[2]<<16;
			u64_return |= (u64)value[3]<<24;
			u64_return |= (u64)value[4]<<32;
			u64_return |= (u64)value[5]<<40;
			u64_return |= (u64)value[6]<<48;
			u64_return |= (u64)value[7]<<56;
			break;
			//return (value[0]<<0) | (value[1]<<8) | (value[2]<<16) | (value[3]<<24) | (value[4]<<32) | (value[5]<<40) | (value[6]<<48) | (value[7]<<56);
	}
	return u64_return;
}

void memdump(FILE* fout, const char* prefix, const u8* data, u32 size)
{
	u32 i;
	u32 prefixlen = strlen(prefix);
	u32 offs = 0;
	u32 line = 0;
	while(size)
	{
		u32 max = 32;

		if (max > size)
			max = size;

		if (line==0)
			fprintf(fout, "%s", prefix);
		else
			fprintf(fout, "%*s", prefixlen, "");


		for(i=0; i<max; i++)
			fprintf(fout, "%02X", data[offs+i]);
		fprintf(fout, "\n");
		line++;
		size -= max;
		offs += max;
	}
}

u32 u8_to_u32(u8 *value, u8 endianness)
{
	u32 new_value;
	switch(endianness){
		case(BIG_ENDIAN): new_value = (value[3]<<0) | (value[2]<<8) | (value[1]<<16) | (value[0]<<24); break;
		case(LITTLE_ENDIAN): new_value = (value[0]<<0) | (value[1]<<8) | (value[2]<<16) | (value[3]<<24); break;
	}
	return new_value;
}

u16 u8_to_u16(u8 *value, u8 endianness)
{
	u16 new_value;
	switch(endianness){
		case(BIG_ENDIAN): new_value =  (value[1]<<0) | (value[0]<<8); break;
		case(LITTLE_ENDIAN): new_value = (value[0]<<0) | (value[1]<<8); break;
	}
	return new_value;
}

int u16_to_u8(u8 *out_value, u16 in_value, u8 endianness)
{
	switch(endianness){
		case(BIG_ENDIAN):
			out_value[0]=(in_value >> 8);
			out_value[1]=(in_value >> 0);
			break;
		case(LITTLE_ENDIAN):
			out_value[0]=(in_value >> 0);
			out_value[1]=(in_value >> 8);
			break;
	}
	return 0;
}

int u32_to_u8(u8 *out_value, u32 in_value, u8 endianness)
{
	switch(endianness){
		case(BIG_ENDIAN):
			out_value[0]=(in_value >> 24);
			out_value[1]=(in_value >> 16);
			out_value[2]=(in_value >> 8);
			out_value[3]=(in_value >> 0);
			break;
		case(LITTLE_ENDIAN):
			out_value[0]=(in_value >> 0);
			out_value[1]=(in_value >> 8);
			out_value[2]=(in_value >> 16);
			out_value[3]=(in_value >> 24);
			break;
	}
	return 0;
}

int u64_to_u8(u8 *out_value, u64 in_value, u8 endianness)
{
	switch(endianness){
		case(BIG_ENDIAN):
			out_value[0]=(in_value >> 56);
			out_value[1]=(in_value >> 48);
			out_value[2]=(in_value >> 40);
			out_value[3]=(in_value >> 32);
			out_value[4]=(in_value >> 24);
			out_value[5]=(in_value >> 16);
			out_value[6]=(in_value >> 8);
			out_value[7]=(in_value >> 0);
			break;
		case(LITTLE_ENDIAN):
			out_value[0]=(in_value >> 0);
			out_value[1]=(in_value >> 8);
			out_value[2]=(in_value >> 16);
			out_value[3]=(in_value >> 24);
			out_value[4]=(in_value >> 32);
			out_value[5]=(in_value >> 40);
			out_value[6]=(in_value >> 48);
			out_value[7]=(in_value >> 56);
			break;
	}
	return 0;
}
