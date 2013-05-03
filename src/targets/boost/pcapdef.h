#ifndef PCAPDEF_H
#define PCAPDEF_H

/**
 * PCAP Global header
 *
 * http://wiki.wireshark.org/Development/LibpcapFileFormat
 */
typedef struct pcap_hdr_s {
	uint32_t magic_number;   /* magic number */
	uint16_t version_major;  /* major version number */
	uint16_t version_minor;  /* minor version number */
	int32_t  thiszone;       /* GMT to local correction */
	uint32_t sigfigs;        /* accuracy of timestamps */
	uint32_t snaplen;        /* max length of captured packets, in octets */
	uint32_t network;        /* data link type */
} pcap_hdr_t;

/**
 * PCAP Packet (record) header
 *
 * http://wiki.wireshark.org/Development/LibpcapFileFormat
 */
typedef struct pcaprec_hdr_s {
	uint32_t ts_sec;         /* timestamp seconds */
	uint32_t ts_usec;        /* timestamp microseconds */
	uint32_t incl_len;       /* number of octets of packet saved in file */
	uint32_t orig_len;       /* actual length of packet */
} pcaprec_hdr_t;


// Some data link types

#define DLT_EN10MB	1	/* Ethernet (10Mb) */

#ifdef __OpenBSD__
#define DLT_RAW		14	/* raw IP */
#else
#define DLT_RAW		12	/* raw IP */
#endif

#endif // PCAPDEF_H
