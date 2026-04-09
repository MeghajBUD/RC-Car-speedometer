/*
 * Adafruit Ultimate GPS Breakout — Raw UART / No Libraries
 * =========================================================
 * Wiring (Arduino Uno / Mega):
 *   GPS TX  →  Arduino RX1 (pin 19 on Mega) or SoftSerial RX pin
 *   GPS RX  →  Arduino TX1 (pin 18 on Mega) or SoftSerial TX pin
 *   GPS VIN →  3.3 V or 5 V
 *   GPS GND →  GND
 *
 * On the Uno there is only one hardware UART (Serial on pins 0/1,
 * used by USB). This sketch targets the Mega's Serial1; if you are
 * using an Uno, swap every "Serial1" below for a SoftwareSerial
 * instance and uncomment the SoftwareSerial block.
 *
 * The GPS module speaks NMEA 0183 at 9600 baud by default.
 * We parse two sentence types:
 *   $GPRMC – time, status, lat/lon, speed, course
 *   $GPGGA – fix quality, lat/lon, altitude, satellite count
 *
 * All parsing is done by hand (no sscanf, no String class) to keep
 * the sketch lean and deterministic on small AVR targets.
 */

// ---------------------------------------------------------------------------
// Uncomment the three lines below if you are using an Arduino Uno
// and connect GPS TX→pin 4, GPS RX→pin 3.
// ---------------------------------------------------------------------------
// #include <SoftwareSerial.h>
// SoftwareSerial gpsSerial(4, 3); // RX, TX
// #define GPS_SERIAL gpsSerial

#define GPS_SERIAL Serial1          // Change to gpsSerial for Uno

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
#define GPS_BAUD      9600
#define DEBUG_SERIAL  Serial        // USB serial for output
#define DEBUG_BAUD    115200

#define NMEA_MAX_LEN  90            // Maximum NMEA sentence length

// ---------------------------------------------------------------------------
// GPS data structure
// ---------------------------------------------------------------------------
struct GPSData {
  bool    valid;          // True when we have a real fix
  int32_t lat_raw;        // Latitude  ×1 000 000  (positive = N)
  int32_t lon_raw;        // Longitude ×1 000 000  (positive = E)
  int32_t alt_cm;         // Altitude in centimetres
  uint32_t speed_mknots;  // Speed in milli-knots
  uint16_t course_cdeg;   // Course in centi-degrees (0–35999)
  uint8_t  satellites;    // Number of satellites in use
  uint8_t  fix_quality;   // 0=none 1=GPS 2=DGPS
  // UTC time (from last $GPRMC)
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  // UTC date (from last $GPRMC)
  uint8_t  day;
  uint8_t  month;
  uint16_t year;
};

static GPSData gps;

// ---------------------------------------------------------------------------
// NMEA sentence buffer
// ---------------------------------------------------------------------------
static char    nmea_buf[NMEA_MAX_LEN + 1];
static uint8_t nmea_idx = 0;
static bool    in_sentence = false;

// ---------------------------------------------------------------------------
// Utility: parse unsigned integer from a character pointer,
//          advance the pointer past the digits, return the value.
// ---------------------------------------------------------------------------
static uint32_t parse_uint(const char *&p)
{
  uint32_t v = 0;
  while (*p >= '0' && *p <= '9') {
    v = v * 10 + (*p - '0');
    ++p;
  }
  return v;
}

// ---------------------------------------------------------------------------
// Utility: skip to the next comma (or end of string).
// ---------------------------------------------------------------------------
static void skip_field(const char *&p)
{
  while (*p && *p != ',' && *p != '*') ++p;
}

// ---------------------------------------------------------------------------
// Utility: NMEA checksum validation.
// Returns true when the sentence checksum matches the *XX trailer.
// ---------------------------------------------------------------------------
static bool nmea_checksum_ok(const char *sentence)
{
  // sentence starts with '$'
  uint8_t calc = 0;
  const char *p = sentence + 1;       // skip '$'
  while (*p && *p != '*') {
    calc ^= (uint8_t)*p;
    ++p;
  }
  if (*p != '*') return false;        // no checksum present
  ++p;
  // parse two hex digits
  auto hex_val = [](char c) -> uint8_t {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
  };
  uint8_t received = (hex_val(p[0]) << 4) | hex_val(p[1]);
  return calc == received;
}

