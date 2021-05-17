function Decoder( bytes, port ) {
  
  var sensor = {};

  sensor.latitude = convertBin( bytes, 0, 4 ) / 100000.0;
  sensor.longitude = convertBin( bytes, 4, 4 ) / 100000.0;
  sensor.altitude = convertBin( bytes, 8, 2 );
  sensor.speed = convertBin( bytes, 12, 2);
  sensor.hdop = bytes[10];
  sensor.battery = bytes[11] / 10;
  sensor.sats = bytes[14];
  
  return( sensor );
}

//  Function to convert binary string back to numeric value
function convertBin( buffer, offset, len, format="LSB" ){
  var res = 0;
  for( k = offset; k < offset + len; k++ )
      res += buffer[k] << ( 8 * ( (format=="LSB") ? k : len - k ) );  
  return( res );
}