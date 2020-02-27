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
    private static final String ICLIP_SERVICE = "00002547-1212-efde-2547-785feabcd123";
    private static final String ICLIP_BEEP_CHARACTERISTIC = "0000254a-1212-efde-2547-785feabcd123";
    private int speed = 0;
    private boolean isBeeping = false;
    private BluetoothGattCharacteristic beepTX;
    private BluetoothGatt bluetoothGatt;
    private boolean deviceConnected = false;

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
                    beepTX.setValue(new byte[]{0x00});
                } else {
                    beepTX.setValue(new byte[]{0x01});
                }
                boolean status = bluetoothGatt.writeCharacteristic(beepTX);
                if (!status) {
                    peripheralTextView.append("Unable to write to clip.");
                } else {
                    isBeeping = !isBeeping;
                }
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

    private void updateText(final String text){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                peripheralTextView.append(text);
            }
        });
    }


    private int connectionState = STATE_DISCONNECTED;

    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTED = 2;

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            String intentAction;
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                deviceConnected = true;
                updateText("Connected to GATT server.\n");
                bluetoothGatt.discoverServices();
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                deviceConnected = false;
                updateText("Disconnected from GATT server.\n");
            }
        }

        @Override
        // New services discovered
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                for (BluetoothGattService service : gatt.getServices()) {
                    UUID uuid = service.getUuid();
                    if (uuid != null) {
                        updateText("Service found: " + uuid + "\n");
                        if (uuid.toString().equalsIgnoreCase(ICLIP_SERVICE)) {
                            beepTX = service.getCharacteristic(UUID.fromString(ICLIP_BEEP_CHARACTERISTIC));
                            showButton();
                            return;
                        }
                    }

                }
            } else {
                updateText("Service error: " + String.valueOf(status) + "\n");
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

    private void connectToPW(BluetoothDevice device) {
        bluetoothGatt = device.connectGatt(this, false, gattCallback);
    }
    // Device scan callback.
    private ScanCallback leScanCallback = new ScanCallback() {
        @Override
        public void onScanFailed(int errorCode) {
            updateText("Unable to scan. Error received was: " + String.valueOf(errorCode) + "\n");
        }

        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            Log.d("scanning", "name: " + result.getDevice().getName());

            if (result.getDevice().getName() != null && result.getDevice().getName().equalsIgnoreCase(ICLIP_NAME)) {
            //if (result.getDevice().getAddress().equalsIgnoreCase(POWERWHEELS_BLE_ADDRESS)) {
                updateText("Found device, stopping scan and discovering services.\n");
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

    private void disconnectedClip() {
        if (deviceConnected) {
            bluetoothGatt.disconnect();
            deviceConnected = false;
        }
    }

    public void startScanning() {
        stopScanning();
        disconnectedClip();
        peripheralTextView.setText("Scanning...\n");
        AsyncTask.execute(new Runnable() {
            @Override
            public void run() {
                btScanner.startScan(leScanCallback);
            }
        });
    }

    public void stopScanning() {
        peripheralTextView.append("Stopped Scanning.\n");
        AsyncTask.execute(new Runnable() {
            @Override
            public void run() {
                btScanner.stopScan(leScanCallback);
            }
        });
    }
}