// ---------------------------------------------------------------------------
// Parse NMEA latitude/longitude fields.
//
// NMEA encodes as DDmm.mmmm[m] where DD = degrees, mm.mmmm = minutes.
// We convert to millionths of a degree (×1 000 000) as a signed int32.
//
// Fields arrive as two consecutive CSV tokens:
//   <value>,<hemisphere>
// e.g.   4807.038,N
// ---------------------------------------------------------------------------
static int32_t parse_latlon(const char *&p, bool is_lon)
{
  // Degrees: 2 digits for lat, 3 for lon
  uint8_t deg_digits = is_lon ? 3 : 2;

  if (*p == ',' || *p == '*' || *p == '\0') {
    skip_field(p); if (*p == ',') ++p;   // empty hemisphere field
    skip_field(p); if (*p == ',') ++p;
    return 0;
  }

  // Read degree part
  uint32_t degrees = 0;
  for (uint8_t i = 0; i < deg_digits; ++i) {
    degrees = degrees * 10 + (*p - '0');
    ++p;
  }

  // Read minutes (integer part)
  uint32_t min_int = parse_uint(p);

  // Read fractional minutes (up to 5 decimal places → scale to 100 000)
  uint32_t min_frac = 0;
  uint8_t  frac_digits = 0;
  if (*p == '.') {
    ++p;
    while (*p >= '0' && *p <= '9' && frac_digits < 5) {
      min_frac = min_frac * 10 + (*p - '0');
      ++p;
      ++frac_digits;
    }
    // Normalise to 5 decimal places
    while (frac_digits < 5) { min_frac *= 10; ++frac_digits; }
    skip_field(p);  // consume any trailing digits beyond 5
  }

  // total minutes ×100 000
  uint32_t min_scaled = min_int * 100000UL + min_frac;

  // Convert to millionths of a degree:
  // deg_millionths = degrees×1 000 000 + (min_scaled / 60) × 10
  //   (min_scaled is ×100 000, dividing by 60 gives ×1 667, ×10 = ×16 667 ≈ correct)
  // More precisely: result = degrees * 1 000 000 + min_scaled * 1 000 000 / 60 / 100 000
  //                        = degrees * 1 000 000 + min_scaled * 10 / 6
  int32_t result = (int32_t)degrees * 1000000L
                 + (int32_t)(min_scaled * 10UL / 6UL);

  if (*p == ',') ++p;   // skip comma before hemisphere

  // Apply sign from hemisphere character
  char hemi = *p;
  skip_field(p);
  if (*p == ',') ++p;

  if (hemi == 'S' || hemi == 'W') result = -result;
  return result;
}

// ---------------------------------------------------------------------------
// Parse $GPRMC sentence.
// Format:
//  $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
//         0         1 2       3 4         5 6   7   8       9   10
// ---------------------------------------------------------------------------
static void parse_gprmc(const char *p)
{
  // Skip tag ($GPRMC,)
  skip_field(p); if (*p == ',') ++p;

  // Field 0: hhmmss.ss
  if (*p != ',') {
    uint32_t t = parse_uint(p);
    gps.second = t % 100; t /= 100;
    gps.minute = t % 100; t /= 100;
    gps.hour   = t % 100;
    skip_field(p);   // skip fractional seconds
  }
  if (*p == ',') ++p;

  // Field 1: Status A=active V=void
  char status = *p;
  gps.valid = (status == 'A');
  skip_field(p); if (*p == ',') ++p;

  // Fields 2,3: Latitude + hemisphere
  gps.lat_raw = parse_latlon(p, false);

  // Fields 4,5: Longitude + hemisphere
  gps.lon_raw = parse_latlon(p, true);

  // Field 6: Speed over ground (knots × 10, stored as milli-knots)
  {
    uint32_t spd_int  = parse_uint(p);
    uint32_t spd_frac = 0;
    if (*p == '.') { ++p; spd_frac = parse_uint(p); }
    gps.speed_mknots = spd_int * 1000 + spd_frac * 100;
    skip_field(p); if (*p == ',') ++p;
  }

  // Field 7: Course over ground (degrees × 100, stored as centi-degrees)
  {
    uint32_t crs_int  = parse_uint(p);
    uint32_t crs_frac = 0;
    if (*p == '.') { ++p; crs_frac = parse_uint(p); }
    gps.course_cdeg = (uint16_t)(crs_int * 100 + crs_frac * 10);
    skip_field(p); if (*p == ',') ++p;
  }

  // Field 8: ddmmyy
  if (*p != ',') {
    uint32_t d = parse_uint(p);
    gps.year  = 2000 + (d % 100); d /= 100;
    gps.month = d % 100;          d /= 100;
    gps.day   = d % 100;
    skip_field(p);
  }
  // remaining fields not needed for basic position
}

