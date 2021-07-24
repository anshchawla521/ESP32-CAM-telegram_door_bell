String sendPhotoTelegram(String CHAT_ID) {
  
  if ((WiFi.status() == WL_CONNECTED)) {

    const char* myDomain = "api.telegram.org";
    String getAll = "";
    String getBody = "";
    
    camera_fb_t * fb = NULL;
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(10);
    fb = esp_camera_fb_get();
    delay(10);
    digitalWrite(FLASH_LED_PIN, LOW);

    if (!fb) {
      Serial.println("Camera capture failed");
      delay(1000);
      ESP.restart();
      return "Camera capture failed";
    }

    Serial.println("Connecting to " + String(myDomain));


    if (clientTCP.connect(myDomain, 443))
    {
      Serial.println("Connected to " + String(myDomain));

      String head = "--India\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--India\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
      String tail = "\r\n--India--\r\n";

      uint16_t imageLen = fb->len;
      uint16_t extraLen = head.length() + tail.length();
      uint16_t totalLen = imageLen + extraLen;

      clientTCP.println("POST /bot" + BOTtoken + "/sendPhoto HTTP/1.1");
      clientTCP.println("Host: " + String(myDomain));
      clientTCP.println("Content-Length: " + String(totalLen));
      clientTCP.println("Content-Type: multipart/form-data; boundary=India");
      clientTCP.println();
      clientTCP.print(head);

      uint8_t *fbBuf = fb->buf;
      size_t fbLen = fb->len;


      for (size_t n = 0; n < fbLen; n = n + 1024)
      {

        if (n + 1024 < fbLen)
        {
          clientTCP.write(fbBuf, 1024);
          fbBuf += 1024;
        }
        else if (fbLen % 1024 > 0)
        {
          size_t remainder = fbLen % 1024;
          clientTCP.write(fbBuf, remainder);
        }
      }

      clientTCP.print(tail);

      esp_camera_fb_return(fb);

      int waitTime = 10000;   // timeout 10 seconds
      unsigned long startTime = millis();
      boolean state = false;

      while ((startTime + waitTime) > millis())
      {
        if (startTime > millis()) // means millis() overflowed
        {
          startTime = millis();
        }
        else {
          Serial.print(".");
          delay(100);
          while (clientTCP.available())
          {
            char c = clientTCP.read();
            if (c == '\n')
            {
              if (getAll.length() == 0) state = true;
              getAll = "";
            }
            else if (c != '\r')
              getAll += String(c);
            if (state == true) getBody += String(c);
            startTime = millis();
          }
          if (getBody.length() > 0) break;
        }
      }
      clientTCP.stop();
      Serial.println(getBody);
    }
    else {
      getBody = "Connected to api.telegram.org failed.";
      Serial.println("Connected to api.telegram.org failed.");
    }
    return getBody;
  }else{
    blinkLED(1);
    WiFi.disconnect();
    WiFi.begin(ssid , password);    
    
  }
}

void handleNewMessages(int numNewMessages)
{
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages && i < 10 ; i++) // handel maximum of 10 msgs at a time
  {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    Serial.print("text = ");
    Serial.println(text);
    if (text.indexOf(" ") != -1 )
    {
      String pswd = text.substring(text.indexOf(" ") + 1);
      text =  text.substring(0, text.indexOf(" "));
      if ( text == "/secretconfig" && pswd == "doorbell"  )
      { bot.sendMessage(chat_id, "entering config mode", "Markdown");
        configmode();

      }
    }

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/flash")
    {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
    } else if (text == "/photo" )
    {
      if (chat_id == CHAT_ID || chat_id == CHAT_ID1) {
        sendPhoto = true;
      }
      if (chat_id != CHAT_ID && chat_id != CHAT_ID1) {
        sendPhoto = false ;
        bot.sendMessage(chat_id, "unautorised", "Markdown");
        bot.sendMessage(CHAT_ID, "unauthorised access by " + chat_id + " " + from_name , "Markdown");
      }
    }  else if (text == "/start")
    {
      String welcome = "Welcome " + from_name + " to ESP32Cam Telegram bot.\n\n";
      welcome += "/photo : will take a photo\n";
      welcome += "/flash : toggle flash LED (VERY BRIGHT!)\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}


void configmode()
{ int enter_time = millis();
  while ( millis() - enter_time < 60000)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    { enter_time = millis();
      String subword = "";
      Serial.println("got response");
      for (int i = 0; i < numNewMessages && i < 10 ; i++) // handel maximum of 10 msgs at a time
      {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;
        Serial.print("text = ");
        Serial.println(text);
        if (text.indexOf(" ") != -1 )
        {
          subword = text.substring(text.indexOf(" ") + 1);
          text =  text.substring(0, text.indexOf(" "));
        }
        if (text == "/exit")
        {
          bot.sendMessage(chat_id, "exited", "Markdown");
          return;
        } else if (text == "/addchatid" && subword != text && subword != "")
        {
          CHAT_ID1 = subword;
          bot.sendMessage(chat_id, "added " + CHAT_ID1, "Markdown");
        }
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
    }
  }
}
