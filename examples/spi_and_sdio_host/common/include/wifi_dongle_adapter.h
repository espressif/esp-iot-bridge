/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define PRIO_Q_SERIAL                             0
#define PRIO_Q_BT                                 1
#define PRIO_Q_OTHERS                             2
#define MAX_PRIORITY_QUEUES                       3

/* ESP Payload Header Flags */
#define MORE_FRAGMENT                             (1 << 0)
#define DHCPS_CHANGED                             (1 << 1)

/* Serial interface */
#define SERIAL_IF_FILE                            "/dev/esps0"

/* Protobuf related info */
/* Endpoints registered must have same string length */
#define CTRL_EP_NAME_RESP                         "ctrlResp"
#define CTRL_EP_NAME_EVENT                        "ctrlEvnt"

#define NIC_LINK_UP             false
#define NIC_LINK_DOWN           true

/** @ingroup ipaddr
 * IP address types for use in ip_addr_t.type member.
 */
enum ip_addr_type {
    /** IPv4 */
    TYPE_V4 =   0U,
    /** IPv6 */
    TYPE_V6 =   6U,
    /** IPv4+IPv6 ("dual-stack") */
    TYPE_ANY = 46U
};

typedef struct {
    uint32_t addr;
} ipv4_addr_t;

typedef struct {
    uint32_t addr[4];
    uint8_t zone;
} ipv6_addr_t;

/** @brief IPV4 IP address information
  */
typedef struct {
    ipv4_addr_t ip;      /**< Interface IPV4 address */
    ipv4_addr_t netmask; /**< Interface IPV4 netmask */
    ipv4_addr_t gw;      /**< Interface IPV4 gateway address */
} ipv4_info_t;

/** @brief IPV6 IP address information
  */
typedef struct {
    ipv6_addr_t ip; /**< Interface IPV6 address */
} ipv6_info_t;

struct esp_dhcps {
    union {
        ipv6_info_t ip6;
        ipv4_info_t ip4;
    } u_addr;
    /** @ref ip_addr_type */
    uint8_t type;
    uint8_t set_link;
};

typedef struct esp_dhcps esp_dhcps_t;

struct esp_payload_header {
	uint8_t          if_type:4;
	uint8_t          if_num:4;
	uint8_t          flags;
	uint16_t         len;
	uint16_t         offset;
	uint16_t         checksum;
	uint16_t		 seq_num;
	uint8_t          reserved2;
	/* Position of union field has to always be last,
	 * this is required for hci_pkt_type */
	union {
		uint8_t      reserved3;
		uint8_t      hci_pkt_type;		/* Packet type for HCI interface */
		uint8_t      priv_pkt_type;		/* Packet type for priv interface */
	};
	/* Do no add anything here */
} __attribute__((packed));

typedef enum {
	ESP_STA_IF,
	ESP_AP_IF,
	ESP_SERIAL_IF,
	ESP_HCI_IF,
	ESP_PRIV_IF,
	ESP_TEST_IF,
	ESP_MAX_IF,
} ESP_INTERFACE_TYPE;

typedef enum {
	ESP_OPEN_DATA_PATH,
	ESP_CLOSE_DATA_PATH,
	ESP_RESET,
	ESP_MAX_HOST_INTERRUPT,
} ESP_HOST_INTERRUPT;


typedef enum {
	ESP_WLAN_SDIO_SUPPORT = (1 << 0),
	ESP_BT_UART_SUPPORT = (1 << 1),
	ESP_BT_SDIO_SUPPORT = (1 << 2),
	ESP_BLE_ONLY_SUPPORT = (1 << 3),
	ESP_BR_EDR_ONLY_SUPPORT = (1 << 4),
	ESP_WLAN_SPI_SUPPORT = (1 << 5),
	ESP_BT_SPI_SUPPORT = (1 << 6),
	ESP_CHECKSUM_ENABLED = (1 << 7),
} ESP_CAPABILITIES;

typedef enum {
	ESP_PACKET_TYPE_EVENT,
} ESP_PRIV_PACKET_TYPE;

typedef enum {
	ESP_PRIV_EVENT_INIT,
} ESP_PRIV_EVENT_TYPE;

typedef enum {
	ESP_PRIV_CAPABILITY,
	ESP_PRIV_SPI_CLK_MHZ,
	ESP_PRIV_FIRMWARE_CHIP_ID,
} ESP_PRIV_TAG_TYPE;

struct esp_priv_event {
	uint8_t		event_type;
	uint8_t		event_len;
	uint8_t		event_data[0];
}__attribute__((packed));


static inline uint16_t compute_checksum(uint8_t *buf, uint16_t len)
{
	uint16_t checksum = 0;
	uint16_t i = 0;

	while(i < len) {
		checksum += buf[i];
		i++;
	}

	return checksum;
}