// ---------------------------------------------------------------------------
// Parse $GPGGA sentence.
// Format:
//  $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,,*hh
//         0         1       2 3         4 5 6  7   8       9
// ---------------------------------------------------------------------------
static void parse_gpgga(const char *p)
{
  // Skip tag
  skip_field(p); if (*p == ',') ++p;

  // Field 0: time (already handled by RMC; skip)
  skip_field(p); if (*p == ',') ++p;

  // Fields 1,2: Latitude + hemisphere
  gps.lat_raw = parse_latlon(p, false);

  // Fields 3,4: Longitude + hemisphere
  gps.lon_raw = parse_latlon(p, true);

  // Field 5: Fix quality
  gps.fix_quality = (uint8_t)parse_uint(p);
  if (*p == ',') ++p;

  // Field 6: Number of satellites
  gps.satellites = (uint8_t)parse_uint(p);
  if (*p == ',') ++p;

  // Field 7: HDOP (skip)
  skip_field(p); if (*p == ',') ++p;

  // Field 8: Altitude (metres, decimal)
  {
    uint32_t alt_m    = parse_uint(p);
    uint32_t alt_frac = 0;
    if (*p == '.') { ++p; alt_frac = parse_uint(p); }
    gps.alt_cm = (int32_t)(alt_m * 100 + alt_frac);  // store as cm
    skip_field(p); if (*p == ',') ++p;
  }
  // 'M' unit field — skip the rest
}

// ---------------------------------------------------------------------------
// Process one complete NMEA sentence.
// ---------------------------------------------------------------------------
static void process_sentence(const char *s)
{
  if (!nmea_checksum_ok(s)) return;   // discard corrupt sentences

  if (strncmp(s, "$GPRMC", 6) == 0 ||
      strncmp(s, "$GNRMC", 6) == 0) {
    parse_gprmc(s);
  } else if (strncmp(s, "$GPGGA", 6) == 0 ||
             strncmp(s, "$GNGGA", 6) == 0) {
    parse_gpgga(s);
  }
  // Other sentences ($GPGSV, $GPVTG, etc.) are silently ignored.
}

// ---------------------------------------------------------------------------
// Feed one byte from the GPS UART into the sentence assembler.
// ---------------------------------------------------------------------------
static void gps_feed(char c)
{
  if (c == '$') {
    in_sentence = true;
    nmea_idx    = 0;
    nmea_buf[0] = '\0';
  }

  if (!in_sentence) return;

  if (c == '\r' || c == '\n') {
    if (nmea_idx > 0) {
      nmea_buf[nmea_idx] = '\0';
      process_sentence(nmea_buf);
    }
    in_sentence = false;
    nmea_idx    = 0;
    return;
  }

  if (nmea_idx < NMEA_MAX_LEN) {
    nmea_buf[nmea_idx++] = c;
  } else {
    // Overlong line — discard
    in_sentence = false;
    nmea_idx    = 0;
  }
}

// ---------------------------------------------------------------------------
// Pretty-print helpers
// ---------------------------------------------------------------------------

// Print a ×1 000 000 coordinate as  DD.MMMMMM°
static void print_coord(int32_t v, char pos_ch, char neg_ch)
{
  if (v < 0) { v = -v; DEBUG_SERIAL.print(neg_ch); }
  else          DEBUG_SERIAL.print(pos_ch);

  uint32_t deg  = (uint32_t)v / 1000000UL;
  uint32_t frac = (uint32_t)v % 1000000UL;

  DEBUG_SERIAL.print(deg);
  DEBUG_SERIAL.print('.');
  // Print leading zeros for fractional part
  for (uint32_t d = 100000UL; d > 1; d /= 10) {
    if (frac < d) DEBUG_SERIAL.print('0');
  }
  DEBUG_SERIAL.print(frac);
  DEBUG_SERIAL.print('^');   // degree symbol substitute
}

