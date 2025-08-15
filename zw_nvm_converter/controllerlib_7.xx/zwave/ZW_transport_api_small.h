#ifndef _ZW_TRANSPORT_API_SMALL_H
#define _ZW_TRANSPORT_API_SMALL_H

// ---------------------------------------------------------------------------
// ZWave/API/ZW_transport_api.h
// ---------------------------------------------------------------------------

/* Max number of nodes in a Z-wave system */
#define ZW_LR_MAX_NODE_ID     1279  // LR bitmask has only 128 bytes in NV
#define ZW_LR_MIN_NODE_ID     256
#define ZW_CLASSIC_MAX_NODES  232
#define ZW_LR_MAX_NODES       ((ZW_LR_MAX_NODE_ID - ZW_LR_MIN_NODE_ID) + 1)

#define HOMEID_LENGTH      4
#define MAX_REPEATERS      4

/* Protocol version in nodeinfo capability */
#define ZWAVE_NODEINFO_PROTOCOL_VERSION_MASK    0x07
#define ZWAVE_NODEINFO_VERSION_2                0x01
#define ZWAVE_NODEINFO_VERSION_3                0x02
#define ZWAVE_NODEINFO_VERSION_4                0x03

/* Baud rate in nodeinfo capability */
#define ZWAVE_NODEINFO_BAUD_RATE_MASK           0x38
#define ZWAVE_NODEINFO_BAUD_9600                0x08
#define ZWAVE_NODEINFO_BAUD_40000               0x10

/* Routing support in nodeinfo capabilitity */
#define ZWAVE_NODEINFO_ROUTING_SUPPORTED_MASK   0x40
#define ZWAVE_NODEINFO_ROUTING_SUPPORT          0x40

/* Listening in nodeinfo capabilitity */
#define ZWAVE_NODEINFO_LISTENING_MASK            0x80
#define ZWAVE_NODEINFO_LISTENING_SUPPORT         0x80

/* Security bit in nodeinfo security field */
#define ZWAVE_NODEINFO_SECURITY_MASK             0x01
#define ZWAVE_NODEINFO_SECURITY_SUPPORT          0x01

/* Controller bit in nodeinfo security field */
#define ZWAVE_NODEINFO_CONTROLLER_MASK           0x02
#define ZWAVE_NODEINFO_CONTROLLER_NODE           0x02

/* Specific device type bit in nodeinfo security field */
#define ZWAVE_NODEINFO_SPECIFIC_DEVICE_MASK      0x04
#define ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE      0x04

/* Slave_routing bit in nodeinfo security field */
#define ZWAVE_NODEINFO_SLAVE_ROUTING_MASK        0x08
#define ZWAVE_NODEINFO_SLAVE_ROUTING             0x08

/* Beam capability bit in nodeinfo security field */
#define ZWAVE_NODEINFO_BEAM_CAPABILITY_MASK      0x10
#define ZWAVE_NODEINFO_BEAM_CAPABILITY           0x10

/* Sensor mode bits in nodeinfo security field */
#define ZWAVE_NODEINFO_SENSOR_MODE_MASK          0x60
#define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000   0x40
#define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250    0x20
#define ZWAVE_NODEINFO_SENSOR_MODE_RESERVED      0x60

/* Optional Functionality bit in nodeinfo security field */
#define ZWAVE_NODEINFO_OPTIONAL_FUNC_MASK        0x80
#define ZWAVE_NODEINFO_OPTIONAL_FUNC             0x80

/* Speed extension in nodeinfo reserved field */
#define ZWAVE_NODEINFO_BAUD_100K                 0x01
#define ZWAVE_NODEINFO_BAUD_100KLR               0x02

/* Defines for Zensor wakeup time - how long must the beam be, so that the zensor */
/* is capable to hear it. */
/* Not a zensor findneighbor operation, but a plain standard findneighbor operation */
#define ZWAVE_SENSOR_WAKEUPTIME_NONE    0x00
/* The wakeup time for the zensor node in nodemask is 1000ms */
#define ZWAVE_SENSOR_WAKEUPTIME_1000MS  0x01
/* The wakeup time for the zensor node in nodemask is 250ms */
#define ZWAVE_SENSOR_WAKEUPTIME_250MS   0x02
#endif // _ZW_TRANSPORT_API_SMALL_H

#define NODEINFO_FLAG_ROUTING     0x01
#define NODEINFO_FLAG_LISTENING   0x02
#define NODEINFO_FLAG_SPECIFIC    0x04
#define NODEINFO_FLAG_BEAM        0x08
#define NODEINFO_FLAG_OPTIONAL    0x10
#define NODEINFO_MASK_SENSOR      0x60

