
import React, { Component } from 'react';
import { Text, View, StyleSheet } from 'react-native';
import BigSlider from 'react-native-big-slider';
import { BleManager } from "react-native-ble-plx"

const styles = StyleSheet.create({
  root: {
    flex:1,
    justifyContent:'center',
    alignItems:'center',
    backgroundColor:'white',
    alignItems:'center'
  },
  container: {
    width:400,
    justifyContent:'center',
    alignItems:'flex-start',
    flexDirection:'row',
    padding:10,
  },
  slider: {
    flexDirection:'column',
    height:100,
    width:200,
  }
});


export default class EthanWheels extends Component {
  constructor() {
    super();
    this.manager = new BleManager();
    this.state = {
      speed:50,
      scanning:false
    };
  }
  
  scanAndConnect() {
    this.manager.startDeviceScan(null, null, (error, device) => {
      console.log(device)

      if (error) {
        console.log(error.message)
        return
      }
      console.log(device.name);
/*
      if (device.name === 'TI BLE Sensor Tag' || device.name === 'SensorTag') {
        this.manager.stopDeviceScan()
        device.connect()
          .then((device) => {
            this.info("Discovering services and characteristics")
            return device.discoverAllServicesAndCharacteristics()
          })
          .then((device) => {
            this.info("Setting notifications")
            return this.setupNotifications(device)
          })
          .then(() => {
            this.info("Listening...")
          }, (error) => {
            this.error(error.message)
          })
      }*/
    });
  }
  
  componentDidMount() {
    this.scanAndConnect();
  }
  
  render () {
    return (
      <View style={styles.root}>
        <View style={styles.container}>
          <Text style={{marginRight:10}}>Speed</Text>
          <BigSlider style={styles.slider} minimumValue={0} maximumValue={100}
            label={`${this.state.speed | 0}%`}
            value={this.state.speed} onValueChange={valA => {
              this.setState({speed: valA })
            }} />
        </View>
      </View>
    );
  }
}

