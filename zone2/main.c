/*
 * Copyright(C) 2018 Hex Five Security, Inc. - Some rights reserved.
 *
 * This file is part of multizone-sdk/zone2.
 *
 * multizone-sdk/zone2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * multizone-sdk/zone2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with multizone-sdk/zone2. If not, see <https://www.gnu.org/licenses/>.
 */

#include <platform.h>
#include <libhexfive.h>

#define RV32

#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_icmp4.h>
#include <pico_socket.h>
#include <pico_dev_xemaclite.h>

#define NUM_PING 10

static int finished = 0;

void *pico_zalloc(size_t size)
{
    void *ptr = malloc(size);
    if(ptr)
        memset(ptr, 0u, size);

    return ptr;
}

void pico_free(void *ptr)
{
    free(ptr);
}

pico_time PICO_TIME_MS(void)
{
    return (pico_time)(ECALL_CSRR_MCYCLE() * 1000 / CPU_FREQ);
}

pico_time PICO_TIME(void)
{
    return (pico_time)(ECALL_CSRR_MCYCLE() / CPU_FREQ);
}

void PICO_IDLE(void)
{
    ECALL_YIELD();
}

/* gets called when the ping receives a reply, or encounters a problem */
void cb_ping(struct pico_icmp4_stats *s)
{
    char host[30];
    pico_ipv4_to_string(host, s->dst.addr);
    if (s->err == 0) {
        /* if all is well, print some pretty info */
        printf("%lu bytes from %s: icmp_req=%lu ttl=%lu time=%lu ms\n", s->size,
                host, s->seq, s->ttl, (long unsigned int)s->time);
    } else {
        /* if something went wrong, print it and signal we want to stop */
        printf("PING %lu to %s: Error %d\n", s->seq, host, s->err);
    }
}

#define IAC "\xff"

#define WILL "\xfb"
#define DONT "\xfe"

#define ECHO "\x01"
#define SUPRESS_GO_AHEAD "\x03"
#define LINEMODE "\x22"

void cb_telnet(uint16_t ev, struct pico_socket *s)
{
    static uint8_t buf[16];
    static int bytes = 0;
    static int sent_mode = 0;

    if (ev & PICO_SOCK_EV_CONN) {
        struct pico_ip4 ipaddr;
        uint16_t port;
        struct pico_socket *sock_a;
        uint32_t yes = 1;

        sock_a = pico_socket_accept(s, &ipaddr.addr, &port);
        pico_socket_setoption(sock_a, PICO_TCP_NODELAY, &yes);
        sent_mode = 0;
    }

    if ((ev & PICO_SOCK_EV_RD) && sent_mode) {
        bytes = pico_socket_read(s, buf, sizeof(buf));

        if (buf[0] == '\xff') {
            bytes = 0;
        } else if (buf[0] == '\x0a') {
            bytes = 2;
            buf[0] = '\x0d';
            buf[1] = '\x0a';
        } else if (buf[0] == '\x0d') {
            bytes = 2;
            buf[0] = '\x0d';
            buf[1] = '\x0a';
        }
    }

    if (ev & PICO_SOCK_EV_WR) {
        if (bytes > 0 && sent_mode) {
            pico_socket_write(s, buf, bytes);
            bytes = 0;
        } else if (!sent_mode) {
            const char mode[] = IAC DONT LINEMODE
                                IAC WILL SUPRESS_GO_AHEAD
                                IAC WILL ECHO;

            pico_socket_write(s, mode, sizeof(mode));
            sent_mode = 1;
        }
    }

    if ((ev & PICO_SOCK_EV_CLOSE) && (ev & PICO_SOCK_EV_RD)) {
        pico_socket_shutdown(s, PICO_SHUT_WR);
    }
}

int main(int argc, char *argv[]){
    int id;
    uint16_t port = short_be(23);
    uint32_t yes = 1;
    struct pico_ip4 ipaddr, netmask;
    struct pico_socket* socket;
    struct pico_device* dev;

    uint16_t bmsr = 0;

    while ((bmsr & 0x4) == 0) {
        bmsr = pico_xemaclite_mdio_read(0x01, 0x01);
    }

    /* initialise the stack. Super important if you don't want ugly stuff like
     * segfaults and such! */
    pico_stack_init();

    printf("pico stack initialized\n");

    dev = pico_xemaclite_create();
    if (!dev) {
        printf("Could not initialize device\n");
        return -1;
    }

    /* assign the IP address to the tap interface */
    pico_string_to_ipv4("192.168.1.2", &ipaddr.addr);
    pico_string_to_ipv4("255.255.255.0", &netmask.addr);
    pico_ipv4_link_add(dev, ipaddr, netmask);

    printf("Listening on port %d\n", short_be(port));
    socket = pico_socket_open(PICO_PROTO_IPV4, PICO_PROTO_TCP, cb_telnet);
    if (!socket) {
        printf("Could not open socket!\n");
        return -1;
    }

    pico_socket_setoption(socket, PICO_TCP_NODELAY, &yes);

    if (pico_socket_bind(socket, &ipaddr, &port) != 0) {
        printf("Could not bind!\n");
        return -1;
    }

    if (pico_socket_listen(socket, 1) != 0) {
        printf("Could not start listening!\n");
        return -1;
    }

    /* keep running stack ticks to have picoTCP do its network magic. Note that
     * you can do other stuff here as well, or sleep a little. This will impact
     * your network performance, but everything should keep working (provided
     * you don't go overboard with the delays). */
    while (finished != 1)
    {
        pico_stack_tick();
        ECALL_YIELD();
    }

    printf("finished !\n");
    return 0;
}
