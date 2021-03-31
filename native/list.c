/*
 * idevice_id.c
 * Prints device name or a list of attached devices
 *
 * Copyright (C) 2010-2018 Nikias Bassen <nikias@gmx.li>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define TOOL_NAME "idevice_id"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include "utils.h"

#define MODE_NONE 0
#define MODE_SHOW_ID 1
#define MODE_LIST_DEVICES 2

static void print_usage(int argc, char **argv, int is_error)
{
    char *name = NULL;
    name = strrchr(argv[0], '/');
    fprintf(is_error ? stderr : stdout, "Usage: %s [OPTIONS] [UDID]\n", (name ? name + 1: argv[0]));
    fprintf(is_error ? stderr : stdout,
        "\n" \
        "List attached devices or print device name of given device.\n" \
        "\n" \
        "  If UDID is given, the name of the connected device with that UDID" \
        "  will be retrieved.\n" \
        "\n" \
        "OPTIONS:\n" \
        "  -l, --list      list UDIDs of all devices attached via USB\n" \
        "  -n, --network   list UDIDs of all devices available via network\n" \
        "  -f, --full      show full device info\n" \
        "  -d, --debug     enable communication debugging\n" \
        "  -h, --help      prints usage information\n" \
    );
}

#define FORMAT_KEY_VALUE 1
#define FORMAT_XML 2

static const char *domains[] = {
	"com.apple.disk_usage",
	"com.apple.disk_usage.factory",
	"com.apple.mobile.battery",
/* FIXME: For some reason lockdownd segfaults on this, works sometimes though
	"com.apple.mobile.debug",. */
	"com.apple.iqagent",
	"com.apple.purplebuddy",
	"com.apple.PurpleBuddy",
	"com.apple.mobile.chaperone",
	"com.apple.mobile.third_party_termination",
	"com.apple.mobile.lockdownd",
	"com.apple.mobile.lockdown_cache",
	"com.apple.xcode.developerdomain",
	"com.apple.international",
	"com.apple.mobile.data_sync",
	"com.apple.mobile.tethered_sync",
	"com.apple.mobile.mobile_application_usage",
	"com.apple.mobile.backup",
	"com.apple.mobile.nikita",
	"com.apple.mobile.restriction",
	"com.apple.mobile.user_preferences",
	"com.apple.mobile.sync_data_class",
	"com.apple.mobile.software_behavior",
	"com.apple.mobile.iTunes.SQLMusicLibraryPostProcessCommands",
	"com.apple.mobile.iTunes.accessories",
	"com.apple.mobile.internal", /**< iOS 4.0+ */
	"com.apple.mobile.wireless_lockdown", /**< iOS 4.0+ */
	"com.apple.fairplay",
	"com.apple.iTunes",
	"com.apple.mobile.iTunes.store",
	"com.apple.mobile.iTunes",
	NULL
};

static int is_domain_known(const char *domain)
{
	int i = 0;
	while (domains[i] != NULL) {
		if (strstr(domain, domains[i++])) {
			return 1;
		}
	}
	return 0;
}

int printDeviceDetails(char* udid, int use_network);

int printDeviceDetails(char* udid, int use_network) {
	idevice_t device = NULL;        
    idevice_error_t ret = IDEVICE_E_UNKNOWN_ERROR;

	lockdownd_client_t client = NULL;    
	lockdownd_error_t ldret = LOCKDOWN_E_UNKNOWN_ERROR;

    int simple = 0;
	int format = FORMAT_KEY_VALUE;

    const char *domain = NULL;
	const char *key = NULL;
	char *xml_doc = NULL;
	uint32_t xml_length;
	plist_t node = NULL;

    ret = idevice_new_with_options(&device, udid, (use_network) ? IDEVICE_LOOKUP_NETWORK : IDEVICE_LOOKUP_USBMUX);
    if (ret != IDEVICE_E_SUCCESS) {
        if (udid) {
            printf("ERROR: Device %s not found!\n", udid);
        } else {
            printf("ERROR: No device found!\n");
        }
        return -1;
    }

    if (LOCKDOWN_E_SUCCESS != (ldret = simple ?
            lockdownd_client_new(device, &client, TOOL_NAME):
            lockdownd_client_new_with_handshake(device, &client, TOOL_NAME))) {
        //fprintf(stderr, "ERROR: Could not connect to lockdownd: %s (%d)\n", lockdownd_strerror(ldret), ldret);
        idevice_free(device);
        return -1;
    }

    if (domain && !is_domain_known(domain)) {
        fprintf(stderr, "WARNING: Sending query with unknown domain \"%s\".\n", domain);
    }

    /* run query and output information */
    if(lockdownd_get_value(client, domain, key, &node) == LOCKDOWN_E_SUCCESS) {
        if (node) {
            switch (format) {
            case FORMAT_XML:
                plist_to_xml(node, &xml_doc, &xml_length);
                printf("%s", xml_doc);
                free(xml_doc);
                break;
            case FORMAT_KEY_VALUE:
                plist_print_to_stream(node, stdout);
                break;
            default:
                if (key != NULL)
                    plist_print_to_stream(node, stdout);
            break;
            }
            plist_free(node);
            node = NULL;
        }
    }

    lockdownd_client_free(client);
    idevice_free(device);

    return 0;
}

