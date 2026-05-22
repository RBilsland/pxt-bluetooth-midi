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
#include "ble_gatt.h"

using namespace codal;

// Nordic BLE-MIDI guide: device name in advertising, 128-bit MIDI UUID in scan response.
static const uint8_t midi_scanrsp_uuid[] = {
    0x11, 0x07,
    0x00, 0xc7, 0xc4, 0x4e, 0xe3, 0x6c, 0x51, 0xa7,
    0x33, 0x4b, 0xe8, 0xed, 0x5a, 0x0e, 0xb8, 0x03,
};

void BluetoothMIDIService::requestMidiConnectionParams(microbit_gaphandle_t conn)
{
    if (conn == BLE_CONN_HANDLE_INVALID)
        return;

    ble_gap_conn_params_t params;
    memset(&params, 0, sizeof(params));
    params.min_conn_interval = 6;   // 7.5 ms
    params.max_conn_interval = 12;  // 15 ms (BLE MIDI spec maximum)
    params.slave_latency = 0;
    params.conn_sup_timeout = 400;
    sd_ble_gap_conn_param_update(conn, &params);
}

void BluetoothMIDIService::configureMidiAdvertising(uint8_t serviceUuidType)
{
    (void)serviceUuidType;

    uint8_t adv_handle = 0;
    uint8_t enc_adv[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
    uint16_t adv_len = sizeof(enc_adv);

    ble_advdata_t advdata;
    memset(&advdata, 0, sizeof(advdata));
    advdata.flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED | BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE;
    advdata.name_type = BLE_ADVDATA_FULL_NAME;
    uint32_t err = ble_advdata_encode(&advdata, enc_adv, &adv_len);
    if (err != NRF_SUCCESS) {
        adv_len = 3;
        enc_adv[0] = 0x02;
        enc_adv[1] = 0x01;
        enc_adv[2] = 0x06;
    }

    ble_gap_adv_data_t gap_adv_data;
    memset(&gap_adv_data, 0, sizeof(gap_adv_data));
    gap_adv_data.adv_data.p_data = enc_adv;
    gap_adv_data.adv_data.len = adv_len;
    gap_adv_data.scan_rsp_data.p_data = (uint8_t *)midi_scanrsp_uuid;
    gap_adv_data.scan_rsp_data.len = sizeof(midi_scanrsp_uuid);

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
    pendingHandshake = true;

    RegisterBaseUUID(service_base_uuid);
    CreateService(serviceUUID);
    midiAdvUuidType = bs_uuid_type;

    RegisterBaseUUID(char_base_uuid);
    uint16_t props = microbit_propREAD | microbit_propWRITE | microbit_propWRITE_WITHOUT | microbit_propNOTIFY;
    CreateCharacteristic(mbbs_cIdxMIDI, charUUID[mbbs_cIdxMIDI],
        midiBuffer, 0, sizeof(midiBuffer), props);

    configureMidiAdvertising(midiAdvUuidType);

    if (MicroBitBLEManager::manager)
        MicroBitBLEManager::manager->servicesChanged();
}

bool BluetoothMIDIService::onBleEvent(const microbit_ble_evt_t *p_ble_evt)
{
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONN_SEC_UPDATE:
            requestMidiConnectionParams(p_ble_evt->evt.gap_evt.conn_handle);
            break;
        default:
            break;
    }
    return MicroBitBLEService::onBleEvent(p_ble_evt);
}

void BluetoothMIDIService::completeMidiHandshake()
{
    // notifyChrValue() only works after the central writes CCCD (Subscribe).
    // macOS MIDI Studio often reads the characteristic first; an early notify
    // fails silently and used to clear pendingHandshake, so no notify was sent
    // after Subscribe — Connect then bounced back to Connect.
    if (!pendingHandshake || !getConnected())
        return;
    if (!chars[mbbs_cIdxMIDI].cccdNotify())
        return;
    if (notifyChrValue(mbbs_cIdxMIDI, midiBuffer, 0))
        pendingHandshake = false;
}

void BluetoothMIDIService::onConnect(const microbit_ble_evt_t *p_ble_evt)
{
    (void)p_ble_evt;
    pendingHandshake = true;
}

void BluetoothMIDIService::onDataRead(microbit_onDataRead_t *params)
{
    if (params->handle != valueHandle(mbbs_cIdxMIDI))
        return;

    params->data = midiBuffer;
    params->length = 0;
    params->allow = true;
    params->update = true;

    completeMidiHandshake();
}

void BluetoothMIDIService::onDataWritten(const microbit_ble_evt_write_t *params)
{
    microbit_charattr_t type;
    int idx = charHandleToIdx(params->handle, &type);
    if (idx < 0)
        return;

    if (type == microbit_charattrCCCD && params->len == 2) {
        uint16_t cccd = uint16_decode(params->data);
        if (cccd & BLE_GATT_HVX_NOTIFICATION)
            completeMidiHandshake();
    }
}

void BluetoothMIDIService::onDisconnect(const microbit_ble_evt_t *p_ble_evt)
{
    (void)p_ble_evt;
    pendingHandshake = true;
    configureMidiAdvertising(midiAdvUuidType);
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
    pendingHandshake = true;

    GattCharacteristic midiCharacteristic(midiCharacteristicUuid, midiBuffer, 0, sizeof(midiBuffer),
          GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
        | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE
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
        if (pendingHandshake) {
            ble.gattServer().notify(midiCharacteristicHandle, (uint8_t *)midiBuffer, 0);
            pendingHandshake = false;
        }
    }
}

void BluetoothMIDIService::onDisconnection(const Gap::DisconnectionCallbackParams_t* params) {
    (void)params;
    pendingHandshake = true;
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
