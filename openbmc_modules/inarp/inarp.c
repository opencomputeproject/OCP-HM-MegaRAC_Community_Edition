/******************************************************************************
 * Copyright 2016 Foxconn
 * Copyright 2016 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_link.h>
#include <linux/if_packet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

struct eth_addr {
	uint8_t		eth_addr[ETH_ALEN];
} __attribute__((packed));

struct arp_packet {
	struct ethhdr	eh;
	struct arphdr	arp;
	struct eth_addr	src_mac;
	struct in_addr	src_ip;
	struct eth_addr	dest_mac;
	struct in_addr	dest_ip;
} __attribute__((packed));

struct interface {
	int		ifindex;
	char		ifname[IFNAMSIZ+1];
	struct eth_addr	eth_addr;
};

struct inarp_ctx {
	int			arp_sd;
	int			nl_sd;
	struct interface	*interfaces;
	unsigned int		n_interfaces;
	bool			syslog;
	bool			debug;
};

static __attribute__((format(printf, 3, 4)))
	void inarp_log(struct inarp_ctx *inarp,
		int priority,
		const char *format, ...)
{
	va_list ap;

	if (priority > LOG_INFO && !inarp->debug)
		return;

	va_start(ap, format);
	if (inarp->syslog) {
		vsyslog(priority, format, ap);
	} else {
		vprintf(format, ap);
		printf("\n");
	}

	va_end(ap);
}

/* helpers for rtnetlink message iteration */
#define for_each_nlmsg(buf, nlmsg, len) \
	for (nlmsg = (struct nlmsghdr *)buf; \
		NLMSG_OK(nlmsg, len) && nlmsg->nlmsg_type != NLMSG_DONE; \
		nlmsg = NLMSG_NEXT(nlmsg, len))

#define for_each_rta(buf, rta, attrlen) \
	for (rta = (struct rtattr *)(buf); RTA_OK(rta, attrlen); \
			rta = RTA_NEXT(rta, attrlen))

static int send_arp_packet(struct inarp_ctx *inarp,
		int ifindex,
		const struct eth_addr *src_mac,
		const struct in_addr *src_ip,
		const struct eth_addr *dest_mac,
		const struct in_addr *dest_ip)
{
	struct sockaddr_ll addr;
	struct arp_packet arp;
	int rc;

	memset(&arp, 0, sizeof(arp));

	/* Prepare our link-layer address: raw packet interface,
	 * using the ifindex interface, receiving ARP packets
	 */
	addr.sll_family = PF_PACKET;
	addr.sll_protocol = htons(ETH_P_ARP);
	addr.sll_ifindex = ifindex;
	addr.sll_hatype = ARPHRD_ETHER;
	addr.sll_pkttype = PACKET_OTHERHOST;
	addr.sll_halen = ETH_ALEN;
	memcpy(addr.sll_addr, dest_mac, ETH_ALEN);

	/* set the frame header */
	memcpy(arp.eh.h_dest, dest_mac, ETH_ALEN);
	memcpy(arp.eh.h_source, src_mac, ETH_ALEN);
	arp.eh.h_proto = htons(ETH_P_ARP);

	/* Fill InARP request data for ethernet + ipv4 */
	arp.arp.ar_hrd = htons(ARPHRD_ETHER);
	arp.arp.ar_pro = htons(ETH_P_ARP);
	arp.arp.ar_hln = ETH_ALEN;
	arp.arp.ar_pln = 4;
	arp.arp.ar_op = htons(ARPOP_InREPLY);

	/* fill arp ethernet mac & ipv4 info */
	memcpy(&arp.src_mac, src_mac, sizeof(arp.src_mac));
	memcpy(&arp.src_ip, src_ip, sizeof(arp.src_ip));
	memcpy(&arp.dest_mac, dest_mac, sizeof(arp.dest_mac));
	memcpy(&arp.dest_ip, dest_ip, sizeof(arp.dest_ip));

	/* send the packet */
	rc = sendto(inarp->arp_sd, &arp, sizeof(arp), 0,
			(struct sockaddr *)&addr, sizeof(addr));
	if (rc < 0)
		inarp_log(inarp, LOG_NOTICE,
				"Failure sending ARP response: %m");

	return rc;
}

static const char *eth_mac_to_str(const struct eth_addr *mac_addr)
{
	static char mac_str[ETH_ALEN * (sizeof("00:") - 1)];
	const uint8_t *addr = mac_addr->eth_addr;

	snprintf(mac_str, sizeof(mac_str),
			"%02x:%02x:%02x:%02x:%02x:%02x",
			addr[0], addr[1], addr[2],
			addr[3], addr[4], addr[5]);

	return mac_str;
}

