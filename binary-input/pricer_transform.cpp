// pricer_transform.cpp : Defines the entry point for the console application.
//
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string.h>
#include <stdint.h>
#include "pricer_types.h"

int main(int argc, char* argv[])
{
    if(argc<3)
    {
        std::cerr << "usage: "<<argv[0]<<" pricer.in pricer.in.bin\n"
                    "pricer.in - filename of input file in pricer.in format\n"
                    "pricer.in.bin - filename of outputfile for pricer2 program\n";
        return -1;
    }

	FILE * f = fopen(argv[2], "wb");
	if(NULL == f)
	{
		std::cerr << "can't open output file "<<argv[2]<<" for writing\n";
		return -1;
	}

	std::string input_line;
	message_t m;

	std::fstream in(argv[1],std::ios_base::in);
    if(!in.good())
    {
		std::cerr << "can't open input file "<<argv[1]<<" for reading\n";
		return -1;
    }
	
	while(in.good())
	{
        std::string     id_str;

		std::getline(in, input_line);

		if(input_line.empty()) { break; }

		std::stringstream is(input_line);

		memset(&m, 0, sizeof(m));
        is >> m.timestamp >> m.msg_type >> id_str;
    
		assert(id_str.size()<=8);
        memcpy(&m.id, id_str.c_str(), id_str.size());
    
        if('A' == m.msg_type)
        {
            is >> m.side >> m.price >> m.size;
        }
        else if('R'== m.msg_type)
        {
            is >> m.size;
        }
		fwrite(&m, sizeof(m), 1, f);
	}
	fclose(f);
	return 0;
}

