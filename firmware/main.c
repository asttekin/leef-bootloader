/*-----------------------------------------------------------------------------
/
/
/
/
/
/
/
/
/----------------------------------------------------------------------------*/
#include <stdio.h>
#include <inttypes.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "./hal/hal.h"

void (*funcptr)(void) = 0x0000;

/* SPM_PAGESIZE is 128 bytes */
static uint8_t pageBuf[SPM_PAGESIZE];

#define newMessage() (UCSR0A & (1<<RXC0))
#define getByte() (UDR0)

static void boot_program_page(uint32_t page, uint8_t *buf)
{
    uint16_t i;

    cli();
    eeprom_busy_wait();

    for(i=0;i<SPM_PAGESIZE;i+=2)
    {
        uint16_t w = *buf++;
        w += (*buf++) << 8;

        boot_page_fill(page+i,w);
    }

    boot_page_write(page);
    boot_spm_busy_wait();  

    boot_rww_enable();
}

int main(void) 
{    
    uint8_t t8;
    uint16_t i;
    uint8_t msg;
    uint32_t t32;
    uint8_t run = 1;
    uint32_t pageNum;
    uint32_t counter = 0;

    init_serial();        
    pinMode(B,5,OUTPUT);
    digitalWrite(B,5,HIGH);

    /* Wait until a message arrives or timeout */
    while((!newMessage())&&(run))
    {        
        if(counter++ > 0xFFFFF)
        {
            run = 0;
        }        
    }

    /* Was it correct message? */
    if(getByte() != 'a')
    {
        run = 0;
    }
    else
    {
        /* Send ACK */
        send_char('Y');
    }    

    while(run)
    {        
        while(!newMessage());
        msg = getByte();
        
        switch(msg)
        {
            /* Ping */
            case 'a':
            {
                /* Send ACK */
                send_char('Y');
                break;
            }
            /* Fill the page buffer */
            case 'b':
            {
                /* Send ACK */
                send_char('Y');

                for(i=0;i<SPM_PAGESIZE;i++)
                {
                    while(!newMessage());
                    pageBuf[i] = getByte();
                }

                break;
            }
            /* Program the buffer */
            case 'c':
            {
                /* Send ACK */
                send_char('Y');

                while(!newMessage());
                pageNum = getByte();

                while(!newMessage());
                t32 = getByte();
                t32 = t32 << 8;
                pageNum += t32;

                while(!newMessage());
                t32 = getByte();
                t32 = t32 << 16;
                pageNum += t32;

                while(!newMessage());
                t32 = getByte();
                t32 = t32 << 24;
                pageNum += t32;

                boot_program_page(pageNum,pageBuf);

                /* Send ACK */
                send_char('Y');
                break;
            }
            /* Delete the pages */
            case 'd':
            {   
                for(t32=0;t32<BOOTSTART;t32+=SPM_PAGESIZE)
                {
                    boot_page_erase(t32);
                    boot_spm_busy_wait();               
                }                    

                /* Send ACK */
                send_char('Y');
                break;
            }
            /* Go to user app ... */
            case 'x':
            {                    
                digitalWrite(B,5,LOW);                    
                funcptr();                 
                break;
            }
        }     
    }
    
    digitalWrite(B,5,LOW);

    /* Go to user app ... */
    funcptr();

    return 0;
}

