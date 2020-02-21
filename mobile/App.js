import React, { Component } from 'react';
import {
  AppRegistry,
  StyleSheet,
  Text,
  View
} from 'react-native';

import BigSlider from 'react-native-big-slider'

export default class bigSlider extends Component {
  constructor() {
    super()
    this.state = { speed:50 }
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
    alignItems:'top',
    flexDirection:'row',
    padding:10,
  },
  slider: {
    flexDirection:'column',
    height:100,
    width:200,
  }
});

AppRegistry.registerComponent('bigSlider', () => bigSlider);