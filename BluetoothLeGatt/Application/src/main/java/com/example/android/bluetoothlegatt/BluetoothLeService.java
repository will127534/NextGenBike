/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.android.bluetoothlegatt;

import android.app.Activity;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.UUID;

/**
 * Service for managing connection and data communication with a GATT server hosted on a
 * given Bluetooth LE device.
 */
public class BluetoothLeService extends Service {

    public static final UUID DESCRIPTOR_UUID =          UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    public static final UUID NRF_SERVICE_UUID =         UUID.fromString("4607eda0-f65e-4d59-a9ff-84420d87a4ca");
    public static final UUID NRF_NAVIGATION_CHAR_UUID = UUID.fromString("4607eda1-f65e-4d59-a9ff-84420d87a4ca");
    public static final UUID NRF_BRAKE_CHAR_UUID =      UUID.fromString("4607eda2-f65e-4d59-a9ff-84420d87a4ca");

    public static final UUID BREAKOUT_SERVICE_UUID =    UUID.fromString("4607eda0-f65e-4d59-a9ff-84420d87a4ca");
    public static final UUID BREAKOUT_CHAR_UUID =       UUID.fromString("4607eda1-f65e-4d59-a9ff-84420d87a4ca");

    private final static String TAG = BluetoothLeService.class.getSimpleName();

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;
    private String mBluetoothDeviceAddress;
    private String mBluetoothDeviceAddressBreakout;
    private BluetoothGatt mBluetoothGatt;
    private BluetoothGatt mBluetoothGattBreakout;
    private int mConnectionState = STATE_DISCONNECTED;
    private int mConnectionStateBreakout = STATE_DISCONNECTED;
    private boolean mBreakoutConnected = false;

    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_CONNECTED = 2;

    public final static String ACTION_GATT_CONNECTED =
            "com.example.bluetooth.le.ACTION_GATT_CONNECTED";
    public final static String ACTION_GATT_DISCONNECTED =
            "com.example.bluetooth.le.ACTION_GATT_DISCONNECTED";
    public final static String ACTION_GATT_SERVICES_DISCOVERED =
            "com.example.bluetooth.le.ACTION_GATT_SERVICES_DISCOVERED";
    public final static String ACTION_DATA_AVAILABLE =
            "com.example.bluetooth.le.ACTION_DATA_AVAILABLE";
    public final static String EXTRA_DATA =
            "com.example.bluetooth.le.EXTRA_DATA";

    public final static UUID UUID_HEART_RATE_MEASUREMENT =
            UUID.fromString(SampleGattAttributes.HEART_RATE_MEASUREMENT);

