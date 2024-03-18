// internet_radio.ino
/* 
 internet radio; combined code from
 Simple_esp32_radio - https://www.hackster.io/mircemk/simple-esp32-internet-radio-with-oled-display-83e49d
 with 
 ESP-radio-github - https://github.com/Edzelf/Esp-radio 

 ESP32 libraries used:
  - ESP32 for Arduino - https://dl.espressif.com/dl/package_esp32_index.json (include in Menu | Preferences | Board managers)
  - Preferences       - https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h
  - helloMp3.h        - included with code
  - radiostations.h   - included with code
  - WiFi.h            - WIFININA - to include via Library manager
  - HTTPClient.h      - https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.h
  - Adafruit_GFX      - https://github.com/adafruit/Adafruit-GFX-Library
  - Adafruit_ST7735.h - https://github.com/adafruit/Adafruit-ST7735-Library
  - SPI.h             - Part of ESP8266 Arduino default libraries.
  - Ticker.h          - https://github.com/sstaub/Ticker
  - esp_wifi.h        - https://github.com/madhephaestus/Esp32WifiManager
  - Arduino.h         - Part of ESP8266 Arduino default libraries.
  - IRremoteESP8266.h - https://github.com/esp8266/Basic/tree/master/libraries/IRremoteESP8266
  - IRrecv.h          - https://github.com/esp8266/Basic/tree/master/libraries/IRremoteESP8266
  - IRutils.h         - https://github.com/esp8266/Basic/tree/master/libraries/IRremoteESP8266
  - VS1053.h          - https://github.com/baldram/ESP_VS1053_Library/tree/master/src

 hardware
  - ESP32             - https://www.aliexpress.com/item/32799253567.html?spm=a2g0s.9042311.0.0.27424c4dLpU1BO
  - Rotary decoder    - https://www.aliexpress.com/item/32969404416.html?spm=a2g0s.9042311.0.0.27424c4dLpU1BO
  - IR set            - https://www.aliexpress.com/item/32859719981.html?spm=a2g0s.9042311.0.0.27424c4dLpU1BO
  - TFT display       - https://www.aliexpress.com/item/32817839166.html?spm=a2g0s.9042311.0.0.27424c4dLpU1BO
  - VS1053            - https://www.aliexpress.com/item/32966454016.html?spm=a2g0s.9042311.0.0.27424c4dLpU1BO

If possible streaming info of artist and song title is shown. For some reason i've not been able to show this streaming information
for all streams.....
Please let me know if you are able to show more streaming info and I'd be honered if you will share the updated code with me!


*/

#include <Preferences.h>    //For reading and writing into the FLASH memory
#include "helloMp3.h"       // to say 'Hello' to listener
#include "radiostations.h"  // all streaming channels used in this player

// display
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <Ticker.h>
// ESP32
#include <esp_wifi.h>
#include <WiFi.h>
#include <HTTPClient.h>
// IR
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
// This ESP_VS1053_Library
#include <VS1053.h>

Preferences preferences; // instance to acces NVRAM

bool DEBUG = false;  // if "true" all info will be shown in serial monitor; id "false" no info is shown on serial monitor

// Rotary Encoder start
const int PinSW=5;   // Rotary Encoder Switch
const int PinDT=4;    // DATA signal
const int PinCLK=2;    // CLOCK signal
// Variables to debounce Rotary Encoder start
long TimeOfLastDebounce = 0;
int DelayofDebounce = 0.01;
unsigned long previousMillisrotating = 0;
unsigned long previousMillispressed = 0;
unsigned long interval = 2000;
// Store previous Pins state
int PreviousCLK;   
int PreviousDATA;
int rotary_step_counter = 0;
bool rotating = false;
bool pressed = false;
// Rotary Encoder end

// IR init start
unsigned long key_value = 0;
const uint16_t kRecvPin = 27; // An IR detector/demodulator is connected to GPIO pin 27
IRrecv irrecv(kRecvPin);
decode_results results;
unsigned long previousMillisIR = 0;
unsigned long IR_interval = 2000; // after 2 seconds no input : stop waiting and activate info received
bool IR_volume = false;
bool IR_station = false;
bool digitinput = false;
unsigned int IR_value, Nbr_chars;
//IR init end