int main(int argc, char **argv)
{
    idevice_t device = NULL;
    lockdownd_client_t client = NULL;
    idevice_info_t *dev_list = NULL;
    char *device_name = NULL;
    int ret = 0;
    int i;
    int mode = MODE_LIST_DEVICES;
    int include_usb = 0;
    int include_network = 0;
    int show_details = 0;
    const char* udid = NULL;

    int c = 0;
    const struct option longopts[] = {
        { "debug", no_argument, NULL, 'd' },
        { "help",  no_argument, NULL, 'h' },
        { "list",  no_argument, NULL, 'l' },
        { "network", no_argument, NULL, 'n' },
        { "full", no_argument, NULL, 'f' },
        { NULL, 0, NULL, 0}
    };

    while ((c = getopt_long(argc, argv, "dhlnf", longopts, NULL)) != -1) {
        switch (c) {
        case 'd':
            idevice_set_debug_level(1);
            break;
        case 'h':
            print_usage(argc, argv, 0);
            exit(EXIT_SUCCESS);
        case 'l':
            mode = MODE_LIST_DEVICES;
            include_usb = 1;
            break;
        case 'n':
            mode = MODE_LIST_DEVICES;
            include_network = 1;
            break;
        case 'f':
            show_details = 1;
            break;
        default:
            print_usage(argc, argv, 1);
            exit(EXIT_FAILURE);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 1) {
        mode = MODE_SHOW_ID;
    } else if (argc == 0 && optind == 1) {
        include_usb = 1;
        //include_network = 1;
        show_details = 1;
    }
    udid = argv[0];

    switch (mode) {
    case MODE_SHOW_ID:
        idevice_new_with_options(&device, udid, IDEVICE_LOOKUP_USBMUX | IDEVICE_LOOKUP_NETWORK);
        if (!device) {
            fprintf(stderr, "ERROR: No device with UDID %s attached.\n", udid);
            return -2;
        }

        if (LOCKDOWN_E_SUCCESS != lockdownd_client_new(device, &client, TOOL_NAME)) {
            idevice_free(device);
            fprintf(stderr, "ERROR: Connecting to device failed!\n");
            return -2;
        }

        if ((LOCKDOWN_E_SUCCESS != lockdownd_get_device_name(client, &device_name)) || !device_name) {
            fprintf(stderr, "ERROR: Could not get device name!\n");
            ret = -2;
        }

        lockdownd_client_free(client);
        idevice_free(device);

        if (ret == 0) {
            printf("%s\n", device_name);
        }

        if (device_name) {
            free(device_name);
        }
        break;

    case MODE_LIST_DEVICES:
    default:
        if (idevice_get_device_list_extended(&dev_list, &i) < 0) {
            fprintf(stderr, "ERROR: Unable to retrieve device list!\n");
            return -1;
        }
        for (i = 0; dev_list[i] != NULL; i++) {
            if (dev_list[i]->conn_type == CONNECTION_USBMUXD && !include_usb) continue;
            if (dev_list[i]->conn_type == CONNECTION_NETWORK && !include_network) continue;
            if (show_details) {
                printDeviceDetails(dev_list[i]->udid, include_network);
            }
            else {
                printf("%s", dev_list[i]->udid);
            }
            if (include_usb && include_network) {
                if (dev_list[i]->conn_type == CONNECTION_NETWORK) {
                    printf(" (Network)");
                } else {
                    printf(" (USB)");
                }
            }
            printf("\n");
        }
        idevice_device_list_extended_free(dev_list);
        break;
    }
    return ret;
}

