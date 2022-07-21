// Very first definition of a TLB
// Very low level implementation of a TLB.

#include <stdbool.h>

#define TLB_Lines 16
#define ADDRESS_P 8
#define ADDRESS_D 8
#define ADDRESS_F 8

int TLB_Translation(int logical_address);
int TLB_Add();
int TLB_Remove();

struct tlb_struct
{
    bool Valid;
    bool Modified;
    int Protection;
    int page_number;
    int frame_number;
} TLB_Data[TLB_Lines];

int
TLB_Translation(int logical_address) {
    int p_logical_address;
    int d_logical_address;

    int f_physical_address;
    int d_physical_address;
    int physical_address;

    bool tlb_line_found = false;
    int tlb_line = 0;

    for(int i = 0; i < TLB_Lines; i++) {
        if (TLB_Data[i].page_number == p_logical_address) {
            if (TLB_Data[i].Valid == 1) {                
                tlb_line_found = 1;
                tlb_line = i;
            } else {
                // TODO #2
            }
        } else continue; 
    }

    switch(tlb_line_found) {
        case 0:
            // TLB Miss
            // search the f value in the page_table
            // TODO #1
            break;
        case 1:
            // TLB Hit
            f_physical_address = TLB_Data[tlb_line].frame_number;
            break;
        default:
            // You should never be here
    }


}