// Display init start
// pin definition for ESP32 for display
// VCC - +5Volt
// Gnd - Gnd
#define TFT_CS   17
#define TFT_Rst  16
#define TFT_DC   15 // A0
// SDA - 23
// SCK - 18
// LED - backlight +5 Volt via Resistor
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_Rst);
// Display init end

// WiFi init start
//WiFi network info
char ssid[] = "hcg";    //  your network SSID (name) 
char pass[] = "!!snapper123hans";   // your network password
String LocalIP="";
int status = WL_IDLE_STATUS;
WiFiClient  client;
// WiFi init end

/* VS1053 player init start
VS1053 - connections detail
 XRST = EN (D3)
 MISO = D19
 MOSI = D23
 SCLK = D18
 VCC = 5V / 3.3 V
 Gnd = Gnd 
*/
// Wiring of VS1053 board (SPI connected in a standard way)
#define VS1053_CS    32 //32
#define VS1053_DCS   33  //33
#define VS1053_DREQ  35 //15
uint8_t mp3buff[32];   // vs1053 likes 32 bytes at a time
VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);
int my_volume = 75; // volume level 0-100
// VS1053 init end

// data definitions for getting radio data and playing it start
#define RINGBFSIZ 20000                                     // Ringbuffer for smooth playing. 20000 bytes is 160 Kbits, about 1.5 seconds at 128kb bitrate.
uint8_t*         ringbuf ;                                  // Ringbuffer for VS1053
uint16_t         rbwindex = 0 ;                             // Fill pointer in ringbuffer
uint16_t         rbrindex = RINGBFSIZ - 1 ;                 // Emptypointer in ringbuffer
uint16_t         rcount = 0 ;                               // Number of bytes in ringbuffer

String           icystreamtitle ;                           // Streamtitle from metadata
String           icyname ;                                  // Icecast station name

enum datamode_t { INIT = 1, HEADER = 2, DATA = 4,
                  METADATA = 8, NoMETA = 16,
                  PLAYLISTHEADER = 32, PLAYLISTDATA = 64,
                  STOPREQD = 128, STOPPED = 256} ;          // State for datastream
datamode_t       datamode ;                                 // State of datastream                
bool             chunked = false ;                          // Station provides chunked transfer
int              chunkcount = 0 ;                           // Counter for chunked transfer
int              metaint = 0 ;                              // Number of databytes between metadata
int              bitrate ;                                  // Bitrate in kb/sec
uint32_t         totalcount = 0 ;                           // Counter mp3 data
String           metaline ;                                 // Readable line in metadata
int              datacount ;                                // Counter databytes before metadata
int              metacount ;                                // Number of bytes in metadata
int8_t           playlist_num = 0 ;                         // Nonzero for selection from playlist
char             newstreamtitle[150] = "";                  // storeStreamtitle from metadata
Ticker           tckr ;                                    // For timing 100 msec

int radioStation,old_radioStation,new_radioStation;
// data definitions for getting radio data and playing it end

// start music code
// Ringbuffer code start
//******************************************************************************************
// Ringbuffer (fifo) routines.                                                             *
//******************************************************************************************
//******************************************************************************************
//                              R I N G S P A C E                                          *
//******************************************************************************************
inline bool ringspace()
{
  return ( rcount < RINGBFSIZ ) ;     // True is at least one byte of free space is available
}

//******************************************************************************************
//                              R I N G A V A I L                                          *
//******************************************************************************************
inline uint16_t ringavail()
{
  return rcount ;                     // Return number of bytes available
}

//******************************************************************************************
//                                P U T R I N G                                            *
//******************************************************************************************
void putring ( uint8_t b )                 // Put one byte in the ringbuffer
{
  // No check on available space.  See ringspace()
  *(ringbuf + rbwindex) = b ;         // Put byte in ringbuffer
  if ( ++rbwindex == RINGBFSIZ )      // Increment pointer and
  {
    rbwindex = 0 ;                    // wrap at end
  }
  rcount++ ;                          // Count number of bytes in the ringbuffer
}

//******************************************************************************************
//                                G E T R I N G                                            *
//******************************************************************************************
uint8_t getring()
{
  // Assume there is always something in the bufferpace.  See ringavail()
  if ( ++rbrindex == RINGBFSIZ )      // Increment pointer and
  {
    rbrindex = 0 ;                    // wrap at end
  }
  rcount-- ;                          // Count is now one less
  return *(ringbuf + rbrindex) ;      // return the oldest byte
}

