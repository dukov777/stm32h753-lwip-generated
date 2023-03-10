/**
  ******************************************************************************
  * @file    LwIP/LwIP_UDP_Echo_Server/Src/udp_echoserver.c
  * @author  MCD Application Team
  * @brief   UDP echo server
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lwip/pbuf.h"
#if LWIP_UDP

#include "lwip/udp.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include "udp_echoserver.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void udp_echoserver_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initialize the server application.
  * @param  None
  * @retval None
  */
void udp_echoserver_init(const ip4_addr_t *ipaddr)
{
	struct udp_pcb *upcb;
	err_t err;

	/* Create a new UDP control block  */
	upcb = udp_new();

	if (upcb)
	{
		/* Bind the upcb to the UDP_PORT port */
		err = udp_bind(upcb, ipaddr, UDP_SERVER_PORT);

		if (err == ERR_OK)
		{
			/* Set a receive callback for the upcb */
			udp_recv(upcb, udp_echoserver_receive_callback, NULL);
		}
		else
		{
			udp_remove(upcb);
		}
	}
}

/**
  * @brief This function is called when an UDP datagrm has been received on the port UDP_PORT.
  * @param arg user supplied argument (udp_pcb.recv_arg)
  * @param pcb the udp_pcb which received data
  * @param p the packet buffer that was received
  * @param addr the remote IP address from which the packet was received
  * @param port the remote port from which the packet was received
  * @retval None
  */
char str[100];

void udp_echoserver_receive_callback(void *arg, struct udp_pcb *upcb,
		struct pbuf *p, const ip_addr_t *addr, u16_t port) {
	struct pbuf *p_tx;

	strcpy(str, "Server Reply: ");
	//append the payload
	strncat(str, p->payload, p->len);

	/* allocate pbuf from RAM*/
	p_tx = pbuf_alloc(PBUF_TRANSPORT, strlen(str), PBUF_RAM);

	if (p_tx != NULL) {
		// initialize the TX payload with data we supplied
		pbuf_take(p_tx, str, strlen(str));

		/* connect to the remote client */
		udp_connect(upcb, addr, UDP_CLIENT_PORT);

		/* Tell the client that we have accepted it */
		udp_send(upcb, p_tx);

		/* free the UDP connection, so we can accept new clients */
		udp_disconnect(upcb);

		/* Free the p_tx buffer */
		pbuf_free(p_tx);

		/* Free the p buffer */
		pbuf_free(p);
	}
}

#endif

