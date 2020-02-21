import React, { Component } from 'react';
import {
  StyleSheet,
  Text,
  View,
  NativeEventEmitter,
  NativeModules,
  Platform,
  PermissionsAndroid,
} from 'react-native';

import BigSlider from 'react-native-big-slider';
import BleManager from 'react-native-ble-manager';

const BleManagerModule = NativeModules.BleManager;
const bleManagerEmitter = new NativeEventEmitter(BleManagerModule);

export default class bigSlider extends Component {
  constructor() {
    super()
    this.state = {
      speed:50,
      scanning:false
    }

  }
  
  componentDidMount() {
    console.log('wtf', BleManager);
    BleManager.start({showAlert: false}).then(() => {
      // Success code
      console.log('Module initialized');
    });
    
    if (Platform.OS === 'android' && Platform.Version >= 23) {
        PermissionsAndroid.check(PermissionsAndroid.PERMISSIONS.ACCESS_COARSE_LOCATION).then((result) => {
            if (result) {
              console.log("Permission is OK");
            } else {
              PermissionsAndroid.requestPermission(PermissionsAndroid.PERMISSIONS.ACCESS_COARSE_LOCATION).then((result) => {
                if (result) {
                  console.log("User accept");
                } else {
                  console.log("User refuse");
                }
              });
            }
      });
    }
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
