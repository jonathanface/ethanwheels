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
    private static final String POWERWHEELS_BLE_ADDRESS = "F8:30:02:00:1F:BC";

    private static final String POWERWHEELS_SERVICE = "0000ffe0-0000-1000-8000-00805f9b34fb";
    private static final String POWERWHEELS_MOTOR_CHARACTERISTIC = "0000ffe1-0000-1000-8000-00805f9b34fb";
    private int speed = 0;
    private BluetoothGattCharacteristic motorTX;
    private BluetoothGatt bluetoothGatt;
    private boolean deviceConnected = false;

    private static int COMMAND_SPEEDCHANGE = 100;

    private boolean writeSpeed() {
        // pipe char signifies end of message, b/c BLE is a fast,
        // constant stream, and sometimes multiple messages get appended
        motorTX.setValue(COMMAND_SPEEDCHANGE + "=" + speed + "|");
        Log.d("Sending speed", String.valueOf(speed));
        return bluetoothGatt.writeCharacteristic(motorTX);
    }

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
                if (!writeSpeed()) {
                    Log.d("WRITE", "failed to write");
                };
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
                        Log.d("scan", uuid.toString());
                        if (uuid.toString().equalsIgnoreCase(POWERWHEELS_SERVICE)) {
                            Log.d("service", POWERWHEELS_SERVICE);
                            List<BluetoothGattCharacteristic> chars = service.getCharacteristics();
                            for (int i=0; i < chars.size(); i++) {
                                for (BluetoothGattCharacteristic bc : chars) {
                                    Log.d("chars", bc.getUuid().toString());
                                }
                            }
                            Log.d("char", POWERWHEELS_MOTOR_CHARACTERISTIC);
                            motorTX = service.getCharacteristic(UUID.fromString(POWERWHEELS_MOTOR_CHARACTERISTIC));
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
            //Log.d("scanning", "name: " + result.getDevice().getName());
            //Log.d("scanning", "mac: " + result.getDevice().getAddress());
            if (result.getDevice().getAddress().equalsIgnoreCase(POWERWHEELS_BLE_ADDRESS)) {
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
