package com.jface.ethanwheels;

import android.Manifest;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.DragEvent;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;
import androidx.appcompat.widget.AppCompatSeekBar;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    BluetoothManager btManager;
    BluetoothAdapter btAdapter;
    BluetoothLeScanner btScanner;
    Button startScanningButton;
    Button beepButton;
    TextView peripheralTextView;
    AppCompatSeekBar seekbar;

    private final static int REQUEST_ENABLE_BT = 1;
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;
    private static final String POWERWHEELS_BLE_ADDRESS = "00:35:FF:1F:74:4E";
    private static final String ICLIP_NAME = "Pi_iClip";
    private int speed = 0;
    private boolean isBeeping = false;
    private BluetoothGattCharacteristic beepTX;
    private BluetoothGatt bluetoothGatt;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        peripheralTextView = (TextView) findViewById(R.id.PeripheralTextView);
        peripheralTextView.setMovementMethod(new ScrollingMovementMethod());

        startScanningButton = (Button) findViewById(R.id.StartScanButton);
        startScanningButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                startScanning();
            }
        });

        beepButton = (Button) findViewById(R.id.beepButton);
        beepButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (isBeeping) {
                    String value= "00";
                    //byte[] value =
                    beepTX.setValue(new byte[]{0x00});
                    boolean status = bluetoothGatt.writeCharacteristic(beepTX);
                    Log.d("val", String.valueOf(status));
                } else {
                    String value = "01";
                    beepTX.setValue(new byte[]{0x01});
                    boolean status = bluetoothGatt.writeCharacteristic(beepTX);
                    Log.d("val", String.valueOf(status));

                }
                bluetoothGatt.writeCharacteristic(beepTX);
                isBeeping = !isBeeping;
            }
        });
        beepButton.setVisibility(View.INVISIBLE);

        seekbar = (AppCompatSeekBar) findViewById(R.id.speedAdjust);
        seekbar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
           @Override
           public void onStopTrackingTouch(SeekBar seekBar) {
               // TODO Auto-generated method stub
           }

           @Override
           public void onStartTrackingTouch(SeekBar seekBar) {
               // TODO Auto-generated method stub
           }

           @Override
           public void onProgressChanged(SeekBar seekBar, int progress,boolean fromUser) {
                Log.d("CHANGE", String.valueOf(progress));
                speed = progress;
           }
        });
        btManager = (BluetoothManager)getSystemService(Context.BLUETOOTH_SERVICE);
        btAdapter = btManager.getAdapter();
        btScanner = btAdapter.getBluetoothLeScanner();


        if (btAdapter != null && !btAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        }

        // Make sure we have access coarse location enabled, if not, prompt the user to enable it
        if (this.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            final AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle("This app needs location access");
            builder.setMessage("Please grant location access so this app can detect peripherals.");
            builder.setPositiveButton(android.R.string.ok, null);
            builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                @Override
                public void onDismiss(DialogInterface dialog) {
                    requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION);
                }
            });
            builder.show();
        }
    }

    private void showButton(){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                beepButton.setVisibility(View.VISIBLE);
            }
        });
    }


    private int connectionState = STATE_DISCONNECTED;

    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_CONNECTED = 2;

    private final static String TAG = MainActivity.class.getSimpleName();

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

    public static String HM_10 = "0000ffe1-0000-1000-8000-00805f9b34fb";
    private static HashMap<String, String> attributes = new HashMap();
    static {
        // Sample Services.
        attributes.put("0000ffe0-0000-1000-8000-00805f9b34fb", "HM-10 Service");
        attributes.put(HM_10, "HM-10 Module");
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {



        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            String intentAction;
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                intentAction = ACTION_GATT_CONNECTED;
                connectionState = STATE_CONNECTED;
                broadcastUpdate(intentAction);

                //peripheralTextView.append("Connected to GATT server.\n");
                Log.i(TAG, "Attempting to start service discovery:" + bluetoothGatt.discoverServices());



            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                intentAction = ACTION_GATT_DISCONNECTED;
                connectionState = STATE_DISCONNECTED;
                //peripheralTextView.append("Disconnected from GATT server.\n");
                broadcastUpdate(intentAction);
            }
        }

        @Override
        // New services discovered
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_GATT_SERVICES_DISCOVERED);
            } else {
                peripheralTextView.append("onServicesDiscovered received: " + String.valueOf(status));
            }
        }

        @Override
        // Result of a characteristic read operation
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                peripheralTextView.append("Read characteristic: " + characteristic.toString());
            }
        }
    };

    public void writeCharacteristic(BluetoothGattCharacteristic characteristic) {
        if (btAdapter == null || bluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }

        bluetoothGatt.writeCharacteristic(characteristic);
    }

    private void broadcastUpdate(final String action) {
        final Intent intent = new Intent(action);
        peripheralTextView.append("Discovered " + action + "\n");

        switch(action) {
            case ACTION_GATT_SERVICES_DISCOVERED:
                List<BluetoothGattService> gattServices = bluetoothGatt.getServices();
                Log.d("serviceLength", String.valueOf(gattServices.size()));
                for (BluetoothGattService gattService : gattServices) {
                    Log.d("services", gattService.toString());
                    HashMap<String, String> currentServiceData = new HashMap<String, String>();
                    UUID uuid = gattService.getUuid();
                    Log.d("service", uuid.toString());
                    peripheralTextView.append("Service: " + uuid + "\n");
                    if (uuid != null && uuid.toString().equalsIgnoreCase("00002547-1212-efde-2547-785feabcd123")) {
                        Log.d("service", "matched");
                        beepTX = gattService.getCharacteristic(UUID.fromString("0000254a-1212-efde-2547-785feabcd123"));
                        showButton();
                            //gattService.disconnect();

                    }
                    /*
                    currentServiceData.put(
                            LIST_NAME, SampleGattAttributes.lookup(uuid, unknownServiceString));

                    // If the service exists for HM 10 Serial, say so.
                    if(SampleGattAttributes.lookup(uuid, unknownServiceString) == "HM 10 Serial") { isSerial.setText("Yes, serial :-)"); } else {  isSerial.setText("No, serial :-("); }
                    currentServiceData.put(LIST_UUID, uuid);
                    gattServiceData.add(currentServiceData);

                    // get characteristic when UUID matches RX/TX UUID
                    characteristicTX = gattService.getCharacteristic(BluetoothLeService.UUID_HM_RX_TX);
                    characteristicRX = gattService.getCharacteristic(BluetoothLeService.UUID_HM_RX_TX);
                    */
                }
                //BluetoothGattService gattService
                //characteristicTX = gattService.getCharacteristic(HM_10);
                break;
        }
        sendBroadcast(intent);
    }



    private void broadcastUpdate(final String action, final BluetoothGattCharacteristic characteristic) {
        final Intent intent = new Intent(action);
        Log.v("AndroidLE", "broadcastUpdate()");

        final byte[] data = characteristic.getValue();

        Log.v("AndroidLE", "data.length: " + data.length);

        if (data != null && data.length > 0) {
            final StringBuilder stringBuilder = new StringBuilder(data.length);
            for(byte byteChar : data) {
                stringBuilder.append(String.format("%02X ", byteChar));

                peripheralTextView.append(String.format("%02X ", byteChar));
            }
            intent.putExtra(EXTRA_DATA, new String(data) + "\n" + stringBuilder.toString());
        }

        sendBroadcast(intent);
    }

    private void connectToPW(BluetoothDevice device) {
        bluetoothGatt = device.connectGatt(this, false, gattCallback);
        //connectionState = STATE_CONNECTING;

    }
    // Device scan callback.
    private ScanCallback leScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            //Log.d("scanning", "name: " + result.getDevice().getName());

            if (result.getDevice().getName() != null && result.getDevice().getName().equalsIgnoreCase(ICLIP_NAME)) {
            //if (result.getDevice().getAddress().equalsIgnoreCase(POWERWHEELS_BLE_ADDRESS)) {
                peripheralTextView.append("Found device, stopping scan and connecting.\n");
                stopScanning();
                connectToPW(result.getDevice());
                return;
            }
            final int scrollAmount = peripheralTextView.getLayout().getLineTop(peripheralTextView.getLineCount()) - peripheralTextView.getHeight();
            if (scrollAmount > 0) {
                peripheralTextView.scrollTo(0, scrollAmount);
            }

        }
    };

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        switch (requestCode) {
            case PERMISSION_REQUEST_COARSE_LOCATION: {
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    System.out.println("coarse location permission granted");
                } else {
                    final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setTitle("Functionality limited");
                    builder.setMessage("Since location access has not been granted, this app will not be able to discover beacons when in the background.");
                    builder.setPositiveButton(android.R.string.ok, null);
                    builder.setOnDismissListener(new DialogInterface.OnDismissListener() {

                        @Override
                        public void onDismiss(DialogInterface dialog) {
                        }

                    });
                    builder.show();
                }
                return;
            }
        }
    }

    public void startScanning() {
        peripheralTextView.setText("Scanning...\n");
        startScanningButton.setVisibility(View.INVISIBLE);
        AsyncTask.execute(new Runnable() {
            @Override
            public void run() {
                btScanner.startScan(leScanCallback);
            }
        });
    }

    public void stopScanning() {
        peripheralTextView.append("Stopped Scanning.\n");
        startScanningButton.setVisibility(View.VISIBLE);
        AsyncTask.execute(new Runnable() {
            @Override
            public void run() {
                btScanner.stopScan(leScanCallback);
            }
        });
    }
}