//******************************************************************************************
//                               E M P T Y R I N G                                         *
//******************************************************************************************
void emptyring()
{
  rbwindex = 0 ;                      // Reset ringbuffer administration
  rbrindex = RINGBFSIZ - 1 ;
  rcount = 0 ;
}
// Ringbuffer code end

// data evaluation to find streaming info and play music start
//******************************************************************************************
//                            C H K H D R L I N E                                          *
//******************************************************************************************
// Check if a line in the header is a reasonable headerline.                               *
// Normally it should contain something like "icy-xxxx:abcdef".                            *
//******************************************************************************************
bool chkhdrline ( const char* str )
{
  char    b ;                                         // Byte examined
  int     len = 0 ;                                   // Lengte van de string

  while ( ( b = *str++ ) )                            // Search to end of string
  {
    len++ ;                                           // Update string length
    if ( ! isalpha ( b ) )                            // Alpha (a-z, A-Z)
    {
      if ( b != '-' )                                 // Minus sign is allowed
      {
        if ( b == ':' )                               // Found a colon?
        {
          return ( ( len > 5 ) && ( len < 50 ) ) ;    // Yes, okay if length is okay
        }
        else
        {
          return false ;                              // Not a legal character
        }
      }
    }
  }
  return false ;                                      // End of string without colon
}

//******************************************************************************************
//                           H A N D L E B Y T E _ C H                                     *
//******************************************************************************************
// Handle the next byte of data from server.                                               *
// Chunked transfer encoding aware. Chunk extensions are not supported.                    *
//******************************************************************************************
void handlebyte_ch ( uint8_t b, bool force )
{
  static int  chunksize = 0 ;                         // Chunkcount read from stream

  if ( chunked && !force && 
       ( datamode & ( DATA |                          // Test op DATA handling
                      METADATA |
                      PLAYLISTDATA ) ) )
  {
    if ( chunkcount == 0 )                            // Expecting a new chunkcount?
    {
       if ( b == '\r' )                               // Skip CR
       {
         return ;      
       }
       else if ( b == '\n' )                          // LF ?
       {
         chunkcount = chunksize ;                     // Yes, set new count
         chunksize = 0 ;                              // For next decode
         return ;
       }
       // We have received a hexadecimal character.  Decode it and add to the result.
       b = toupper ( b ) - '0' ;                      // Be sure we have uppercase
       if ( b > 9 )
       {
         b = b - 7 ;                                  // Translate A..F to 10..15
       }
       chunksize = ( chunksize << 4 ) + b ;
    }
    else
    {
      handlebyte ( b, force ) ;                       // Normal data byte
      chunkcount-- ;                                  // Update count to next chunksize block
    }
  }
  else
  {
    handlebyte ( b, force ) ;                         // Normal handling of this byte
  }
}

