// This file contains definitions for functions that manipulate data

#include "Macros.h"
#include "Core_structs.h"

void concat_errors(err *master, err *addition)
{
    //Concatenates errors from 1 pin to the list of errors of the cable
    //master - the list of errors of the cable
    //addition - the list of errors of the current (input)pin/wire
    int r = master->err_count;
    master->err_count += addition->err_count;

    int b = 0;
    for(int a = r; a < master->err_count; a++)
    {
        master->err_list[a][0] = addition->err_list[b][0];
        master->err_list[a][1] = addition->err_list[b][1];
        master->err_list[a][2] = addition->err_list[b][2];
        b++;
    }
}
