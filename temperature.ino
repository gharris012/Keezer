
void resetOWN()
{
    byte i = 0;
    for ( i = 0 ; i < DS_SENSOR_COUNT ; i ++ )
    {
        ds_temp_sensor[i].tempF = 0;
        ds_temp_sensor[i].present = FALSE;
    }
}

void scanOWN()
{
    uint8_t addr[8];
    byte i = 0;
    bool found = FALSE;

    for ( i = 0 ; i < DS_SENSOR_COUNT ; i ++ )
    {
        ds_temp_sensor[i].present = FALSE;
    }

    Serial.println("Searching OWN");
    if ( own.reset() == 1 )
    {
        Serial.println("Network present");
    }
    else
    {
        Serial.println("Network problem :(");
    }

    own.reset_search();
    delay(250);
    // search own for sensors
    while(own.search(addr))
    {
        found = FALSE;
        Serial.print("Found: ");
        Serial.print(addr[6], HEX);
        Serial.print("-");
        Serial.print(addr[7], HEX);
        for ( i = 0 ; i < DS_SENSOR_COUNT ; i ++ )
        {
            if ( memcmp(addr, ds_temp_sensor[i].addr, 8) == 0 )
            {
                Serial.print(" ");
                Serial.print(ds_temp_sensor[i].name);
                Serial.print(" at index ");
                Serial.println(i);
                ds_temp_sensor[i].present = TRUE;
                found = TRUE;
            }
        }
        if ( ! found )
        {
            Serial.println("Not matched");
        }
    }
    own.reset_search();
}

void read_temperatures()
{
    Serial.println("Refreshing temperatures");
    own.reset();
    own.skip();
    own.write(0x44);
    own.reset();
    // schedule a reading from ds sensors
    //Serial.println("scheduling ds read");
    ds_temp_sensor_is_converting = TRUE;
    ds_temp_sensor_convert_complete_time = millis() + DS_TEMP_SENSOR_CONVERT_DURATION;

    byte i = 0;
}

void read_ds_temperatures()
{
    Serial.println("Reading ds temperatures");

    byte i = 0;
    byte present_count = 0;
    float therm = INVALID_TEMPERATURE;

    own.reset();
    for ( i = 0 ; i < DS_SENSOR_COUNT ; i ++ )
    {
        therm = INVALID_TEMPERATURE;
        if ( ds_temp_sensor[i].present )
        {
            present_count++;
            own.reset();
            own.select(ds_temp_sensor[i].addr);
            if ( own.read() )
            {
                // if at least one comes back, indicate that we are done converting
                ds_temp_sensor_is_converting = FALSE;
                therm = readTempC(&ds_temp_sensor[i]);
                ds_temp_sensor[i].last_tempF = therm;
                if ( therm != INVALID_TEMPERATURE )
                {
                    ds_temp_sensor[i].tempF = convertTempCtoF(therm);
                    ds_temp_sensor[i].last_valid_read = millis();
                }
            }
            else
            {
                ds_temp_sensor[i].last_tempF = INVALID_TEMPERATURE;
            }
            // if we didn't get a good read, and it's been more than
            //  DS_TEMP_GRACE_PERIOD millis, note it
            // but leave it.. it might come back!
            if ( ds_temp_sensor[i].last_tempF == INVALID_TEMPERATURE
                    && ds_temp_sensor[i].tempF != INVALID_TEMPERATURE
                    && ( ds_temp_sensor[i].last_valid_read + DS_TEMP_GRACE_PERIOD ) < millis() )
            {
                ds_temp_sensor[i].tempF = INVALID_TEMPERATURE;
                Serial.print(" ");
                Serial.print(ds_temp_sensor[i].name);
                Serial.println(" invalid for too long, removing");
            }
        }
    }
    // if there are none found, don't keep hammering the network
    if ( present_count == 0 )
    {
        Serial.println("No DS Temperatures Present!");
        ds_temp_sensor_is_converting = FALSE;
    }
}

float readTempC(DSTempSensor *dstemp)
{
    byte i;
    uint8_t data[12];
    float celsius;
    int16_t raw;

    own.reset();
    own.select(dstemp->addr);
    own.write(0xBE);

    for ( i = 0 ; i < 9 ; i ++ )
    {
        data[i] = own.read();
    }

    uint8_t crc = OneWire::crc8(data, 8);
    if ( crc != data[8] )
    {
        Serial.print(" invalid crc: ");
        Serial.print(crc);
        Serial.print(" != ");
        Serial.println(data[8]);
        return INVALID_TEMPERATURE;
    }

    raw = (data[1] << 8) | data[0];

    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if ( cfg == 0x00 )
    {
        raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    }
    else if (cfg == 0x20)
    {
         raw = raw & ~3; // 10 bit res, 187.5 ms
    }
    else if (cfg == 0x40)
    {
         raw = raw & ~1; // 11 bit res, 375 ms
    }

    celsius = (float)raw / 16.0;

    return celsius;
}

float convertTempCtoF(float tempC)
{
    return tempC * 1.8 + 32.0;
}
