/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* Demo app includes. */
#include "enc28j60.h"

/* uIP includes. */
#undef HTONS
#include "uip.h"
#include "uip_arp.h"
#include "uip_timer.h"
#include "telnetd.h"

/* The start of the uIP buffer, which will contain the frame headers. */
#define pucUIP_Buffer   ( ( struct uip_eth_hdr * ) &uip_buf[ 0 ] )
#define CLOCK_SECOND    configTICK_RATE_HZ
/*-----------------------------------------------------------*/

void vuIP_TASK( void *pvParameters )
{
      int i;
      struct uip_eth_addr MacAddr = *(struct uip_eth_addr *)pvParameters;
      uip_ipaddr_t ipaddr;
      struct timer periodic_timer, arp_timer;
      
      clock_init();  
      timer_set(&periodic_timer, CLOCK_SECOND / 2);
      timer_set(&arp_timer, CLOCK_SECOND * 10);

      /* Initialise the Enc28j60*/
      enc28j60Init(MacAddr.addr);
      
      /* Initialize the uIP TCP/IP stack. */
      uip_init();
      uip_arp_init();
      
      /*init mac addr to uip*/
      uip_setethaddr(MacAddr);
      
      uip_ipaddr(ipaddr, 192,168,1,101);
      uip_sethostaddr(ipaddr);
      uip_ipaddr(ipaddr, 192,168,1,1);
      uip_setdraddr(ipaddr);
      uip_ipaddr(ipaddr, 255,255,255,0);
      uip_setnetmask(ipaddr);
      
      /* Initialize the HTTP server. */
      telnetd_init();

      while(1) {
        uip_len = enc28j60PacketReceive();
        if(uip_len > 0) 
        {
          if(pucUIP_Buffer->type == htons(UIP_ETHTYPE_IP)) 
          {
            uip_arp_ipin();
            uip_input();
            /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
            if(uip_len > 0) {
              uip_arp_out();
              enc28j60PacketSend();
            }
          }
          else if(pucUIP_Buffer->type == htons(UIP_ETHTYPE_ARP)) 
          {
            uip_arp_arpin();
            /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
            if(uip_len > 0) {
              enc28j60PacketSend();
            }
          }
          
        }
        else if(timer_expired(&periodic_timer)) 
        {
          timer_reset(&periodic_timer);
          for(i = 0; i < UIP_CONNS; i++) 
          {
            uip_periodic(i);
            /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
            if(uip_len > 0) 
            {
              uip_arp_out();
              enc28j60PacketSend();
            }
          }
          
#if UIP_UDP
          for(i = 0; i < UIP_UDP_CONNS; i++) 
          {
            uip_udp_periodic(i);
            /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
            if(uip_len > 0) 
            {
              uip_arp_out();
              enc28j60PacketSend();
            }
          }
#endif /* UIP_UDP */
          
          /* Call the ARP timer function every 10 seconds. */
          if(timer_expired(&arp_timer)) 
          {
            timer_reset(&arp_timer);
            uip_arp_timer();
          }
        }
      }
}

/*---------------------------------------------------------------------------*/
void uip_log(char *msg)
{
}
/*-----------------------------------------------------------------------------------*/