    // Implements callback methods for GATT events that the app cares about.  For example,
    // connection change and services discovered.
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            String intentAction;
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                intentAction = ACTION_GATT_CONNECTED;
                mConnectionState = STATE_CONNECTED;
                broadcastUpdate(intentAction);
                Log.i(TAG, "Connected to GATT server.");
                // Attempts to discover services after successful connection.
                Log.i(TAG, "Attempting to start service discovery:" +
                        mBluetoothGatt.discoverServices());

            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                intentAction = ACTION_GATT_DISCONNECTED;
                mConnectionState = STATE_DISCONNECTED;
                Log.i(TAG, "Disconnected from GATT server.");
                broadcastUpdate(intentAction);
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_GATT_SERVICES_DISCOVERED);
                //setNotifySensor(gatt);
            } else {
                Log.w(TAG, "onServicesDiscovered received: " + status);
            }
        }
        private void setNotifySensor(BluetoothGatt gatt) {
            BluetoothGattCharacteristic characteristic = gatt.getService(NRF_SERVICE_UUID).getCharacteristic(NRF_BRAKE_CHAR_UUID);
            gatt.setCharacteristicNotification(characteristic, true);
            for (BluetoothGattDescriptor descriptor:characteristic.getDescriptors()){
                Log.d("BLE", "BluetoothGattDescriptor: "+descriptor.getUuid().toString());
            }
            Log.d("BLE", "Characteristic: " + characteristic.getUuid());
            BluetoothGattDescriptor desc = characteristic.getDescriptor(DESCRIPTOR_UUID);
            Log.d("BLE", "Descriptor is " + desc); // this is not null
            desc.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            Log.d("BLE", "Descriptor write: " + gatt.writeDescriptor(desc)); // returns true


        }
        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d("BT", "CharacteristicRead: "+characteristic.getValue());
                Log.d("BT", "Characteristic UUID: "+characteristic.getUuid());
                broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            Log.d("BT", "CharacteristicChanged UUID: "+characteristic.getUuid());
            if(characteristic.getUuid().equals(NRF_BRAKE_CHAR_UUID)){

                final byte[] data = characteristic.getValue();
                if (data != null && data.length > 0) {
                    final StringBuilder stringBuilder = new StringBuilder(data.length);
                    for(byte byteChar : data)
                        stringBuilder.append(String.format("%02X ", byteChar));
                    Log.d("BT", "CharacteristicChanged VALUE: "+stringBuilder);

                    Log.d("BT", "breakout connected? " + mBreakoutConnected);
                    //ByteBuffer wrapped = ByteBuffer.wrap(data); // big-endian by default
                    int dataInteger = data[0] & 0xFF;
                    Log.d("BT", "dataInteger: " + dataInteger);
                    if(mBreakoutConnected){
                        notifyBreakOut(dataInteger);
                    }

                }


            }
            //brakeToast(characteristic);

            broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
        }
    };

    public void toast(final String text){
            final Context context = getApplicationContext();
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    Toast toast = Toast.makeText(context, text, Toast.LENGTH_SHORT);
                    toast.show();
                }
            });
    }
    private final BluetoothGattCallback mGattCallbackBreakout = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            String intentAction;
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                intentAction = ACTION_GATT_CONNECTED;
                mConnectionStateBreakout = STATE_CONNECTED;
                broadcastUpdate(intentAction);
                Log.i(TAG, "Connected to GATT server.");
                // Attempts to discover services after successful connection.
                Log.i(TAG, "Attempting to start service discovery:" +
                        mBluetoothGattBreakout.discoverServices());

            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                intentAction = ACTION_GATT_DISCONNECTED;
                mConnectionStateBreakout = STATE_DISCONNECTED;
                Log.i(TAG, "Disconnected from GATT server.");
                broadcastUpdate(intentAction);
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_GATT_SERVICES_DISCOVERED);
            } else {
                Log.w(TAG, "onServicesDiscovered received: " + status);
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
            Log.d("BT", "BreakoutCharacteristicChanged: "+characteristic.getValue());
        }
    };
    private void broadcastUpdate(final String action) {
        final Intent intent = new Intent(action);
        sendBroadcast(intent);
    }

    private void broadcastUpdate(final String action,
                                 final BluetoothGattCharacteristic characteristic) {
        final Intent intent = new Intent(action);

        Log.d("BT", "broadcaseUpdate: "+characteristic.getValue());
        // This is special handling for the Heart Rate Measurement profile.  Data parsing is
        // carried out as per profile specifications:
        // http://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.heart_rate_measurement.xml
        if (UUID_HEART_RATE_MEASUREMENT.equals(characteristic.getUuid())) {
            int flag = characteristic.getProperties();
            int format = -1;
            if ((flag & 0x01) != 0) {
                format = BluetoothGattCharacteristic.FORMAT_UINT16;
                Log.d(TAG, "Heart rate format UINT16.");
            } else {
                format = BluetoothGattCharacteristic.FORMAT_UINT8;
                Log.d(TAG, "Heart rate format UINT8.");
            }
            final int heartRate = characteristic.getIntValue(format, 1);
            Log.d(TAG, String.format("Received heart rate: %d", heartRate));
            intent.putExtra(EXTRA_DATA, String.valueOf(heartRate));
        } else {
            // For all other profiles, writes the data formatted in HEX.
            final byte[] data = characteristic.getValue();
            if (data != null && data.length > 0) {
                final StringBuilder stringBuilder = new StringBuilder(data.length);
                for(byte byteChar : data)
                    stringBuilder.append(String.format("%02X ", byteChar));
                intent.putExtra(EXTRA_DATA, new String(data) + "\n" + stringBuilder.toString());
            }

        }
        sendBroadcast(intent);
    }

    public class LocalBinder extends Binder {
        BluetoothLeService getService() {
            return BluetoothLeService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        // After using a given device, you should make sure that BluetoothGatt.close() is called
        // such that resources are cleaned up properly.  In this particular example, close() is
        // invoked when the UI is disconnected from the Service.
        close();
        return super.onUnbind(intent);
    }

    private final IBinder mBinder = new LocalBinder();

    /**
     * Initializes a reference to the local Bluetooth adapter.
     *
     * @return Return true if the initialization is successful.
     */
    public boolean initialize() {
        // For API level 18 and above, get a reference to BluetoothAdapter through
        // BluetoothManager.
        if (mBluetoothManager == null) {
            mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
            if (mBluetoothManager == null) {
                Log.e(TAG, "Unable to initialize BluetoothManager.");
                return false;
            }
        }

        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Unable to obtain a BluetoothAdapter.");
            return false;
        }

        return true;
    }

    /**
     * Connects to the GATT server hosted on the Bluetooth LE device.
     *
     * @param address The device address of the destination device.
     *
     * @return Return true if the connection is initiated successfully. The connection result
     *         is reported asynchronously through the
     *         {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     *         callback.
     */
    public boolean connect(final String address) {
        if (mBluetoothAdapter == null || address == null) {
            Log.w(TAG, "BluetoothAdapter not initialized or unspecified address.");
            return false;
        }

        // Previously connected device.  Try to reconnect.
        if (mBluetoothDeviceAddress != null && address.equals(mBluetoothDeviceAddress)
                && mBluetoothGatt != null) {
            Log.d(TAG, "Trying to use an existing mBluetoothGatt for connection.");
            if (mBluetoothGatt.connect()) {
                mConnectionState = STATE_CONNECTING;
                return true;
            } else {
                return false;
            }
        }

        final BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address);
        if (device == null) {
            Log.w(TAG, "Device not found.  Unable to connect.");
            return false;
        }
        // We want to directly connect to the device, so we are setting the autoConnect
        // parameter to false.
        mBluetoothGatt = device.connectGatt(this, false, mGattCallback);
        Log.d(TAG, "Trying to create a new connection.");
        mBluetoothDeviceAddress = address;
        mConnectionState = STATE_CONNECTING;
        return true;
    }
    public boolean connectBreakout(final String address) {
        if (mBluetoothAdapter == null || address == null) {
            Log.d("BT", "Breakout connection failed!");
            Log.w(TAG, "BluetoothAdapter not initialized or unspecified address.");
            return false;
        }

        // Previously connected device.  Try to reconnect.
        if (mBluetoothDeviceAddressBreakout != null && address.equals(mBluetoothDeviceAddressBreakout)
                && mBluetoothGattBreakout != null) {
            Log.d(TAG, "Trying to use an existing mBluetoothGatt for connection.");
            if (mBluetoothGattBreakout.connect()) {
                mConnectionStateBreakout = STATE_CONNECTING;
                mBreakoutConnected = true;
                Log.d("BT", "Breakout connected!");
                toast("Breakout connected!");
                return true;
            } else {
                Log.d("BT", "Breakout connection failed!");
                toast("Breakout connection Failed!");
                return false;
            }
        }

        final BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address);
        if (device == null) {
            Log.w(TAG, "Device not found.  Unable to connect.");
            return false;
        }
        // We want to directly connect to the device, so we are setting the autoConnect
        // parameter to false.
        mBluetoothGattBreakout = device.connectGatt(this, false, mGattCallbackBreakout);
        Log.d(TAG, "Trying to create a new connection.");
        mBluetoothDeviceAddressBreakout = address;
        mConnectionStateBreakout = STATE_CONNECTING;
        return true;
    }
    /**
     * Disconnects an existing connection or cancel a pending connection. The disconnection result
     * is reported asynchronously through the
     * {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     * callback.
     */
    public void disconnect() {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.disconnect();
    }

    /**
     * After using a given BLE device, the app must call this method to ensure resources are
     * released properly.
     */
    public void close() {
        if (mBluetoothGatt == null) {
            return;
        }
        mBluetoothGatt.close();
        mBluetoothGatt = null;
        if (mBluetoothGattBreakout == null) {
            return;
        }
        mBluetoothGattBreakout.close();
        mBluetoothGattBreakout = null;
    }

    /**
     * Request a read on a given {@code BluetoothGattCharacteristic}. The read result is reported
     * asynchronously through the {@code BluetoothGattCallback#onCharacteristicRead(android.bluetooth.BluetoothGatt, android.bluetooth.BluetoothGattCharacteristic, int)}
     * callback.
     *
     * @param characteristic The characteristic to read from.
     */
    public void readCharacteristic(BluetoothGattCharacteristic characteristic) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.readCharacteristic(characteristic);
    }

    /**
     * Enables or disables notification on a give characteristic.
     *
     * @param characteristic Characteristic to act on.
     * @param enabled If true, enable notification.  False otherwise.
     */
    public void setCharacteristicNotification(BluetoothGattCharacteristic characteristic,
                                              boolean enabled) {
        Log.d("BT", "setCN");
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.setCharacteristicNotification(characteristic, enabled);

        // This is specific to Heart Rate Measurement.
        if (UUID_HEART_RATE_MEASUREMENT.equals(characteristic.getUuid())) {
            BluetoothGattDescriptor descriptor = characteristic.getDescriptor(
                    UUID.fromString(SampleGattAttributes.CLIENT_CHARACTERISTIC_CONFIG));
            descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            mBluetoothGatt.writeDescriptor(descriptor);
        }
    }

    /**
     * Retrieves a list of supported GATT services on the connected device. This should be
     * invoked only after {@code BluetoothGatt#discoverServices()} completes successfully.
     *
     * @return A {@code List} of supported services.
     */
    public List<BluetoothGattService> getSupportedGattServices() {
        if (mBluetoothGatt == null) return null;

        return mBluetoothGatt.getServices();
    }

    public boolean right(){
        BluetoothGattCharacteristic characteristic =
                mBluetoothGatt.getService(NRF_SERVICE_UUID)
                        .getCharacteristic(NRF_NAVIGATION_CHAR_UUID);
        characteristic.setValue(1,android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT8,0);
        if(!mBluetoothGatt.writeCharacteristic(characteristic)){
            Log.d("BT", "Failed to write characteristic 1");
            return false;
        }else{
            Log.d("BT", "Succeed to write characteristic 1");
            return true;
        }
    }
    public boolean left(){
        BluetoothGattCharacteristic characteristic =
                mBluetoothGatt.getService(NRF_SERVICE_UUID)
                        .getCharacteristic(NRF_NAVIGATION_CHAR_UUID);
        characteristic.setValue(2,android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT8,0);
        if(!mBluetoothGatt.writeCharacteristic(characteristic)){
            Log.d("BT", "Failed to write characteristic 2");
            return false;
        }else{

            Log.d("BT", "Succeed to write characteristic 2");
            return true;
        }
    }
    public boolean arrived(){
        BluetoothGattCharacteristic characteristic =
                mBluetoothGatt.getService(NRF_SERVICE_UUID)
                        .getCharacteristic(NRF_NAVIGATION_CHAR_UUID);
        characteristic.setValue(3,android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT8,0);
        if(!mBluetoothGatt.writeCharacteristic(characteristic)){
            Log.d("BT", "Failed to write characteristic 3");
            return false;
        }else{

            Log.d("BT", "Succeed to write characteristic 3");
            return true;
        }
    }

    public boolean setCharacteristicNotificationButton(){

        Log.d("BT", "setCN");
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return false;
        }
        BluetoothGattCharacteristic characteristic = mBluetoothGatt.getService(NRF_SERVICE_UUID).getCharacteristic(NRF_BRAKE_CHAR_UUID);

        mBluetoothGatt.setCharacteristicNotification(characteristic, true);

        Log.d("BLE", "Characteristic UUID: " + characteristic.getUuid());
        for (BluetoothGattDescriptor descriptor:characteristic.getDescriptors()){
            Log.d("BLE", "BluetoothGattDescriptor: "+descriptor.getUuid().toString());
        }
        BluetoothGattDescriptor desc = characteristic.getDescriptor(DESCRIPTOR_UUID);
        Log.d("BLE", "Descriptor is " + desc); // this is not null
        desc.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        Log.d("BLE", ""+BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        Log.d("BLE", "Descriptor write: " + mBluetoothGatt.writeDescriptor(desc)); // returns true
        return true;
    }
    public void notifyBreakOut(int data){


        Log.d("BT", "Into notify breakout with data: " + data);
        BluetoothGattCharacteristic characteristic =
                mBluetoothGattBreakout.getService(BREAKOUT_SERVICE_UUID)
                        .getCharacteristic(BREAKOUT_CHAR_UUID);
        characteristic.setValue(data,android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT8,0);


        if(!mBluetoothGattBreakout.writeCharacteristic(characteristic)){
            Log.d("BT", "Failed to notify breakout");
        }else{
            Log.d("BT", "Succeed to notify breakout");
        }
    }
}
