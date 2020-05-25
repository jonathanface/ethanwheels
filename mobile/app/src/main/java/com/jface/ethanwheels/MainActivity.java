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
import android.opengl.Visibility;
import android.os.AsyncTask;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.DragEvent;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import androidx.appcompat.widget.AppCompatSeekBar;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    BluetoothManager btManager;
    BluetoothAdapter btAdapter;
    BluetoothLeScanner btScanner;
    Button startScanningButton;
    TextView peripheralTextView;
    RelativeLayout controls;
    AppCompatSeekBar seekbar;

    private final static int REQUEST_ENABLE_BT = 1;
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;
    private static final String POWERWHEELS_BLE_ADDRESS = "F8:30:02:00:1F:BC";

    private static final String POWERWHEELS_SERVICE = "0000ffe0-0000-1000-8000-00805f9b34fb";
    private static final String POWERWHEELS_MOTOR_CHARACTERISTIC = "0000ffe1-0000-1000-8000-00805f9b34fb";
    private static final String POWERWHEELS_DESCRIPTOR = "00002902-0000-1000-8000-00805f9b34fb";
    private static final String BUTTON_LABEL_SCAN = "Scan";
    private static final String BUTTON_LABEL_DISCONNECT = "Disconnect";

    private int speed = 25;
    private BluetoothGattCharacteristic motorTX;
    private BluetoothGatt bluetoothGatt;
    private boolean deviceConnected = false;

    // pipe char signifies end of message, b/c BLE is a fast,
    // constant stream, and sometimes multiple messages get appended
    private static final String BLE_DELIMITER = "|";

    private final String STATUS_PARAM_SPEED = "speed";
    private final String STATUS_PARAM_LIGHTS = "lights";
    private final String STATUS_PARAM_HAZARDS = "hazards";

    // Communication enums.
    // Not using actual java enums b/c they're memory hogs in android
    private final int COMMAND_REQUEST_STATUS = 100;
    private final int COMMAND_REQUEST_STATUS_REPLY = 101;
    private final int COMMAND_DISCONNECT = 102;
    private final int COMMAND_SPEED_CHANGE = 200;
    private final int COMMAND_FORWARD = 300;
    private final int COMMAND_REVERSE = 301;
    private final int COMMAND_LEFT = 302;
    private final int COMMAND_RIGHT = 303;
    private final int COMMAND_STOP = 304;

    private boolean writeJeepMaxSpeed() {

        motorTX.setValue(COMMAND_SPEED_CHANGE + "=" + speed + BLE_DELIMITER);
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
                if (deviceConnected) {
                    bleDisconnect();
                } else {
                    startScanning();
                }
            }
        });

        controls = (RelativeLayout) findViewById(R.id.controls);

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
                TextView display = (TextView) findViewById(R.id.speedDisplay);
                display.setText(String.valueOf(speed) + "%");
                if (!writeJeepMaxSpeed()) {
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

    private void updateStatusText(final String text){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                peripheralTextView.append(text);
            }
        });
    }
    private void clearStatusText() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                peripheralTextView.setText("");
            }
        });
    }

    private void toggleControlsVisibility(final int state) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                controls.setVisibility(state);
            }
        });
    }
    private void updateScanButtonText(final String text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                startScanningButton.setText(text);
            }
        });
    }



    private void requestJeepStatus() {
        Log.d("sending status", String.valueOf(COMMAND_REQUEST_STATUS));
        String message = String.valueOf(COMMAND_REQUEST_STATUS) + BLE_DELIMITER;
        motorTX.setValue(message);
        Log.d("comm", "Sending status request");
        bluetoothGatt.writeCharacteristic(motorTX);
    }

    private void processStatusParam(String type, String val) {
        switch (type) {
            case STATUS_PARAM_SPEED:
                speed = Integer.parseInt(val);
                seekbar.setProgress(speed);
                updateStatusText("Received max speed setting from remote device.\n");
                break;
            case STATUS_PARAM_LIGHTS:
                updateStatusText("Received headlights status from remote device.\n");
                break;
            case STATUS_PARAM_HAZARDS:
                updateStatusText("Received hazard lights status from remote device.\n");
                break;
        }
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {

        private String characteristicChangedText = "";

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            String intentAction;
            Log.d("state", String.valueOf(newState));
            Log.d("servicestate", String.valueOf(BluetoothProfile.STATE_CONNECTED));
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                deviceConnected = true;
                updateScanButtonText(BUTTON_LABEL_DISCONNECT);

                updateStatusText("Connected to GATT server.\n");
                if (!bluetoothGatt.discoverServices()) {
                    bleDisconnect();
                }
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                deviceConnected = false;
                toggleControlsVisibility(View.INVISIBLE);
                updateScanButtonText(BUTTON_LABEL_SCAN);
                clearStatusText();
                updateStatusText("Disconnected from GATT server.\n");
                bluetoothGatt.close();
            }
        }

        @Override
        // New services discovered
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            Log.d("services", "discovered");
            boolean foundMatch = false;
            if (status == BluetoothGatt.GATT_SUCCESS) {
                for (BluetoothGattService service : gatt.getServices()) {
                    UUID uuid = service.getUuid();
                    if (uuid != null) {
                        updateStatusText("Service found: " + uuid + "\n");
                        Log.d("scan", uuid.toString());
                        if (uuid.toString().equalsIgnoreCase(POWERWHEELS_SERVICE)) {
                            Log.d("service", POWERWHEELS_SERVICE);
                            List<BluetoothGattCharacteristic> chars = service.getCharacteristics();
                            for (int i=0; i < chars.size(); i++) {
                                for (BluetoothGattCharacteristic bc : chars) {
                                    Log.d("chars", bc.getUuid().toString());
                                    if (bc.getUuid().toString().equalsIgnoreCase(POWERWHEELS_MOTOR_CHARACTERISTIC)) {
                                        motorTX = bc;
                                        foundMatch = true;
                                        gatt.setCharacteristicNotification(motorTX, true);
                                        BluetoothGattDescriptor descriptor = motorTX.getDescriptor(UUID.fromString(POWERWHEELS_DESCRIPTOR));
                                        descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                                        gatt.writeDescriptor(descriptor);
                                        break;
                                    }
                                }
                                if (foundMatch) {
                                    return;
                                }
                            }
                        }
                    }
                }
            } else {
                updateStatusText("Service error: " + String.valueOf(status) + "\n");
                gatt.close();
            }
        }
        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d("comm", "Callback: Wrote GATT Descriptor successfully.");
                requestJeepStatus();
            } else {
                Log.d("comm", "Callback: Error writing GATT Descriptor: " + status);
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            String val = characteristic.getStringValue(0).trim();
            if (val.length() > 0) {
                Log.d("comm", "Change: " + val);
                int ender = val.indexOf(BLE_DELIMITER);
                if (ender == -1) {
                    characteristicChangedText += val;
                    return;
                } else {
                    characteristicChangedText += val.substring(0, ender);
                }
                String[] separated = characteristicChangedText.split("&");
                Log.d("comm", "changedappended: " + characteristicChangedText);
                switch (Integer.parseInt(separated[0])) {
                    case COMMAND_REQUEST_STATUS_REPLY: {
                        for (int i=1; i < separated.length; i++) {
                            String[] params = separated[i].split("=");
                            processStatusParam(params[0].trim(), params[1].trim());
                        }
                        toggleControlsVisibility(View.VISIBLE);
                    }
                }
            }
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,
                                          BluetoothGattCharacteristic characteristic,
                                          int status) {
            Log.d("comm", "results: " + String.valueOf(status));
            if (status == BluetoothGatt.GATT_SUCCESS) {
              //gatt.readCharacteristic(motorRX);
            }
        }

        @Override
        // Result of a characteristic read operation
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                final byte[] data = characteristic.getValue();
                if (data != null && data.length > 0) {
                    final StringBuilder stringBuilder = new StringBuilder(data.length);
                    for (byte byteChar : data) {
                        stringBuilder.append(byteChar);
                    }
                    final String strReceived = stringBuilder.toString();
                    Log.d("comm", strReceived);
                }
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
            updateStatusText("Unable to scan. Error received was: " + String.valueOf(errorCode) + "\n");
        }

        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            if (result.getDevice().getAddress().equalsIgnoreCase(POWERWHEELS_BLE_ADDRESS)) {
                updateStatusText("Found device, stopping scan and discovering services.\n");
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

    private void bleDisconnect() {
        if (deviceConnected) {
            motorTX.setValue(COMMAND_DISCONNECT + BLE_DELIMITER);
            bluetoothGatt.writeCharacteristic(motorTX);
            startScanningButton.setText("Scan");
            bluetoothGatt.disconnect();
            bluetoothGatt.close();
            deviceConnected = false;
        }
    }

    public void startScanning() {
        stopScanning();
        bleDisconnect();
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