static int do_ifreq(int fd, unsigned long type,
		const char *ifname, struct ifreq *ifreq)
{
	memset(ifreq, 0, sizeof(*ifreq));
	strncpy(ifreq->ifr_name, ifname, sizeof(ifreq->ifr_name));

	return ioctl(fd, type, ifreq);
}

static int get_local_ipaddr(struct inarp_ctx *inarp,
		const char *ifname, struct in_addr *addr)
{
	struct sockaddr_in *sa;
	struct ifreq ifreq;
	int rc;

	rc = do_ifreq(inarp->arp_sd, SIOCGIFADDR, ifname, &ifreq);
	if (rc) {
		inarp_log(inarp, LOG_WARNING,
			"Error querying local IP address for %s: %m",
			ifname);
		return -1;
	}

	if (ifreq.ifr_addr.sa_family != AF_INET) {
		inarp_log(inarp, LOG_WARNING,
			"Unknown address family %d in address response",
			ifreq.ifr_addr.sa_family);
		return -1;
	}

	sa = (struct sockaddr_in *)&ifreq.ifr_addr;
	memcpy(addr, &sa->sin_addr, sizeof(*addr));
	return 0;
}

static struct interface *find_interface_by_ifindex(struct inarp_ctx *inarp,
		int ifindex)
{
	unsigned int i;

	for (i = 0; i < inarp->n_interfaces; i++) {
		struct interface *iface = &inarp->interfaces[i];
		if (iface->ifindex == ifindex)
			return iface;
	}

	return NULL;
}

