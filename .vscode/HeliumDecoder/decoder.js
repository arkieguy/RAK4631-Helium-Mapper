function Decoder(bytes, port) {
    // Decode plain text from hex.
    var sensor = {};
    
    sensor.latitude = ( bytes[0] | bytes[1] << 8 | bytes[2] << 16 | bytes[3] << 24 | (bytes[3] & 0x80 ? 0xFF << 24 : 0)) / 100000.0;
    
    sensor.longitude = (bytes[4] | bytes[5] << 8 | bytes[6] << 16 | bytes[7] << 24 | (bytes[7] & 0x80 ? 0xFF << 24 : 0)) / 100000.0;
    
    sensor.altitude = (bytes[8] | bytes[9] << 8 | (bytes[9] & 0x80 ? 0xFF << 16 : 0));
    
    sensor.speed = (bytes[12] | bytes[13] << 8 | (bytes[13] & 0x80 ? 0xFF << 16 : 0));
    
    sensor.char_a = bytes[10];
    sensor.char_b = bytes[11];
    sensor.char_c = bytes[12];
    sensor.char_d = bytes[13];
    sensor.char_e = bytes[14];
    sensor.char_f = bytes[15];
   
    sensor.hdop = bytes[10];
    sensor.battery = bytes[11] / 10;
    sensor.sats = bytes[14]; 
   
    return( sensor );
  }