//******************************************************************************************
//                           H A N D L E B Y T E                                           *
//******************************************************************************************
// Handle the next byte of data from server.                                               *
// This byte will be send to the VS1053 most of the time.                                  *
// Note that the buffer the data chunk must start at an address that is a muttiple of 4.   *
// Set force to true if chunkbuffer must be flushed.                                       *
//******************************************************************************************
void handlebyte ( uint8_t b, bool force )
{
  static uint16_t  playlistcnt ;                       // Counter to find right entry in playlist
  static bool      firstmetabyte ;                     // True if first metabyte (counter)
  static int       LFcount ;                           // Detection of end of header
  static __attribute__((aligned(4))) uint8_t buf[32] ; // Buffer for chunk
  static int       bufcnt = 0 ;                        // Data in chunk
  static bool      firstchunk = true ;                 // First chunk as input
  String           lcml ;                              // Lower case metaline
  String           ct ;                                // Contents type
  static bool      ctseen = false ;                    // First line of header seen or not
  int              inx ;                               // Pointer in metaline
  int              i ;                                 // Loop control

  if ( datamode == INIT )                              // Initialize for header receive
  {
    ctseen = false ;                                   // Contents type not seen yet
    metaint = 0 ;                                      // No metaint found
    LFcount = 0 ;                                      // For detection end of header
    bitrate = 0 ;                                      // Bitrate still unknown
    if (DEBUG == true) Serial.println ( "Switch to HEADER" ) ;
    datamode = HEADER ;                                // Handle header
    totalcount = 0 ;                                   // Reset totalcount
    metaline = "" ;                                    // No metadata yet
    firstchunk = true ;                                // First chunk expected
  }
  if ( datamode == DATA )                              // Handle next byte of MP3/Ogg data
  {
    buf[bufcnt++] = b ;                                // Save byte in chunkbuffer
    if ( bufcnt == sizeof(buf) || force )              // Buffer full?
    {
      if ( firstchunk )
      {
        firstchunk = false ;
        if (DEBUG == true) {
          Serial.println ( "First chunk:" ) ;            // Header for printout of first chunk
          for ( i = 0 ; i < 32 ; i += 8 )                // Print 4 lines
          { Serial.printf ( "%02X %02X %02X %02X %02X %02X %02X %02X",
                            buf[i], buf[i + 1], buf[i + 2], buf[i + 3],
                            buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7] ) ;
            Serial.println ("");
          }  
        }
      }
      player.playChunk ( buf, bufcnt ) ;               // Yes, send to player
      bufcnt = 0 ;                                     // Reset count
    }
    totalcount++ ;                                     // Count number of bytes, ignore overflow
    if ( metaint != 0 )                                // No METADATA on Ogg streams or mp3 files
    {
      if ( --datacount == 0 )                          // End of datablock?
      {
        if ( bufcnt )                                  // Yes, still data in buffer?
        {
          player.playChunk ( buf, bufcnt ) ;           // Yes, send to player
          bufcnt = 0 ;                                 // Reset count
        }
        if (DEBUG == true)  Serial.println ( "Switch to METADATA" ) ;
        datamode = METADATA ;
        firstmetabyte = true ;                         // Expecting first metabyte (counter)
      }
    } else {     
          if (DEBUG == true)  Serial.println("Switching to NoMeta"); 
          datamode = NoMETA;
          station_connect(radioStation); 
    }
    return ;
  }
  if ( datamode == HEADER )                            // Handle next byte of MP3 header
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds

      if ( chkhdrline ( metaline.c_str() ) )           // Reasonable input?
      {
        lcml = metaline ;                              // Use lower case for compare
        lcml.toLowerCase() ;
        if (DEBUG == true) {
          Serial.println("metaline :");
          Serial.printf ( metaline.c_str() ) ;           // Yes, Show it
          Serial.println("");
        }  
        if ( lcml.indexOf ( "content-type" ) >= 0 )    // Line with "Content-Type: xxxx/yyy"
        {
          ctseen = true ;                              // Yes, remember seeing this
          ct = metaline.substring ( 14 ) ;             // Set contentstype. Not used yet
          if (DEBUG == true) {
            Serial.printf ( "%s seen.", ct.c_str() ) ;
            Serial.println("");
          }  
        }
        if ( lcml.startsWith ( "icy-br:" ) )
        {
          bitrate = metaline.substring(7).toInt() ;    // Found bitrate tag, read the bitrate
          if ( bitrate == 0 )                          // For Ogg br is like "Quality 2"
          {
            bitrate = 87 ;                             // Dummy bitrate
          }
        }
        else if ( lcml.startsWith ( "icy-metaint:" ) )
        {
          metaint = metaline.substring(12).toInt() ;   // Found metaint tag, read the value
        }
        else if ( lcml.startsWith ( "icy-name:" ) )
        {
          icyname = metaline.substring(9) ;            // Get station name
          icyname.trim() ;                             // Remove leading and trailing spaces
          if (DEBUG == true) {
            Serial.println(icyname);       
            Serial.println("");
          }  
        }
        else if ( lcml.startsWith ( "transfer-encoding:" ) )
        {
          // Station provides chunked transfer
          if ( lcml.endsWith ( "chunked" ) )
          {
            chunked = true ;                           // Remember chunked transfer mode
            chunkcount = 0 ;                           // Expect chunkcount in DATA
          }
        }
      }
      metaline = "" ;                                  // Reset this line
      if ( ( LFcount == 2 ) && ctseen )                // Some data seen and a double LF? ( ( LFcount == 2 ) && ctseen )
      {
        if (DEBUG == true) {
          Serial.printf ( "Switch to DATA, bitrate is %d"     // Show bitrate
                           ", metaint is %d",                  // and metaint
                            bitrate, metaint ) ;
          Serial.println("");
        }  
        datamode = DATA ;                              // Expecting data now
        datacount = metaint ;                          // Number of bytes before first metadata
        bufcnt = 0 ;                                   // Reset buffer count
        player.startSong() ;                           // Start a new song
       }
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
    return ;
  }
  if ( datamode == METADATA )                          // Handle next byte of metadata
  {
    if ( firstmetabyte )                               // First byte of metadata?
    {
      firstmetabyte = false ;                          // Not the first anymore
      metacount = b * 16 + 1 ;                         // New count for metadata including length byte
      if ( metacount > 1 )
      {
        if (DEBUG == true)  Serial.printf ( "Metadata block %d bytes",
                   metacount - 1 ) ;                   // Most of the time there are zero bytes of metadata
      }
      metaline = "" ;                                  // Set to empty
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
    }
    if ( --metacount == 0 )
    {
      if ( metaline.length() )                         // Any info present?
      {
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if present.
        showstreamtitle ( metaline.c_str(),false ) ;         // Show artist and title if present in metadata
      }
      if ( metaline.length() > 1500 )                  // Unlikely metaline length?
      {
        if (DEBUG == true)  Serial.printf ( "Metadata block too long! Skipping all Metadata from now on." ) ;
        metaint = 0 ;                                  // Probably no metadata
        metaline = "" ;                                // Do not waste memory on this
      }
      datacount = metaint ;                            // Reset data count
      bufcnt = 0 ;                                     // Reset buffer count
      if (DEBUG == true) Serial.println ( "Switch to DATA" ) ; 
      datamode = DATA ;                                // Expecting data
    }
  }
}
// end song tilte code

