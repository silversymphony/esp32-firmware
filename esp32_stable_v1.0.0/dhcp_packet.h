#ifndef DHCP_PACKET_H
#define DHCP_PACKET_H

#include <Arduino.h>

// =========================================================================
//           CẤU TRÚC GÓI TIN DHCP (RFC 2131 / RFC 2132)
// =========================================================================
#pragma pack(push, 1)
struct DhcpPacket {
  uint8_t op;           // 1: BOOTREQUEST, 2: BOOTREPLY
  uint8_t htype;        // 1: Ethernet
  uint8_t hlen;         // 6: MAC len
  uint8_t hops;         // 0
  uint32_t xid;         // Transaction ID
  uint16_t secs;        // 0
  uint16_t flags;       // 0x8000 = broadcast
  uint32_t ciaddr;      // Client IP (0)
  uint32_t yiaddr;      // Your (client) IP
  uint32_t siaddr;      // Server IP
  uint32_t giaddr;      // Relay agent IP (0)
  uint8_t chaddr[16];   // Client MAC
  char sname[64];       // 0
  char file[128];       // 0
  uint8_t magic[4];     // 0x63, 0x82, 0x53, 0x63
  uint8_t options[308]; // Options
};
#pragma pack(pop)

#endif // DHCP_PACKET_H
