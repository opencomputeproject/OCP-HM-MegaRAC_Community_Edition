#include "pldm.h"
#include "base.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

const uint8_t MCTP_MSG_TYPE_PLDM = 1;

pldm_requester_rc_t pldm_open()
{
	int fd = -1;
	int rc = -1;

	fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (-1 == fd) {
		return fd;
	}

	const char path[] = "\0mctp-mux";
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, path, sizeof(path) - 1);
	rc = connect(fd, (struct sockaddr *)&addr,
		     sizeof(path) + sizeof(addr.sun_family) - 1);
	if (-1 == rc) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}
	rc = write(fd, &MCTP_MSG_TYPE_PLDM, sizeof(MCTP_MSG_TYPE_PLDM));
	if (-1 == rc) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}

	return fd;
}

/**
 * @brief Read MCTP socket. If there's data available, return success only if
 *        data is a PLDM message.
 *
 * @param[in] eid - destination MCTP eid
 * @param[in] mctp_fd - MCTP socket fd
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM msg,
 *             this function allocates memory, caller to free(*pldm_resp_msg) on
 *             success.
 * @param[out] resp_msg_len - caller owned pointer that will be made point to
 *             the size of the PLDM msg.
 *
 * @return pldm_requester_rc_t (errno may be set). failure is returned even
 *         when data was read, but wasn't a PLDM response message
 */
static pldm_requester_rc_t mctp_recv(mctp_eid_t eid, int mctp_fd,
				     uint8_t **pldm_resp_msg,
				     size_t *resp_msg_len)
{
	ssize_t min_len = sizeof(eid) + sizeof(MCTP_MSG_TYPE_PLDM) +
			  sizeof(struct pldm_msg_hdr);
	ssize_t length = recv(mctp_fd, NULL, 0, MSG_PEEK | MSG_TRUNC);
	if (length <= 0) {
		return PLDM_REQUESTER_RECV_FAIL;
	} else if (length < min_len) {
		/* read and discard */
		uint8_t buf[length];
		recv(mctp_fd, buf, length, 0);
		return PLDM_REQUESTER_INVALID_RECV_LEN;
	} else {
		struct iovec iov[2];
		size_t mctp_prefix_len =
		    sizeof(eid) + sizeof(MCTP_MSG_TYPE_PLDM);
		uint8_t mctp_prefix[mctp_prefix_len];
		size_t pldm_len = length - mctp_prefix_len;
		iov[0].iov_len = mctp_prefix_len;
		iov[0].iov_base = mctp_prefix;
		*pldm_resp_msg = malloc(pldm_len);
		iov[1].iov_len = pldm_len;
		iov[1].iov_base = *pldm_resp_msg;
		struct msghdr msg = {0};
		msg.msg_iov = iov;
		msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
		ssize_t bytes = recvmsg(mctp_fd, &msg, 0);
		if (length != bytes) {
			free(*pldm_resp_msg);
			return PLDM_REQUESTER_INVALID_RECV_LEN;
		}
		if ((mctp_prefix[0] != eid) ||
		    (mctp_prefix[1] != MCTP_MSG_TYPE_PLDM)) {
			free(*pldm_resp_msg);
			return PLDM_REQUESTER_NOT_PLDM_MSG;
		}
		*resp_msg_len = pldm_len;
		return PLDM_REQUESTER_SUCCESS;
	}
}

pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	pldm_requester_rc_t rc =
	    mctp_recv(eid, mctp_fd, pldm_resp_msg, resp_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (hdr->request != PLDM_RESPONSE) {
		free(*pldm_resp_msg);
		return PLDM_REQUESTER_NOT_RESP_MSG;
	}

	uint8_t pldm_rc = 0;
	if (*resp_msg_len < (sizeof(struct pldm_msg_hdr) + sizeof(pldm_rc))) {
		free(*pldm_resp_msg);
		return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
	}

	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t pldm_recv(mctp_eid_t eid, int mctp_fd, uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	pldm_requester_rc_t rc =
	    pldm_recv_any(eid, mctp_fd, pldm_resp_msg, resp_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (hdr->instance_id != instance_id) {
		free(*pldm_resp_msg);
		return PLDM_REQUESTER_INSTANCE_ID_MISMATCH;
	}

	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t pldm_send_recv(mctp_eid_t eid, int mctp_fd,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)
{
	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)pldm_req_msg;
	if ((hdr->request != PLDM_REQUEST) &&
	    (hdr->request != PLDM_ASYNC_REQUEST_NOTIFY)) {
		return PLDM_REQUESTER_NOT_REQ_MSG;
	}

	pldm_requester_rc_t rc =
	    pldm_send(eid, mctp_fd, pldm_req_msg, req_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	while (1) {
		rc = pldm_recv(eid, mctp_fd, hdr->instance_id, pldm_resp_msg,
			       resp_msg_len);
		if (rc == PLDM_REQUESTER_SUCCESS) {
			break;
		}
	}

	return rc;
}

pldm_requester_rc_t pldm_send(mctp_eid_t eid, int mctp_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	uint8_t hdr[2] = {eid, MCTP_MSG_TYPE_PLDM};

	struct iovec iov[2];
	iov[0].iov_base = hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = (uint8_t *)pldm_req_msg;
	iov[1].iov_len = req_msg_len;

	struct msghdr msg = {0};
	msg.msg_iov = iov;
	msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);

	ssize_t rc = sendmsg(mctp_fd, &msg, 0);
	if (rc == -1) {
		return PLDM_REQUESTER_SEND_FAIL;
	}
	return PLDM_REQUESTER_SUCCESS;
}
