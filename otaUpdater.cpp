

/*
 * OTAWebUpdater.ino Example from ArduinoOTA Library
 * Rui Santos 
 * Complete Project Details https://randomnerdtutorials.com
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <Update.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>



const char* loginIndex = 
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<td>Username:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/updateIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";
 

const char* updateIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<style>  body { background-color: #5b6063; padding:10px; color: #97A7B3 }</style>"
"<p> It puts the .bin in the Flash or else it gets the boot again!</p>"
"<form method='POST' action='/' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"

"<br><a href=\"/\"><br>Return to Home Page</a>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!');" 
  "setTimeout(function(){window.location='/';}, 1000);"
  "document.write('SUCCESS! Rebooting.... <br>You will be redirected shotly.');"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";



const char* uploadSuccess = 
    "<p>SUCCESS! <br>Updated Firmware. Rebooting...</p>"
    "<br><a href=\"/\"><br>Return to Home Page</a>";

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    // handle upload and update
    if (!index)
    {
        Serial.printf("Update: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        { //start with max available size
            Update.printError(Serial);
        }
    }

    /* flashing firmware to ESP*/
    if (len)
    {
        Update.write(data, len);
    }

    if (final)
    {
        if (Update.end(true))
        { //true to set the size to the current progress
            Serial.printf("Update Success: %ub written\nRebooting...\n", index+len);

            request->send(200, "text/html", "SUCCESS! <br>Updated Firmware. Rebooting..."
                                             "<br><a href=\"/\"><br>Return to Home Page</a>");
        }
        else
        {
            Update.printError(Serial);
        }
    }
}


void serverOtaRoutes(AsyncWebServer *server){
  /*return index page which is stored in updateIndex */
  server->on("/updateLogin", HTTP_GET, [](AsyncWebServerRequest *request) {
    //request->sendHeader("Connection", "close");
    request->send(200, "text/html", loginIndex);
  });

  server->on("/updateIndex", HTTP_GET, [](AsyncWebServerRequest *request) {
    //server->sendHeader("Connection", "close");
    request->send(200, "text/html", updateIndex);
  });

  /*handling uploading firmware file */
  server->on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    //server->sendHeader("Connection", "close");
    //request->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    request->send(200, "text/html", uploadSuccess);

    ESP.restart();
  }, handleUpload);
//
//  {
//
//    HTTPUpload& upload = server->upload();
//
//    if (upload.status == UPLOAD_FILE_START) {
//      Serial.printf("Update: %s\n", upload.filename.c_str());
//      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
//        Update.printError(Serial);
//      }
//    } else if (upload.status == UPLOAD_FILE_WRITE) {
//      /* flashing firmware to ESP*/
//      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
//        Update.printError(Serial);
//      }
//    } else if (upload.status == UPLOAD_FILE_END) {
//      if (Update.end(true)) { //true to set the size to the current progress
//        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
//      } else {
//        Update.printError(Serial);
//      }
//    }
//  });

}