//******************************************************************************************
//                                  T I M E R 1 0 S E C                                    *
//******************************************************************************************
// Extra watchdog.  Called every 10 seconds.                                               *
// If totalcount has not been changed, there is a problem and playing will stop.           *
// Note that a "yield()" within this routine or in called functions will cause a crash!    *
//******************************************************************************************
void timer10sec()
{
  static uint32_t oldtotalcount = 7321 ;          // Needed foor change detection
  static uint8_t  morethanonce = 0 ;              // Counter for succesive fails
  static uint8_t  t600 = 0 ;                      // Counter for 10 minutes
  if (DEBUG == true) Serial.println("timer10sec");

  if ( datamode & ( INIT | HEADER | DATA |        // Test op playing
                    METADATA | NoMETA ) )
  {
    if ( totalcount == oldtotalcount )            // Still playing?
    {
      if (DEBUG == true) Serial.println ( "No data input" ) ;              // No data detected!
      if ( morethanonce > 10 )                    // Happened too many times?
      {
        if (DEBUG == true) Serial.println ( "Going to restart connection" ) ;
        datamode = INIT ; 
        station_connect(radioStation); 
        if (DEBUG == true) Serial.println ( "Switch to INIT" ) ;             

      }
      morethanonce++ ;                            // Count the fails
    }
    else
    {
      if ( morethanonce )                         // Recovered from data loss?
      {
        if (DEBUG == true) Serial.println ( "Recovered from dataloss" ) ;
        morethanonce = 0 ;                        // Data see, reset failcounter
      }
      oldtotalcount = totalcount ;                // Save for comparison in next cycle
    }
    if ( t600++ == 60 )                           // 10 minutes over?
    {
      t600 = 0 ;                                  // Yes, reset counter
      if (DEBUG == true) Serial.println ( "10 minutes over" ) ;
//      publishIP() ;                               // Re-publish IP
    }
  }
}
// data evaluation to find streaming info and play music end

void setup () {
// Start display
   tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
   tft.fillScreen(ST7735_BLACK);  
   tft.setRotation(1);
   display_Title();   
// rotary decoder
   pinMode (PinCLK, INPUT_PULLUP);                  // Set pinA (Clk) as input
   pinMode (PinDT, INPUT_PULLUP);                   // Set pinB as input
   // Put current pins state in variables
   PreviousCLK=digitalRead(PinCLK);
   PreviousDATA=digitalRead(PinDT);
   // Set the Switch pin to use Arduino PULLUP resistors
   pinMode(PinSW, INPUT_PULLUP);

// IR Start
   irrecv.enableIRIn();  // Start the receiver
// IR end

// play part buffer and timer initialisation
   ringbuf = (uint8_t *) malloc ( RINGBFSIZ ) ;         // Create ring buffer
   tckr.attach ( 10.0, timer10sec ) ;                   // Every 10 sec

// start serial port
   Serial.begin(115200);

// get data from ESP NVRAM
   preferences.begin("my-app", false);
   radioStation = preferences.getUInt("radioStation", 0);
   my_volume    = preferences.getUInt("Volume",70); 

// initialize SPI bus;
   SPI.begin();

// initialize VS1053 player
    player.begin();
//    player.switchToMp3Mode(); // optional, some boards require this
    player.setVolume(my_volume);

// connect tot FiFi
   WiFi.begin(ssid, pass);
   while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (DEBUG == true)  Serial.print(".");
   }
   if (DEBUG == true) {
     Serial.println("WiFi connected");  
     Serial.print("IP address:");  
   }  
   LocalIP=WiFi.localIP().toString();
   if (DEBUG == true) Serial.println(LocalIP);
   display_Connectto();
   displayIP();
   display_Volume(my_volume);
   
