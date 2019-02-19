#include "nrf51.h"
#include "nrf51_bitfields.h"

#include "ble.h"

/**
 * Within the Bluetooth Core Specification v5.1, the relevant references are:
 * 
 * - Vol. 6, Part B, 2.1: packet structure for uncoded PHY (in terms of an unspecified payload)
 * 
 * - Vol. 6, Part B, 2.3: packet payload for advertising packets
 * 
 * - Vol. 6, Part B, 2.4: packet payload for data packets
 * 
 * Both the advertising and data packet share the following structure
 * 
 *     ------------------------------------------------------------------------------
 *    | Preamble | Access Address | Type-specific (1 byte) | Length (1 byte) | Other |
 *     ------------------------------------------------------------------------------
 * 
 * This matches nRF51's radio packet format with S0 length 1 byte, LENGTH 1 byte and S1 length 0 bytes (refer to nRF51 Reference Manual v3.0 17.1.2).
 * 
 * The nRF51's radio only supports a maximum length of 254 bytes, including S0 and LENGTH (nRF51 Reference Manual v3.0 17.1.2).
 * This falls short of the maximum allowed packet length for BLE:
 * 
 * - Bluetooth v5.1 reaches a combined 258 bytes (see Vol. 6, Part B, 2.1: the actual maximum is reached with a data channel packet that includes
 *   CTEInfo)
 * - Bluetooth v4.2 similarly allows a combined 257 bytes (again, Vol. 6, Part B, 2.1)
 */
void radio_rx_setup(void)
{
    NRF_RADIO->INTENCLR = 0x4FFUL; // Disables all interrupts

    NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos;

    NRF_RADIO->PCNF0 = (1UL << RADIO_PCNF0_S0LEN_Pos)  // S0 length:     8  bits  / 1 *byte* (yes, this one is in bytes)
                     | (8UL << RADIO_PCNF0_LFLEN_Pos)  // LENGTH length: 8 *bits* / 1  byte
                     | (0UL << RADIO_PCNF0_S1LEN_Pos); // S1 length:     0 *bits* / 0  byte

    /**
     * "Independent of the configuration of MAXLEN, the combined length of S0, LENGTH, S1 and PAYLOAD cannot exceed 254 bytes."
     * - nRF51 Reference Manual v3.0, 17.1.3
     * To obtain the maximum length hardware allows, we should set MAXLEN to 254 bytes minus the length of S0 (1 byte), LENGTH (1 byte) and S1
     * (0 bytes), which results in 252 bytes.
     */
    NRF_RADIO->PCNF1 = (252UL << RADIO_PCNF1_MAXLEN_Pos ) // Maximum payload length: 252 bytes
                     | (  0UL << RADIO_PCNF1_STATLEN_Pos) // Add 0 bytes to LENGTH field to obtain payload length
                     | (  3UL << RADIO_PCNF1_BALEN_Pos  ) // *3 bytes for base address* + 1 byte prefix for a total of 4 bytes of access address
                     // Little endian required by BT specification and for radio address matching (nRF51 RM v3.0 17.1.13)
                     | (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos)
                     // Enable data whitening. Note that the radio's whitening scheme (nRF51 RM v3.0 17.1.6) is the one described in
                     // the BT specification (v5.1: Vol. 6, Part B, 17.1.6)
                     | (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);

    // 3 byte base address with BT access address LSBs
    // nRF51 RM v3.0 17.1.4: "The base address is truncated from the left if the BALEN is less than 4"
    // @note "Truncating from the left" isn't clear to me at all? I just checked Mynewt's implementation
    NRF_RADIO->BASE0   = (BLE_ACCESS_ADDRESS <<  8) & 0xFFFFFF00;  
    NRF_RADIO->PREFIX0 = (BLE_ACCESS_ADDRESS >> 24) & 0xFF;    // 1 byte prefix with access address MSB

} 