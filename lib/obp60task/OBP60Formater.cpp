#ifdef BOARD_NODEMCU32S_OBP60

#include <Arduino.h>
#include "GwApi.h"
#include "Pagedata.h"

// ToDo
// simulation data
// hold values by missing data

FormatedData formatValue(GwApi::BoatValue *value, CommonData &commondata){
    GwLog *logger = commondata.logger;
    FormatedData result;
    static int dayoffset = 0;

    // Load configuration values
    String stimeZone = commondata.config->getString(commondata.config->timeZone);               // [UTC -14.00...+12.00]
    double timeZone = stimeZone.toDouble();
    String lengthFormat = commondata.config->getString(commondata.config->lengthFormat);        // [m|ft]
    String distanceFormat = commondata.config->getString(commondata.config->distanceFormat);    // [m|km|nm]
    String speedFormat = commondata.config->getString(commondata.config->speedFormat);          // [m/s|km/h|kn]
    String windspeedFormat = commondata.config->getString(commondata.config->windspeedFormat);  // [m/s|km/h|kn|bft]
    String tempFormat = commondata.config->getString(commondata.config->tempFormat);            // [K|°C|°F]
    String dateFormat = commondata.config->getString(commondata.config->dateFormat);            // [DE|GB|US]
    bool usesimudata = commondata.config->getBool(commondata.config->useSimuData);              // [on|off]

    // If boat value not valid
    if (! value->valid && !usesimudata){
        result.svalue = "---";
        return result;
    }
    
//    LOG_DEBUG(GwLog::DEBUG,"formatValue init: getFormat: %s date->value: %f time->value: %f", value->getFormat(), commondata.date->value, commondata.time->value);
    static const int bsize = 30;
    char buffer[bsize+1];
    buffer[0]=0;

    //########################################################
    // Formats for several boat data
    //########################################################
    if (value->getFormat() == "formatDate"){
        
        int dayoffset = 0;
        if (commondata.time->value + int(timeZone*3600) > 86400) {dayoffset = 1;}
        if (commondata.time->value + int(timeZone*3600) < 0) {dayoffset = -1;}

//        LOG_DEBUG(GwLog::DEBUG,"... formatDate value->value: %f tz: %f dayoffset: %d", value->value, timeZone, dayoffset);

        tmElements_t parts;
        time_t tv=tNMEA0183Msg::daysToTime_t(value->value + dayoffset);
        tNMEA0183Msg::breakTime(tv,parts);
        if(usesimudata == false) { 
            if(String(dateFormat) == "DE"){
                snprintf(buffer,bsize,"%02d.%02d.%04d",parts.tm_mday,parts.tm_mon+1,parts.tm_year+1900);
            }
            else if(String(dateFormat) == "GB"){
                snprintf(buffer,bsize,"%02d/%02d/%04d",parts.tm_mday,parts.tm_mon+1,parts.tm_year+1900);
            }
            else if(String(dateFormat) == "US"){
                snprintf(buffer,bsize,"%02d/%02d/%04d",parts.tm_mon+1,parts.tm_mday,parts.tm_year+1900);
            }
            else{
                snprintf(buffer,bsize,"%02d.%02d.%04d",parts.tm_mday,parts.tm_mon+1,parts.tm_year+1900);  
            }
        }
        else{
            snprintf(buffer,bsize,"01.01.2022"); 
        }
        if(timeZone == 0){
            result.unit = "UTC";
        }
        else{
            result.unit = "LOT";
        }
    }
    //########################################################
    else if(value->getFormat() == "formatTime"){
        double timeInSeconds = 0;
        double inthr = 0;
        double intmin = 0;
        double intsec = 0;
        double val = 0;

        timeInSeconds = value->value + int(timeZone * 3600);
        if (timeInSeconds > 86400) {timeInSeconds = timeInSeconds - 86400;}
        if (timeInSeconds <  0) {timeInSeconds = timeInSeconds + 86400;}
//        LOG_DEBUG(GwLog::DEBUG,"... formatTime value: %f tz: %f corrected timeInSeconds: %f ", value->value, timeZone, timeInSeconds);
        if(usesimudata == false) {
            val=modf(timeInSeconds/3600.0,&inthr);
            val=modf(val*3600.0/60.0,&intmin);
            modf(val*60.0,&intsec);
            snprintf(buffer,bsize,"%02.0f:%02.0f:%02.0f",inthr,intmin,intsec);
        }
        else{
            static long sec;
            static long lasttime;
            if(millis() > lasttime + 990){
                sec ++;
            }
            sec = sec % 60;
            snprintf(buffer,bsize,"11:36:%02i", int(sec));
            lasttime = millis();
        }
        if(timeZone == 0){
            result.unit = "UTC";
        }
        else{
            result.unit = "LOT";
        }
    }
    //########################################################
    else if (value->getFormat() == "formatFixed0"){
        if(usesimudata == false) {
            snprintf(buffer,bsize,"%3.0f",value->value);
        }
        else{
            snprintf(buffer,bsize,"%3.0f", 8.0 + float(random(0, 10)) / 10.0);
        }
        result.unit = "";
    }
    //########################################################
    else if (value->getFormat() == "formatCourse" || value->getFormat() == "formatWind"){
        double course = 0;
        if(usesimudata == false) {
            course = value->value;
        }
        else{
            course = 2.53 + float(random(0, 10) / 100.0);
        }    
        course = course * 57.2958;      // Unit conversion form rad to deg

        // Format 3 numbers with prefix zero
        snprintf(buffer,bsize,"%03.0f",course);
        result.unit = "Deg";
    }
    //########################################################
    else if (value->getFormat() == "formatKnots"){
        double speed = 0;
        if(usesimudata == false) {
            speed = value->value;
        }
        else{
            speed = 4.0 + float(random(0, 40));
        }
        if((String(speedFormat) == "km/h" || String(windspeedFormat) == "km/h") && String(windspeedFormat) != "bft"){
        speed = speed * 3.6;        // Unit conversion form m/s to km/h
            result.unit = "m/s";
        }
        else if((String(speedFormat) == "kn" || String(windspeedFormat) == "kn") && String(windspeedFormat) != "bft"){
            speed = speed * 1.94384;      // Unit conversion form m/s to kn
            result.unit = "kn";
        }
        else if(String(windspeedFormat) == "bft"){
            if(speed < 0.3){
                speed = 0;
            }
            if(speed >=0.3 && speed < 1.5){
                speed = 1;
            }
            if(speed >=1.5 && speed < 3.3){
                speed = 2;
            }
            if(speed >=3.3 && speed < 5.4){
                speed = 3;
            }
            if(speed >=5.4 && speed < 7.9){
                speed = 4;
            }
            if(speed >=7.9 && speed < 10.7){
                speed = 5;
            }
            if(speed >=10.7 && speed < 13.8){
                speed = 6;
            }
            if(speed >=13.8 && speed < 17.1){
                speed = 7;
            }
            if(speed >=17.1 && speed < 20.7){
                speed = 8;
            }
            if(speed >=20.7 && speed < 24.4){
                speed = 9;
            }
            if(speed >=24.4 && speed < 28.4){
                speed = 10;
            }
            if(speed >=28.4 && speed < 32.6){
                speed = 11;
            }
            if(speed >=32.6){
                speed = 12;
            }
            result.unit = "bft";
        }
        else{
            speed = speed;              // Unit conversion form m/s to m/s
            result.unit = "m/s";
        }
        if(String(windspeedFormat) == "bft"){
            snprintf(buffer,bsize,"%2.0f",speed);
        }
        else{
            if(speed < 10){
                snprintf(buffer,bsize,"%3.2f",speed);
            }
            if(speed >= 10 && speed < 100){
                snprintf(buffer,bsize,"%3.1f",speed);
            }
            if(speed >= 100){
                snprintf(buffer,bsize,"%3.0f",speed);
            }
        }
    }
    //########################################################
    else if (value->getFormat() == "formatRot"){
        double rotation = 0;
        if(usesimudata == false) {
            rotation = value->value;
        }
        else{
            rotation = 0.04 + float(random(0, 10)) / 100.0;
        }
        rotation = rotation * 57.2958;      // Unit conversion form rad/s to deg/s
        result.unit = "Deg/s";
        if(rotation < -100){
            rotation = -99;
        }
        if(rotation > 100){
            rotation = 99;
        }
        if(rotation > -10 && rotation < 10){
            snprintf(buffer,bsize,"%3.2f",rotation);
        }
        if(rotation <= -10 || rotation >= 10){
            snprintf(buffer,bsize,"%3.0f",rotation);
        }
    }
    //########################################################
    else if (value->getFormat() == "formatDop"){
        double dop = 0;
        if(usesimudata == false) {
            dop = value->value;
        }
        else{
            dop = 2.0 + float(random(0, 40)) / 10.0;
        }
        result.unit = "m";
        if(dop > 99.9){
            dop = 99.9;
        }
        if(dop < 10){
            snprintf(buffer,bsize,"%3.2f",dop);
        }
        if(dop >= 10 && dop < 100){
            snprintf(buffer,bsize,"%3.1f",dop);
        }
    }
    //########################################################
    else if (value->getFormat() == "formatLatitude"){
        if(usesimudata == false) {
            double lat = value->value;
            String latitude = "";
            String latdir = "";
            float degree = abs(int(lat));
            float minute = abs((lat - int(lat)) * 60);
            if(lat > 0){
                latdir = "N";
            }
            else{
                latdir = "S";
            }
            latitude = String(degree,0) + "\" " + String(minute,4) + "' " + latdir;
            result.unit = "";
            strcpy(buffer, latitude.c_str());
        }
        else{
            snprintf(buffer,bsize," 51\" %2.4f' N", 35.0 + float(random(0, 10)) / 10000.0);
        }
    }
    //########################################################
    else if (value->getFormat() == "formatLongitude"){
        if(usesimudata == false) {
            double lon = value->value;
            String longitude = "";
            String londir = "";
            float degree = abs(int(lon));
            float minute = abs((lon - int(lon)) * 60);
            if(lon > 0){
                londir = "E";
            }
            else{
                londir = "W";
            }
            longitude = String(degree,0) + "\" " + String(minute,4) + "' " + londir;
            result.unit = "";
            strcpy(buffer, longitude.c_str());
        }
        else{
            snprintf(buffer,bsize," 15\" %2.4f'", 6.0 + float(random(0, 10)) / 100000.0);
        }
    }
    //########################################################
    else if (value->getFormat() == "formatDepth"){
        double depth = 0;
        if(usesimudata == false) {
            depth = value->value;
        }
        else{
            depth = 18.0 + float(random(0, 100)) / 10.0;
        }
        if(String(lengthFormat) == "ft"){
            depth = depth * 3.28084;
            result.unit = "ft";
        }
        else{
            result.unit = "m";
        }
        if(depth < 10){
            snprintf(buffer,bsize,"%3.2f",depth);
        }
        if(depth >= 10 && depth < 100){
            snprintf(buffer,bsize,"%3.1f",depth);
        }
        if(depth >= 100){
            snprintf(buffer,bsize,"%3.0f",depth);
        }
    }
    //########################################################
    else if (value->getFormat() == "kelvinToC"){
        double temp = 0;
        if(usesimudata == false) {
            temp = value->value;
        }
        else{
            temp = 296.0 + float(random(0, 10)) / 10.0;
        }
        if(String(tempFormat) == "C"){
            temp = temp - 273.15;
            result.unit = "C";
        }
        else if(String(tempFormat) == "F"){
            temp = temp - 459.67;
            result.unit = "F";
        }
        else{
            result.unit = "K";
        }
        if(temp < 10){
            snprintf(buffer,bsize,"%3.2f",temp);
        }
        if(temp >= 10 && temp < 100){
            snprintf(buffer,bsize,"%3.1f",temp);
        }
        if(temp >= 100){
            snprintf(buffer,bsize,"%3.0f",temp);
        }
    }
    //########################################################
    else if (value->getFormat() == "mtr2nm"){
        double distance = 0;
        if(usesimudata == false) {
            distance = value->value;
        }
        else{
            distance = 2960.0 + float(random(0, 10));
        }
        if(String(distanceFormat) == "km"){
            distance = distance * 0.001;
            result.unit = "km";
        }
        else if(String(distanceFormat) == "nm"){
            distance = distance * 0.000539957;
            result.unit = "nm";
        }
        else{;
            result.unit = "m";
        }
        if(distance < 10){
            snprintf(buffer,bsize,"%3.2f",distance);
        }
        if(distance >= 10 && distance < 100){
            snprintf(buffer,bsize,"%3.1f",distance);
        }
        if(distance >= 100){
            snprintf(buffer,bsize,"%3.0f",distance);
        }
    }
    //########################################################
    // Special XDR formats
    // Refer XDR formats in GwXDRMappings.cpp line 40
    //########################################################
     else if (value->getFormat() == "formatXdr:P:P"){
        double pressure = 0;
        if(usesimudata == false) {
            pressure = value->value;
            pressure = pressure;                // Unit conversion form Pa to mBar
        }
        else{
            pressure = 968 + float(random(0, 10));
        }
        snprintf(buffer,bsize,"%4.0f",pressure);
        result.unit = "mBar";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:P:B"){
        double pressure = 0;
        if(usesimudata == false) {
            pressure = value->value;
            pressure = pressure / 100.0;      // Unit conversion form Pa to mBar
        }
        else{
            pressure = 968 + float(random(0, 10));
        }
        snprintf(buffer,bsize,"%4.0f",pressure);
        result.unit = "mBar";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:U:V"){
        double voltage = 0;
        if(usesimudata == false) {
            voltage = value->value;
        }
        else{
            voltage = 12 + float(random(0, 30)) / 10.0;
        }
        if(voltage < 10){
            snprintf(buffer,bsize,"%3.2f",voltage);
        }
        else{
            snprintf(buffer,bsize,"%3.1f",voltage);
        }
        result.unit = "V";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:I:A"){
        double current = 0;
        if(usesimudata == false) {
            current = value->value;
        }
        else{
            current = 8.2 + float(random(0, 50)) / 10.0;
        }
        if(current < 10){
            snprintf(buffer,bsize,"%3.2f",current);
        }
        if(current >= 10 && current < 100){
            snprintf(buffer,bsize,"%3.1f",current);
        }
        if(current >= 100){
            snprintf(buffer,bsize,"%3.0f",current);
        }
        result.unit = "A";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:C:K"){
        double temperature = 0;
        if(usesimudata == false) {
            temperature = value->value - 273.15;    // Convert K to C
        }
        else{
            temperature = 21.8 + float(random(0, 50)) / 10.0;
        }
        if(temperature < 10){
            snprintf(buffer,bsize,"%3.2f",temperature);
        }
        if(temperature >= 10 && temperature < 100){
            snprintf(buffer,bsize,"%3.1f",temperature);
        }
        if(temperature >= 100){
            snprintf(buffer,bsize,"%3.0f",temperature);
        }
        result.unit = "Deg C";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:C:C"){
        double temperature = 0;
        if(usesimudata == false) {
            temperature = value->value;    // Value in C
        }
        else{
            temperature = 21.8 + float(random(0, 50)) / 10.0;
        }
        if(temperature < 10){
            snprintf(buffer,bsize,"%3.2f",temperature);
        }
        if(temperature >= 10 && temperature < 100){
            snprintf(buffer,bsize,"%3.1f",temperature);
        }
        if(temperature >= 100){
            snprintf(buffer,bsize,"%3.0f",temperature);
        }
        result.unit = "Deg C";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:H:P"){
        double humidity = 0;
        if(usesimudata == false) {
            humidity = value->value;    // Value in %
        }
        else{
            humidity = 41.3 + float(random(0, 50)) / 10.0;
        }
        if(humidity < 10){
            snprintf(buffer,bsize,"%3.2f",humidity);
        }
        if(humidity >= 10 && humidity < 100){
            snprintf(buffer,bsize,"%3.1f",humidity);
        }
        if(humidity >= 100){
            snprintf(buffer,bsize,"%3.0f",humidity);
        }
        result.unit = "%";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:V:P"){
        double volume = 0;
        if(usesimudata == false) {
            volume = value->value;    // Value in %
        }
        else{
            volume = 85.8 + float(random(0, 50)) / 10.0;
        }
        if(volume < 10){
            snprintf(buffer,bsize,"%3.2f",volume);
        }
        if(volume >= 10 && volume < 100){
            snprintf(buffer,bsize,"%3.1f",volume);
        }
        if(volume >= 100){
            snprintf(buffer,bsize,"%3.0f",volume);
        }
        result.unit = "%";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:V:M"){
        double volume = 0;
        if(usesimudata == false) {
            volume = value->value;    // Value in l
        }
        else{
            volume = 75.2 + float(random(0, 50)) / 10.0;
        }
        if(volume < 10){
            snprintf(buffer,bsize,"%3.2f",volume);
        }
        if(volume >= 10 && volume < 100){
            snprintf(buffer,bsize,"%3.1f",volume);
        }
        if(volume >= 100){
            snprintf(buffer,bsize,"%3.0f",volume);
        }
        result.unit = "l";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:R:I"){
        double flow = 0;
        if(usesimudata == false) {
            flow = value->value;    // Value in l/min
        }
        else{
            flow = 7.5 + float(random(0, 20)) / 10.0;
        }
        if(flow < 10){
            snprintf(buffer,bsize,"%3.2f",flow);
        }
        if(flow >= 10 && flow < 100){
            snprintf(buffer,bsize,"%3.1f",flow);
        }
        if(flow >= 100){
            snprintf(buffer,bsize,"%3.0f",flow);
        }
        result.unit = "l/min";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:G:"){
        double generic = 0;
        if(usesimudata == false) {
            generic = value->value;    // Value in l/min
        }
        else{
            generic = 18.5 + float(random(0, 20)) / 10.0;
        }
        if(generic < 10){
            snprintf(buffer,bsize,"%3.2f",generic);
        }
        if(generic >= 10 && generic < 100){
            snprintf(buffer,bsize,"%3.1f",generic);
        }
        if(generic >= 100){
            snprintf(buffer,bsize,"%3.0f",generic);
        }
        result.unit = "";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:A:P"){
        double dplace = 0;
        if(usesimudata == false) {
            dplace = value->value;    // Value in %
        }
        else{
            dplace = 55.3 + float(random(0, 20)) / 10.0;
        }
        if(dplace < 10){
            snprintf(buffer,bsize,"%3.2f",dplace);
        }
        if(dplace >= 10 && dplace < 100){
            snprintf(buffer,bsize,"%3.1f",dplace);
        }
        if(dplace >= 100){
            snprintf(buffer,bsize,"%3.0f",dplace);
        }
        result.unit = "%";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:A:D"){
        double angle = 0;
        if(usesimudata == false) {
            angle = value->value;
            angle = angle * 57.2958;      // Unit conversion form rad to deg
        }
        else{
            angle = 20 + random(-5, 5);
        }
        if(angle > -10 && angle < 10){
            snprintf(buffer,bsize,"%3.1f",angle);
        }
        else{
            snprintf(buffer,bsize,"%3.0f",angle);
        }
        result.unit = "Deg";
    }
    //########################################################
    else if (value->getFormat() == "formatXdr:T:R"){
        double rpm = 0;
        if(usesimudata == false) {
            rpm = value->value;    // Value in rpm
        }
        else{
            rpm = 2505 + random(0, 20);
        }
        if(rpm < 10){
            snprintf(buffer,bsize,"%3.2f",rpm);
        }
        if(rpm >= 10 && rpm < 100){
            snprintf(buffer,bsize,"%3.1f",rpm);
        }
        if(rpm >= 100){
            snprintf(buffer,bsize,"%3.0f",rpm);
        }
        result.unit = "rpm";
    }
    //########################################################
    // Default format
    //########################################################
    else{
        if(value->value < 10){
            snprintf(buffer,bsize,"%3.2f",value->value);
        }
        if(value->value >= 10 && value->value < 100){
            snprintf(buffer,bsize,"%3.1f",value->value);
        }
        if(value->value >= 100){
            snprintf(buffer,bsize,"%3.0f",value->value);
        }
        result.unit = "";
    }
    buffer[bsize]=0;
    result.svalue = String(buffer);
    return result;
}

#endif