// store staion for later reference
   old_radioStation=radioStation;
   if (DEBUG == true)  Serial.printf("Current radioStation value: %u\n", radioStation);
   delay(100);
// play 'HELLO' text   
   player.playChunk(hello2, sizeof(hello2)); //VS1053 is wake up & running
// connect to stored station   
   if (DEBUG == true) Serial.println ( "Switch to INIT" ) ;
   datamode = INIT ; 
   station_connect(radioStation); 
   display_Station(radioStation);
}

// update radiostation after new input from IR or rotary decoder
void change_radiostation(){ 
   new_radioStation=radioStation;
   if(old_radioStation != new_radioStation) {
     if (DEBUG == true) {
       Serial.printf("Set radioStation to new_value: %u\n", radioStation);
       Serial.println ( "Switch to INIT" ) ;             
     }  
     player.softReset(); 
     datamode = INIT ; 
   }  
   station_connect(new_radioStation); 
   preferences.putUInt("radioStation",new_radioStation); // save station in nvram
   display_Station(new_radioStation);
   old_radioStation = new_radioStation;
}

// update volume after new input from IR or rotary decoder
void change_volume(){ 
   player.setVolume(my_volume);
   if (DEBUG == true)  Serial.printf("Volume: %u\n", my_volume);
   preferences.putUInt("Volume",my_volume); 
   display_Volume(my_volume);
   player.startSong();
}

// rotary extra code start
// update display information on volume or station after IR or rotary switch change
void update_vol_or_station(){ 
  if (rotating != true){
    rotating = true;
    if (DEBUG == true) Serial.println("rotating = true");
    if (pressed != true) rotary_step_counter = radioStation;  
  }
  previousMillisrotating = millis();
  if (pressed == true) { 
    previousMillispressed = millis();
    if (rotary_step_counter >= 101) rotary_step_counter = 100; 
    if (rotary_step_counter <= 0) rotary_step_counter = 0; 
    if (DEBUG == true) {
      Serial.print("Volume = ");
      Serial.println(rotary_step_counter);
    }  
    display_Volume(rotary_step_counter);
  } else {
    if(rotary_step_counter==nbrStations) rotary_step_counter=0;
    if (rotary_step_counter <= -1) rotary_step_counter=nbrStations-1;
    if (DEBUG == true) {
      Serial.print("Station = ");
      Serial.println(rotary_step_counter);
    }  
    display_Station(rotary_step_counter);
  }
}

// Check if Rotary Encoder was moved
void check_rotary() {
 if ((PreviousCLK == 0) && (PreviousDATA == 1)) {
    if ((digitalRead(PinCLK) == 1) && (digitalRead(PinDT) == 0)) {
      rotary_step_counter++;
      update_vol_or_station();
    }
    if ((digitalRead(PinCLK) == 1) && (digitalRead(PinDT) == 1)) {
      rotary_step_counter--;
      update_vol_or_station();
    }
  }
if ((PreviousCLK == 1) && (PreviousDATA == 1)) {
    if ((digitalRead(PinCLK) == 0) && (digitalRead(PinDT) == 1)) {
      rotary_step_counter++;
      update_vol_or_station();
    }
    if ((digitalRead(PinCLK) == 0) && (digitalRead(PinDT) == 0)) {
      rotary_step_counter--;
      update_vol_or_station();
    }
  }  
}
// rotary extra code end

// IR code start
// show debug info on serial monitor
void check_for_input(){
    if (IR_value >= 0) {
      if (IR_volume == true){
         if (DEBUG == true) {
           Serial.print("Volume update naar : ");       
           Serial.println(IR_value);
         }  
      }
      if (IR_station == true){
         if (DEBUG == true) {
           Serial.print("Station update naar : ");       
           Serial.println(IR_value);
         }  
       }
    }
}

