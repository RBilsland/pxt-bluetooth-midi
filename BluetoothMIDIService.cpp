/* Copyright (c) 2014 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "MicroBitConfig.h"
#include "BluetoothMIDIService.h"

//================================================================
#if MICROBIT_CODAL
//================================================================

#include "pxt.h"
#include "MicroBitBLEManager.h"
#include "ble_advdata.h"
#include "ble_gap.h"

using namespace codal;

// BLE MIDI spec: centrals (macOS Audio MIDI Setup, iOS) scan for this 128-bit UUID in advertisements.
static const uint8_t midi_adv_payload[] = {
    0x02, 0x01, 0x06,
    0x11, 0x07,
    0x00, 0xc7, 0xc4, 0x4e, 0xe3, 0x6c, 0x51, 0xa7,
    0x33, 0x4b, 0xe8, 0xed, 0x5a, 0x0e, 0xb8, 0x03,
};

void BluetoothMIDIService::configureMidiAdvertising(uint8_t serviceUuidType)
{
    (void)serviceUuidType;

    uint8_t adv_handle = 0;
    uint8_t enc_scanrsp[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
    uint16_t scan_len = sizeof(enc_scanrsp);

    // macOS Audio MIDI Setup scans for AD type 0x07 (complete 128-bit service UUID).
    // ble_advdata_encode() with uuids_complete only emits a 16-bit alias, which iOS/macOS
    // BLE MIDI centrals do not treat as a MIDI peripheral in their picker.
    const uint16_t adv_len = sizeof(midi_adv_payload);

    ble_advdata_t srdata;
    memset(&srdata, 0, sizeof(srdata));
    srdata.name_type = BLE_ADVDATA_FULL_NAME;
    uint32_t err = ble_advdata_encode(&srdata, enc_scanrsp, &scan_len);
    if (err != NRF_SUCCESS)
        scan_len = 0;

    ble_gap_adv_data_t gap_adv_data;
    memset(&gap_adv_data, 0, sizeof(gap_adv_data));
    gap_adv_data.adv_data.p_data = (uint8_t *)midi_adv_payload;
    gap_adv_data.adv_data.len = adv_len;
    if (scan_len > 0) {
        gap_adv_data.scan_rsp_data.p_data = enc_scanrsp;
        gap_adv_data.scan_rsp_data.len = scan_len;
    }

    ble_gap_adv_params_t gap_adv_params;
    memset(&gap_adv_params, 0, sizeof(gap_adv_params));
    gap_adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    gap_adv_params.interval = (1000 * MICROBIT_BLE_ADVERTISING_INTERVAL) / 625;
    if (gap_adv_params.interval < BLE_GAP_ADV_INTERVAL_MIN)
        gap_adv_params.interval = BLE_GAP_ADV_INTERVAL_MIN;
    gap_adv_params.duration = 0;
    // Always accept connections from any central. Whitelist filtering here caused
    // "visible in scan, connection timeout" when no bonded peers exist.
    gap_adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
    gap_adv_params.primary_phy = BLE_GAP_PHY_1MBPS;

    sd_ble_gap_adv_stop(adv_handle);
    MICROBIT_BLE_ECHK(sd_ble_gap_adv_set_configure(&adv_handle, &gap_adv_data, &gap_adv_params));

    if (MicroBitBLEManager::manager) {
        MicroBitBLEManager::manager->setAdvertiseOnDisconnect(true);
        MicroBitBLEManager::manager->advertise();
    }
}

// BLE MIDI service: 03B80E5A-EDE8-4B33-A751-6CE34EC4C700
const uint8_t BluetoothMIDIService::service_base_uuid[16] =
    { 0x03, 0xb8, 0x00, 0x00, 0xed, 0xe8, 0x4b, 0x33,
      0xa7, 0x51, 0x6c, 0xe3, 0x4e, 0xc4, 0xc7, 0x00 };

// BLE MIDI characteristic: 7772E5DB-3868-412A-A1A9-F2669D106BF3
const uint8_t BluetoothMIDIService::char_base_uuid[16] =
    { 0x77, 0x72, 0x00, 0x00, 0x38, 0x68, 0x41, 0x12,
      0xa1, 0xa9, 0xf2, 0x66, 0x9d, 0x10, 0x6b, 0xf3 };

const uint16_t BluetoothMIDIService::serviceUUID = 0x0e5a;
const uint16_t BluetoothMIDIService::charUUID[mbbs_cIdxCOUNT] = { 0xe5db };

BluetoothMIDIService::BluetoothMIDIService(BLEDevice &_ble)
{
    memset(midiBuffer, 0, sizeof(midiBuffer));
    firstRead = true;

    RegisterBaseUUID(service_base_uuid);
    CreateService(serviceUUID);
    uint8_t serviceUuidType = bs_uuid_type;

    RegisterBaseUUID(char_base_uuid);
    CreateCharacteristic(mbbs_cIdxMIDI, charUUID[mbbs_cIdxMIDI],
        midiBuffer, 0, sizeof(midiBuffer),
        microbit_propREAD | microbit_propREADAUTH | microbit_propNOTIFY);

    configureMidiAdvertising(serviceUuidType);

    if (MicroBitBLEManager::manager)
        MicroBitBLEManager::manager->servicesChanged();
}

void BluetoothMIDIService::onDataRead(microbit_onDataRead_t *params)
{
    if (params->handle == valueHandle(mbbs_cIdxMIDI) && firstRead)
    {
        notifyChrValue(mbbs_cIdxMIDI, midiBuffer, 0);
        firstRead = false;
        params->data = midiBuffer;
        params->length = 0;
        params->allow = true;
        params->update = true;
    }
}

void BluetoothMIDIService::onDisconnect(const microbit_ble_evt_t *p_ble_evt)
{
    (void)p_ble_evt;
    firstRead = true;
}

bool BluetoothMIDIService::connected()
{
    return getConnected();
}

void BluetoothMIDIService::sendMidiMessage(uint8_t data0)
{
    if (connected())
    {
        unsigned int ticks = (unsigned int)system_timer_current_time() & 0x1fff;
        midiBuffer[0] = 0x80 | ((ticks >> 7) & 0x3f);
        midiBuffer[1] = 0x80 | (ticks & 0x7f);
        midiBuffer[2] = data0;

        notifyChrValue(mbbs_cIdxMIDI, midiBuffer, 3);
    }
}

void BluetoothMIDIService::sendMidiMessage(uint8_t data0, uint8_t data1)
{
    if (connected())
    {
        unsigned int ticks = (unsigned int)system_timer_current_time() & 0x1fff;
        midiBuffer[0] = 0x80 | ((ticks >> 7) & 0x3f);
        midiBuffer[1] = 0x80 | (ticks & 0x7f);
        midiBuffer[2] = data0;
        midiBuffer[3] = data1;

        notifyChrValue(mbbs_cIdxMIDI, midiBuffer, 4);
    }
}

void BluetoothMIDIService::sendMidiMessage(uint8_t data0, uint8_t data1, uint8_t data2)
{
    if (connected())
    {
        unsigned int ticks = (unsigned int)system_timer_current_time() & 0x1fff;
        midiBuffer[0] = 0x80 | ((ticks >> 7) & 0x3f);
        midiBuffer[1] = 0x80 | (ticks & 0x7f);
        midiBuffer[2] = data0;
        midiBuffer[3] = data1;
        midiBuffer[4] = data2;

        notifyChrValue(mbbs_cIdxMIDI, midiBuffer, 5);
    }
}

//================================================================
#else // MICROBIT_CODAL
//================================================================

#include "MicroBit.h"
#include "pxt.h"

// MIDI characteristic
const uint8_t midiCharacteristicUuid[] = {
        0x77, 0x72, 0xe5, 0xdb, 0x38, 0x68, 0x41, 0x12,
        0xa1, 0xa9, 0xf2, 0x66, 0x9d, 0x10, 0x6b, 0xf3
};

// MIDI service
const uint8_t midiServiceUuid[] = {
        0x03, 0xb8, 0x0e, 0x5a, 0xed, 0xe8, 0x4b, 0x33,
        0xa7, 0x51, 0x6c, 0xe3, 0x4e, 0xc4, 0xc7, 0x00
};

BluetoothMIDIService::BluetoothMIDIService(BLEDevice &_ble): ble(_ble) {
    memset(midiBuffer, 0, sizeof(midiBuffer));
    firstRead = true;

    GattCharacteristic midiCharacteristic(midiCharacteristicUuid, midiBuffer, 0, sizeof(midiBuffer),
          GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
        | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
        );
    GattCharacteristic *midiChars[] = {&midiCharacteristic};

    midiCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);

    GattService midiService(midiServiceUuid, midiChars, sizeof(midiChars) / sizeof(GattCharacteristic *));

    ble.addService(midiService);

    midiCharacteristicHandle = midiCharacteristic.getValueHandle();

    ble.gattServer().onDataRead(this, &BluetoothMIDIService::onDataRead);
    ble.onDisconnection(this, &BluetoothMIDIService::onDisconnection);

    tick.start();
}

void BluetoothMIDIService::onDataRead(const GattReadCallbackParams* params)
{
    if (params->handle == midiCharacteristicHandle)
    {
        if (firstRead) {
            // send empty payload upon first connect
            ble.gattServer().notify(midiCharacteristicHandle, (uint8_t *)midiBuffer, 0);
            firstRead = false;
        }
    }
}

void BluetoothMIDIService::onDisconnection(const Gap::DisconnectionCallbackParams_t* params) {
    (void)params;
    firstRead = true;
}

bool BluetoothMIDIService::connected() {
    return ble.getGapState().connected;
}

void BluetoothMIDIService::sendMidiMessage(uint8_t data0) {
    if (connected()) {
        unsigned int ticks = tick.read_ms() & 0x1fff;
        midiBuffer[0] = 0x80 | ((ticks >> 7) & 0x3f);
        midiBuffer[1] = 0x80 | (ticks & 0x7f);
        midiBuffer[2] = data0;

        ble.gattServer().notify(midiCharacteristicHandle, (uint8_t *)midiBuffer, 3);
    }
}

void BluetoothMIDIService::sendMidiMessage(uint8_t data0, uint8_t data1) {
    if (connected()) {
        unsigned int ticks = tick.read_ms() & 0x1fff;
        midiBuffer[0] = 0x80 | ((ticks >> 7) & 0x3f);
        midiBuffer[1] = 0x80 | (ticks & 0x7f);
        midiBuffer[2] = data0;
        midiBuffer[3] = data1;

        ble.gattServer().notify(midiCharacteristicHandle, (uint8_t *)midiBuffer, 4);
    }
}

void BluetoothMIDIService::sendMidiMessage(uint8_t data0, uint8_t data1, uint8_t data2) {
    if (connected()) {
        unsigned int ticks = tick.read_ms() & 0x1fff;
        midiBuffer[0] = 0x80 | ((ticks >> 7) & 0x3f);
        midiBuffer[1] = 0x80 | (ticks & 0x7f);
        midiBuffer[2] = data0;
        midiBuffer[3] = data1;
        midiBuffer[4] = data2;

        ble.gattServer().notify(midiCharacteristicHandle, (uint8_t *)midiBuffer, 5);
    }
}

//================================================================
#endif // MICROBIT_CODAL
//================================================================