static void print_zero_padded(uint8_t v)
{
  if (v < 10) DEBUG_SERIAL.print('0');
  DEBUG_SERIAL.print(v);
}

static void print_gps()
{
  DEBUG_SERIAL.println(F("--- GPS Position ---"));

  // Fix status
  DEBUG_SERIAL.print(F("Fix     : "));
  if (!gps.valid) {
    DEBUG_SERIAL.println(F("NO FIX (waiting for satellites…)"));
    return;
  }
  const char *fix_str[] = {"None", "GPS", "DGPS"};
  DEBUG_SERIAL.println(fix_str[min((uint8_t)2, gps.fix_quality)]);

  // UTC time & date
  DEBUG_SERIAL.print(F("UTC     : "));
  print_zero_padded(gps.hour);   DEBUG_SERIAL.print(':');
  print_zero_padded(gps.minute); DEBUG_SERIAL.print(':');
  print_zero_padded(gps.second); DEBUG_SERIAL.print(F("  "));
  print_zero_padded(gps.day);    DEBUG_SERIAL.print('/');
  print_zero_padded(gps.month);  DEBUG_SERIAL.print('/');
  DEBUG_SERIAL.println(gps.year);

  // Latitude / Longitude
  DEBUG_SERIAL.print(F("Lat     : "));
  print_coord(gps.lat_raw, 'N', 'S');
  DEBUG_SERIAL.println();

  DEBUG_SERIAL.print(F("Lon     : "));
  print_coord(gps.lon_raw, 'E', 'W');
  DEBUG_SERIAL.println();

  // Altitude
  DEBUG_SERIAL.print(F("Alt     : "));
  DEBUG_SERIAL.print(gps.alt_cm / 100);
  DEBUG_SERIAL.print('.');
  uint8_t alt_frac = gps.alt_cm % 100;
  if (alt_frac < 10) DEBUG_SERIAL.print('0');
  DEBUG_SERIAL.print(alt_frac);
  DEBUG_SERIAL.println(F(" m"));

  // Speed
  DEBUG_SERIAL.print(F("Speed   : "));
  uint32_t spd_kt = gps.speed_mknots / 1000;
  uint32_t spd_ms = (gps.speed_mknots * 514) / 1000000UL;   // ×0.514 m/s per knot
  DEBUG_SERIAL.print(spd_kt);
  DEBUG_SERIAL.print(F(" kn  (≈"));
  DEBUG_SERIAL.print(spd_ms);
  DEBUG_SERIAL.println(F(" m/s)"));

  // Course
  DEBUG_SERIAL.print(F("Course  : "));
  DEBUG_SERIAL.print(gps.course_cdeg / 100);
  DEBUG_SERIAL.print('.');
  DEBUG_SERIAL.print(gps.course_cdeg % 100);
  DEBUG_SERIAL.println(F(" deg"));

  // Satellites
  DEBUG_SERIAL.print(F("Sats    : "));
  DEBUG_SERIAL.println(gps.satellites);
  DEBUG_SERIAL.println();
}

// ---------------------------------------------------------------------------
// Arduino setup / loop
// ---------------------------------------------------------------------------
void setup()
{
  DEBUG_SERIAL.begin(DEBUG_BAUD);
  while (!DEBUG_SERIAL) { /* wait for USB CDC on Leonardo/32u4 */ }

  GPS_SERIAL.begin(GPS_BAUD);

  memset(&gps, 0, sizeof(gps));
  DEBUG_SERIAL.println(F("Adafruit Ultimate GPS — raw UART demo"));
  DEBUG_SERIAL.println(F("Waiting for NMEA data…\n"));
}

static uint32_t last_print_ms = 0;
#define PRINT_INTERVAL_MS 2000      // Print position every 2 s

void loop()
{
  // Drain the GPS UART into the sentence assembler as fast as possible
  while (GPS_SERIAL.available()) {
    gps_feed((char)GPS_SERIAL.read());
  }

  // Print parsed data at a human-readable rate
  uint32_t now = millis();
  if (now - last_print_ms >= PRINT_INTERVAL_MS) {
    last_print_ms = now;
    print_gps();
  }
}