// if IR receives digit ('0' .. '9') without leading '#' or '*' act on it as input for volume
void setdigitinput(){
    previousMillisIR  = millis();
    if (DEBUG == true) Serial.println("IR actief zonder keuze -> volume");
    IR_volume = true;
    digitinput = true;
    IR_value = 0; 
    Nbr_chars = 0;  
    display_Volume(my_volume);
}

// Set volume to new value in player
void Update_volume(){
    if (DEBUG == true) {
      Serial.println("Update_volume()");    
      Serial.print("Volume update naar : ");       
      Serial.println(my_volume);   
    }  
    change_volume(); 
}

// Set station to new value in player
void Update_Station(){
    if (DEBUG == true) {
      Serial.println("Update_Station()");    
      Serial.print("Station update naar : ");       
      Serial.println(radioStation);
    }  
    change_radiostation();
}
// IR code end

void process_IR_Digit(int digit) {
   if (DEBUG == true) {
       Serial.print("digit 1 : ");
       Serial.println(digit);
       Serial.print("Nbr_chars 1 : ");
       Serial.println(Nbr_chars);
   }
   if (Nbr_chars == 0) { 
     Serial.println("clear IR_Value and my_volume; volume was set to 100 and new input was detected; so we start over again ");
     IR_value = 0;
     my_volume = 0;
   }
   if (DEBUG == true) {
       Serial.print("digit 2 : ");
       Serial.println(digit);
       Serial.print("Nbr_chars 2 : ");
       Serial.println(Nbr_chars);
}
   if ((IR_volume == false) && (IR_station == false)) {
     if (DEBUG == true) {
       Serial.println(" - setdigitinput");
       Serial.println(digit);
     }  
     setdigitinput();
   }
   if (Nbr_chars = 0) {
     IR_value = digit;
     Nbr_chars++;
   } else {
     if (Nbr_chars = 1) {
       IR_value = 10 * IR_value + digit;
       Nbr_chars++;
     }
   }  
}