static int init_netlink(struct inarp_ctx *inarp)
{
	struct sockaddr_nl addr;
	int rc;
	struct {
		struct nlmsghdr nlmsg;
		struct rtgenmsg rtmsg;
	} msg;

	/* create our socket to listen for rtnetlink events */
	inarp->nl_sd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (inarp->nl_sd < 0) {
		inarp_log(inarp, LOG_ERR, "Error opening netlink socket: %m");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = RTMGRP_LINK;

	rc = bind(inarp->nl_sd, (struct sockaddr *)&addr, sizeof(addr));
	if (rc) {
		inarp_log(inarp, LOG_ERR,
				"Error binding to netlink address: %m");
		goto err_close;
	}

	/* send a query for current interfaces */
	memset(&msg, 0, sizeof(msg));

	msg.nlmsg.nlmsg_len = sizeof(msg);
	msg.nlmsg.nlmsg_type = RTM_GETLINK;
	msg.nlmsg.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg.rtmsg.rtgen_family = AF_UNSPEC;

	rc = send(inarp->nl_sd, &msg, sizeof(msg), MSG_NOSIGNAL);
	if (rc != sizeof(msg)) {
		inarp_log(inarp, LOG_ERR, "Failed to query current links: %m");
		goto err_close;
	}

	return 0;

err_close:
	close(inarp->nl_sd);
	return -1;
}

static void netlink_nlmsg_dellink(struct inarp_ctx *inarp,
		struct interface *iface)
{
	int i;

	if (!iface)
		return;

	inarp_log(inarp, LOG_NOTICE, "dropping interface: %s, [%s]",
			iface->ifname, eth_mac_to_str(&iface->eth_addr));

	/* find the index of the array element to remove */
	i = iface - inarp->interfaces;

	/* remove interface from our array */
	inarp->n_interfaces--;
	inarp->interfaces = realloc(inarp->interfaces,
			inarp->n_interfaces * sizeof(*iface));
	memmove(iface, iface + 1,
			sizeof(*iface) * (inarp->n_interfaces - i));

}
static void netlink_nlmsg_newlink(struct inarp_ctx *inarp,
		struct interface *iface, struct ifinfomsg *ifmsg, int len)
{
	struct rtattr *attr;
	bool new = false;

	/*
	 * We shouldn't already have an interface for this ifindex; so create
	 * one. If we do, we'll update the hwaddr and name to the new values.
	 */
	if (!iface) {
		inarp->n_interfaces++;
		inarp->interfaces = realloc(inarp->interfaces,
				inarp->n_interfaces * sizeof(*iface));
		iface = &inarp->interfaces[inarp->n_interfaces-1];
		new = true;
	}

	memset(iface, 0, sizeof(*iface));
	iface->ifindex = ifmsg->ifi_index;

	for_each_rta(ifmsg + 1, attr, len) {
		void *data = RTA_DATA(attr);

		switch (attr->rta_type) {
		case IFLA_ADDRESS:
			memcpy(&iface->eth_addr.eth_addr, data,
					sizeof(iface->eth_addr.eth_addr));
			break;

		case IFLA_IFNAME:
			strncpy(iface->ifname, data, IFNAMSIZ);
			break;
		}
	}

	inarp_log(inarp, LOG_NOTICE, "%s interface: %s, [%s]",
			new ? "adding" : "updating",
			iface->ifname,
			eth_mac_to_str(&iface->eth_addr));
}

static void netlink_nlmsg(struct inarp_ctx *inarp, struct nlmsghdr *nlmsg)
{
	struct ifinfomsg *ifmsg;
	struct interface *iface;
	int len;

	len = nlmsg->nlmsg_len - sizeof(*ifmsg);
	ifmsg = NLMSG_DATA(nlmsg);

	iface = find_interface_by_ifindex(inarp, ifmsg->ifi_index);

	switch (nlmsg->nlmsg_type) {
	case RTM_DELLINK:
		netlink_nlmsg_dellink(inarp, iface);
		break;
	case RTM_NEWLINK:
		netlink_nlmsg_newlink(inarp, iface, ifmsg, len);
		break;
	default:
		break;
	}
}

static void netlink_recv(struct inarp_ctx *inarp)
{
	struct nlmsghdr *nlmsg;
	uint8_t buf[16384];
	int len;

	len = recv(inarp->nl_sd, &buf, sizeof(buf), 0);
	if (len < 0) {
		inarp_log(inarp, LOG_NOTICE, "Error receiving netlink msg");
		return;
	}

	size_t len_unsigned = (size_t)len;

	for_each_nlmsg(buf, nlmsg, len_unsigned)
		netlink_nlmsg(inarp, nlmsg);
}

static void arp_recv(struct inarp_ctx *inarp)
{
	struct arp_packet inarp_req;
	struct sockaddr_ll addr;
	struct in_addr local_ip;
	struct interface *iface;
	socklen_t addrlen;
	int len, rc;

	addrlen = sizeof(addr);
	len = recvfrom(inarp->arp_sd, &inarp_req,
			sizeof(inarp_req), 0,
			(struct sockaddr *)&addr, &addrlen);
	if (len <= 0) {
		if (errno == EINTR)
			return;
		inarp_log(inarp, LOG_WARNING,
				"Error receiving ARP packet");
	}

	/*
	 * struct sockaddr_ll allows for 8 bytes of hardware address;
	 * we only need ETH_ALEN for a full ethernet address.
	 */
	if (addrlen < sizeof(addr) - (8 - ETH_ALEN))
		return;

	if (addr.sll_family != AF_PACKET)
		return;

	iface = find_interface_by_ifindex(inarp, addr.sll_ifindex);
	if (!iface)
		return;

	/* Is this packet large enough for an inarp? */
	if ((size_t)len < sizeof(inarp_req))
		return;

	/* ... is it an inarp request? */
	if (ntohs(inarp_req.arp.ar_op) != ARPOP_InREQUEST)
		return;

	/* ... for us? */
	if (memcmp(&iface->eth_addr, inarp_req.eh.h_dest, ETH_ALEN))
		return;

	inarp_log(inarp, LOG_DEBUG,
			"request from src mac: %s",
			eth_mac_to_str(&inarp_req.src_mac));

	rc = get_local_ipaddr(inarp, iface->ifname, &local_ip);
	/* if we don't have a local IP address to send, just drop the
	 * request */
	if (rc)
		return;
	inarp_log(inarp, LOG_DEBUG,
			"responding with %s ip %s",
			eth_mac_to_str(&iface->eth_addr),
			inet_ntoa(local_ip));

	send_arp_packet(inarp, iface->ifindex,
			&inarp_req.dest_mac,
			&local_ip,
			&inarp_req.src_mac,
			&inarp_req.src_ip);
}

int main(int argc, char **argv)
{
	struct inarp_ctx inarp;
	int ret, i;

	memset(&inarp, 0, sizeof(inarp));

	inarp.syslog = true;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--debug"))
			inarp.debug = true;
		else if (!strcmp(argv[i], "--no-syslog"))
			inarp.syslog = false;
	}

	if (inarp.syslog)
		openlog("inarp", 0, LOG_DAEMON);

	inarp.arp_sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (inarp.arp_sd < 0) {
		inarp_log(&inarp, LOG_ERR, "Error opening ARP socket");
		exit(EXIT_FAILURE);
	}

	ret = init_netlink(&inarp);
	if (ret)
		exit(EXIT_FAILURE);

	while (1) {
		struct pollfd pollfds[2];

		pollfds[0].fd = inarp.arp_sd;
		pollfds[0].events = POLLIN;
		pollfds[1].fd = inarp.nl_sd;
		pollfds[1].events = POLLIN;

		ret = poll(pollfds, 2, -1);
		if (ret < 0) {
			inarp_log(&inarp, LOG_ERR, "poll failed, exiting");
			break;
		}

		if (pollfds[0].revents)
			arp_recv(&inarp);

		if (pollfds[1].revents)
			netlink_recv(&inarp);


	}
	close(inarp.arp_sd);
	close(inarp.nl_sd);
	return 0;
}