void loop() {
   // IR code start
   unsigned long currentMillis = millis();
   if ( (IR_volume == true) || (IR_station == true)) {
     if (currentMillis - previousMillisIR >= IR_interval) {
      if (DEBUG == true) Serial.println("currentMillis - previousMillisIR >= IR_interval"); 
       if (digitinput == true){
         check_for_input(); 
         if (IR_volume == true) {
          my_volume = IR_value;  
         } else {
          radioStation = IR_value;
         }
       }  
       IR_value = 0; 
       Nbr_chars = 0;  
       digitinput = false; 
       if (DEBUG == true) {
         Serial.println("Took to long after last key input; use input end start waiting for new input on IR"); 
         Serial.print("Volume 0 : ");
         Serial.println(my_volume);
         Serial.print("Radiostation 0 : ");
         Serial.println(radioStation);    
       }  
       if (IR_volume == true) { Update_volume();  }
       if (IR_station == true) { Update_Station(); }
       IR_volume = false;
       IR_station = false;
     }  
   }
   if (irrecv.decode(&results)){
        if (results.value == 0XFFFFFFFF) // repeat
         results.value = key_value;
        previousMillisIR  = millis();
        switch(results.value){
          case 0xFF38C7:  // OK pressed; activate value entered to player        
            if (DEBUG == true) Serial.println("OK");
            if (digitinput == true){
              if (DEBUG == true){
                Serial.print("OK : ");
                Serial.println(IR_value);
              }  
              check_for_input();
              if (IR_volume == true) {
                my_volume = IR_value;  
              } else {
                radioStation = IR_value;
              }
            }  
            digitinput = false; 
            IR_value = 0; 
            Nbr_chars = 0;  
            if (DEBUG == true) { 
              Serial.println("Update IR data to player");
              Serial.print("Volume 1 : ");
              Serial.println(my_volume);
              Serial.print("Radiostation 1 : ");
              Serial.println(radioStation);   
            }  
            if (IR_volume == true) { Update_volume(); }
            if (IR_station == true) { Update_Station(); }
            IR_volume = false;
            IR_station = false;
          break ;    
          case 0xFF18E7:
           if (DEBUG == true) Serial.println("Volume Up (Up)");
           IR_volume = true;
           digitinput = false;
           my_volume++;
           if (my_volume >= 101) my_volume = 100;
           if (DEBUG == true)  {
             Serial.print("Volume 2 : ");  
             Serial.println(my_volume); 
           }  
           display_Volume(my_volume);
           break ;
          case 0xFF10EF:
           if (DEBUG == true) Serial.println("Station terug (Left)");
           IR_station = true;
           digitinput = false;
           radioStation--;
           if (radioStation < 0){
            radioStation = nbrStations-1;
           }
           display_Station(radioStation);
           break ;
          case 0xFF5AA5:
           if (DEBUG == true) Serial.println("Station vooruit (Right)");
           IR_station = true;
           digitinput = false;
           radioStation++;
           if (radioStation >= nbrStations) radioStation = 0;
           if (DEBUG == true) {
             Serial.print("Radiostation 3 : ");
             Serial.println(radioStation);          
           }  
           display_Station(radioStation);
           break ;
          case 0xFF4AB5:
           if (DEBUG == true) Serial.println("Volume Down (Down)");
           IR_volume = true;
           digitinput = false;
           my_volume--;
           if (my_volume < 0) my_volume = 0;
           if (DEBUG == true) {
             Serial.print("Volume 3 : ");
             Serial.println(my_volume); 
           }  
           display_Volume(my_volume);
           break ;                     
          case 0xFFA25D:
            process_IR_Digit(1);
            break;
          case 0xFF629D:
            process_IR_Digit(2);
            break;
          case 0xFFE21D:
            process_IR_Digit(3);
          break;
          case 0xFF22DD:
            process_IR_Digit(4);
            break;
          case 0xFF02FD:
            process_IR_Digit(5);
            break ;  
          case 0xFFC23D:
            process_IR_Digit(6);
            break ;               
          case 0xFFE01F:
            process_IR_Digit(7);
            break ;  
          case 0xFFA857:
            process_IR_Digit(8);
           break ;  
          case 0xFF906F:
            process_IR_Digit(9);
           break ;  
          case 0xFF9867:
            process_IR_Digit(0);
           break ; 
          case 0xFF6897:
            if (DEBUG == true) Serial.println("* - volume");
            if ((IR_volume == false) && (IR_station == false)) {
              if (DEBUG == true) {
                Serial.println(" - setdigitinput");
                Serial.println("*");
              }  
              setdigitinput();
            }
            IR_volume = true;
            IR_station = false; 
            IR_value = 0; 
            Nbr_chars = 0;  
            display_Volume(my_volume);
            break ;  
          case 0xFFB04F:
            if (DEBUG == true) Serial.println("# - station");
            if ((IR_volume == false) && (IR_station == false)) {
              if (DEBUG == true) {
                Serial.println(" - setdigitinput");
                Serial.println("#");
              }  
              setdigitinput();
            }
            IR_volume = false;
            IR_station = true;   
            IR_value = 0; 
            Nbr_chars = 0;           
            display_Station(radioStation); 
            break ;
        }
   // do check on entered data to keep it in right value 0 <-> 100 for volume; 0 <-> nbrStations-1 for stations and update player
   if (digitinput == true) {
     if (DEBUG == true) {
       Serial.print("IR_value 1 : ");
       Serial.println(IR_value);   
     }  
     if (IR_station == true) {
       if (DEBUG == true) Serial.println("IR_station == true");   
       if (IR_value >= nbrStations) {
        if (DEBUG == true) Serial.println("IR_value >= nbrStations");   
        IR_value = 0;
       } else {
        if (IR_value < 0) {
        if (DEBUG == true) Serial.println("IR_value < 0");   
        IR_value = nbrStations-1;
        }        
       }     
       radioStation = IR_value;
       if (DEBUG == true)  {
         Serial.print("Radiostation 2 : ");
         Serial.println(radioStation); 
       }  
       display_Station(radioStation);  
     }
     if (IR_volume == true) {
       if (DEBUG == true) Serial.println("IR_volume == true");   
       if (IR_value > 100) {
        if (DEBUG == true) Serial.println("IR_value > 100");   
        IR_value = 100;
        Nbr_chars = 0; 
       } else {
         if (IR_value < 0) {
           if (DEBUG == true) Serial.println("IR_value < 0");   
           IR_value = 0;       
         }
       }
       my_volume = IR_value;
       if (DEBUG == true) {
...

This file has been truncated, please download it to see its full contents.